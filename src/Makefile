# An useful reference:
#  https://www.tutorialspoint.com/makefile/makefile_quick_guide.htm
CXXFLAGS:=-Wall -Wextra -ggdb -O3
CUDAFLAGS:=-g -O3 -DGPU -x cu
EXE:=exe
CUDAEXE:=cuda
CC:=g++
NVCC:=nvcc
LDFLAGS:=-flto

all: build

OBJS+=argparser.o
argparser.o: argparser.cc argparser.hh

OBJS+=main.o
main.o: main.cc argparser.hh

FILES=main.cc argparser.cc
DEPS=$(FILES:.cc=.d)
OBJS=$(FILES:.cc=.o)

build: $(EXE)

$(EXE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.d: %.cc
	$(CC) -MM -MF $@ $<
-include $(DEPS)

# nvcc  main.cu argparser.cc -o cuda
$(CUDAEXE): $(OBJS)
	$(NVCC) $(CUDAFLAGS) main.cc argparser.cc -o cuda

clean:
	rm -f $(EXE) $(OBJS) $(DEPS) $(CUDAEXE)

