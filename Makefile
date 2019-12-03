CC      = g++

CXXFLAGS	= -std=c++11

#platform dependent variables
ifeq ("$(shell uname)", "Darwin")
  LDFLAGS     = -framework Foundation
else
  ifeq ("$(shell uname)", "Linux")
    LDFLAGS     = -L /usr/lib64/
  endif
endif

LDFLAGS += -lOpenImageIO -lm -lboost_program_options

PROJECT = feedback

OBJECTS = feedback.o #image.o matrix.o

all: CXXFLAGS += -g
all: feedback

o2: CXXFLAGS += -O2
o2: ${PROJECT}

${PROJECT} : ${OBJECTS} 
	${CC} ${CXXFLAGS} -o ${PROJECT} ${OBJECTS} ${LDFLAGS} 

%.o: %.cpp
	${CC} -c ${CXXFLAGS} $<

clean:
	rm -f core.* *.o *~ ${PROJECT}

video:
	ffmpeg -r 20 -f image2 -s 1920x1080 -i output/frame%d.png -vcodec libx264 -crf 25  -pix_fmt yuv420p vid.mp4