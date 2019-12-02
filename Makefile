CC      = g++

CXXFLAGS	= -g -std=c++11

#platform dependent variables
ifeq ("$(shell uname)", "Darwin")
  LDFLAGS     = -framework Foundation -framework GLUT -framework OpenGL -lOpenImageIO -lm
else
  ifeq ("$(shell uname)", "Linux")
    LDFLAGS     = -L /usr/lib64/ -lglut -lGL -lGLU -lOpenImageIO -lm -lboost_program_options
  endif
endif

PROJECT = feedback

OBJECTS = feedback.o #image.o matrix.o

all: feedback

o2: CXXFLAGS += -O2
o2: ${PROJECT}

${PROJECT} : ${OBJECTS} 
	${CC} ${CXXFLAGS} -o ${PROJECT} ${OBJECTS} ${LDFLAGS} 

%.o: %.cpp
	${CC} -c ${CXXFLAGS} $<

clean:
	rm -f core.* *.o *~ ${PROJECT}