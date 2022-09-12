CUDAFLAGS=-g -std=c++14 -DNDEBUG -O3
LDLIBS=-lm
CUDA=


all: cuda

CUDA+=kernel
kernel: kernel.cu
	        nvcc $(CUDAFLAGS) $< -o $@ $(LDLIBS)

CUDA+=float-kernel
float-kernel: float-kernel.cu
	        nvcc $(CUDAFLAGS) $< -o $@ $(LDLIBS)

cuda: $(CUDA)


clean:
	rm -f *.o *.d $(CUDA)

