#include <iostream>
#include <random>
#include <string>

#include <OpenImageIO/imageio.h>

// makes my life easier for parsing input
#include <boost/program_options.hpp>

// this stores function pointers. They're applied in order for each frame
std::vector<std::function<void()>> operations;

std::mt19937 mtrand;
uint32_t seed;

void seedRand(std::string s) {
  if (s == "") {
    std::random_device rd;
    seed = rd();
    mtrand.seed(seed);
  } else {
    std::seed_seq sd(s.begin(), s.end());
    mtrand.seed(sd);
  }
}

int main(int argc, char *argv[]) {

  // Define input options (I used boost, because I'm tired of writing this out)
  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()("help,h", "Display help message")(
      "seed,s", po::value<std::string>()->default_value("")->notifier(seedRand),
      "Seed for random generator");

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << "Video feedback loop simulator" << std::endl
              << desc << std::endl;
    exit(EXIT_SUCCESS);
  }

  std::cout << mtrand() % 255 << std::endl;
}