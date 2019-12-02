#include <iostream>
#include <random>
#include <string>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>

// makes my life easier for parsing input
#include <boost/program_options.hpp>

OIIO_NAMESPACE_USING

const float pi = 3.1415926;

// this stores function pointers. They're applied in order for each frame
std::vector<std::function<void()>> operations;

// image parameters
ImageSpec iparams;

// the image we'll be displaying
ImageBuf image;

// used for blending
ImageBuf oldImage;

// manipulation parameters

ImageBuf gaussianKernel; // kernel used for gaussian blurs
float sharpen;           // contrast value for sharpening
float noise;             // range for noise [0, 1]
float rotate;            // degrees to rotate camera
float zoom;              // zoom in/out
float blend;             // weight the old image when blending
int framelimit;          // when to stop

// output directory
std::string outdir = "";

int height = 500;
int width = 300;

std::mt19937 mtrand;
int seed;
int frame = 0;

void seedRand(int s) { mtrand.seed(seed); }

void writeFile() {
  if (outdir != "")
    outdir += '/';
  std::string outname = outdir + "frame" + std::to_string(frame) + ".png";
  auto out = ImageOutput::create(outname);
  if (!out) {
    std::cout << "Couldn't write out file" << std::endl;
    exit(1);
  }
  ImageSpec spec(width, height, 3, TypeDesc::UINT8);
  out->open(outname, spec);
  std::vector<float> pixels(width * height * 3);
  image.get_pixels({0, width, 0, height}, TypeDesc::FLOAT, &pixels[0]);
  out->write_image(TypeDesc::FLOAT, &pixels[0]);
  out->close();

  // newer versions of OIIO use a unique_ptr
#if OIIO_VERSION < 10903
  ImageOutput::destroy(out);
#endif
}

void applyGaussian() { image = ImageBufAlgo::convolve(image, gaussianKernel); }
void applyUnsharp() {
  image = ImageBufAlgo::unsharp_mask(image, "gaussian", 3, sharpen);
}
void applyNoise() {
  ImageBufAlgo::noise(image, "uniform", -noise, noise, false, mtrand());
}
void applyRotate() { image = ImageBufAlgo::rotate(image, rotate); }
void applyZoom() {
  int offsetWidth = int(0.5 * width * (zoom - 1));
  int offsetHeight = int(0.5 * height * (zoom - 1));
  image = ImageBufAlgo::cut(image, {offsetWidth, width - offsetWidth,
                                    offsetHeight, height - offsetHeight});
  image =
      ImageBufAlgo::resize(image, "nuke-lanczos6", 0, {0, width, 0, height});
}
void applyBlend() {
  ImageBufAlgo::mul(image, image, 1 - blend);
  ImageBufAlgo::mul(oldImage, oldImage, blend);
  image = ImageBufAlgo::add(image, oldImage);
}

void addGaussian(int k) {
  operations.push_back(applyGaussian);
  gaussianKernel = ImageBufAlgo::make_kernel("gaussian", k, k);
}
void addUnsharp(float s) { operations.push_back(applyUnsharp); }
void addNoise(float n) { operations.push_back(applyNoise); }
void addRotate(float r) {
  operations.push_back(applyRotate);
  rotate = r * pi / 180;
}
void addZoom(float z) { operations.push_back(applyZoom); }
void addBlend(float b) { operations.push_back(applyBlend); }

int main(int argc, char *argv[]) {

  // Define input options (I used boost, because I'm tired of writing this out)
  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()                     //
      ("help,h", "Display help message") //
      ("seed",
       po::value<int>(&seed)
           ->default_value(std::random_device()())
           ->notifier(seedRand),
       "Seed for random generator") //
      ("width", po::value<int>(&width)->default_value(500),
       "Width of display") //
      ("height", po::value<int>(&height)->default_value(300),
       "Height of display")                                                  //
      ("outdir,o", po::value<std::string>(&outdir), "Output file directory") //
      ("limit", po::value<int>(&framelimit)->default_value(300),
       "Frame limit") //
      ("gaussian,g", po::value<int>()->default_value(3)->notifier(addGaussian),
       "Apply a gaussian blur of specified kernel size") //
      ("sharpen,s",
       po::value<float>(&sharpen)->default_value(1.0)->notifier(addUnsharp),
       "Pass in the contrast value to sharpen the image. Uses an unsharpen "
       "mask with a 3x3 gaussian kernel.") //
      ("noise,n", po::value<float>(&noise)->notifier(addNoise),
       "Add noise to each frame, in the range [-n,n]") //
      ("rotate,r", po::value<float>()->notifier(addRotate),
       "Rotate the camera") //
      ("zoom,z", po::value<float>(&zoom)->notifier(addZoom),
       "Zoom the camera") //
      ("blend,b", po::value<float>(&blend)->notifier(addBlend),
       "Weight for blending in the last frame");

  po::variables_map vm;
  auto parsed = po::parse_command_line(argc, argv, desc);
  po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
  if (vm.count("help")) {
    std::cout << "Video feedback loop simulator" << std::endl
              << desc << std::endl;
    exit(EXIT_SUCCESS);
  }
  // we always want these to be notified and set
  desc.find("seed", false).semantic()->notify(vm["seed"].value());
  desc.find("width", false).semantic()->notify(vm["width"].value());
  desc.find("height", false).semantic()->notify(vm["height"].value());
  desc.find("limit", false).semantic()->notify(vm["limit"].value());

  // start with an image of uniform
  iparams = ImageSpec(width, height, 3, TypeDesc::FLOAT);
  image = ImageBuf(iparams);
  ImageBufAlgo::noise(image, "uniform", 0.0f, 1.0f, false, seed);

  // build the pipeline
  for (auto &x : parsed.options) {
    desc.find(x.string_key, false).semantic()->notify(vm[x.string_key].value());
  }

  writeFile();
  while (true) {
    frame++;
    std::cout << "\rframe " << frame << std::flush;
    oldImage = image;
    for (int i = 0; i < operations.size(); i++) {
      operations[i]();
    }
    writeFile();
  }
}