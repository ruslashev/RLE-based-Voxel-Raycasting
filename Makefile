src = \
      RLE-Raycaster/src/Bmp.cpp \
      RLE-Raycaster/src/Core.cpp \
      RLE-Raycaster/src/DrawUtils.cpp \
      RLE-Raycaster/src/GL_Main.cpp \
      RLE-Raycaster/src/RayMap.cpp \
      RLE-Raycaster/src/Rle4.cpp \
      RLE-Raycaster/src/Tree.cpp \
      RLE-Raycaster/src/VecMath.cpp \
      RLE-Raycaster/src/glsl.cpp \
      RLE-Raycaster/src/main.cpp

cflags = \
         -I ./RLE-Raycaster/inc \
         -I ./RLE-Raycaster/src \
         -Wno-write-strings \
         -fpermissive \
         -g \
         -w

lflags = -lglut -lGLU -lGLEW -lGL ./Cuda_Main.o -L /usr/local/cuda-11.8/targets/x86_64-linux/lib -lcudart

cxx = g++

all: main
	./main

main: Cuda_Main.o $(src)
	@echo g++ $@
	@$(cxx) $(cflags) $(src) -o main $(lflags)

Cuda_Main.o: ./RLE-Raycaster/src/Cuda_Main.cu
	@echo nvcc $^
	@nvcc $^ -c -I RLE-Raycaster/inc -ccbin g++-11 -w -Xptxas -fastimul --maxrregcount=64 --use_fast_math 

clean:
	rm -f Cuda_Main.o main
