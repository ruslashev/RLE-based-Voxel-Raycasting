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

obj = $(src:RLE-Raycaster/src/%.cpp=obj/%.o) obj/Cuda_Main.o

cflags = \
         -I ./RLE-Raycaster/inc \
         -I ./RLE-Raycaster/src \
         -Wno-write-strings \
         -fpermissive \
         -g \
         -w

lflags = -lglut -lGLEW -lGL -L /usr/local/cuda-11.8/targets/x86_64-linux/lib -lcudart

nflags = -I ./RLE-Raycaster/inc -ccbin g++-11 -w -Xptxas -fastimul --maxrregcount=64 --use_fast_math -lineinfo

cxx = g++

bin = main

all: $(bin)
	./$(bin)

$(bin): $(obj)
	@echo ld $@
	@$(cxx) $^ -o $@    $(lflags)

obj/%.o: RLE-Raycaster/src/%.cpp
	@echo cxx $<
	@mkdir -p $(@D)
	@$(cxx) $^ -o $@ -c $(cflags)

obj/Cuda_Main.o: RLE-Raycaster/src/Cuda_Main.cu
	@echo nvcc $^
	@mkdir -p $(@D)
	@nvcc   $^ -o $@ -c $(nflags)

clean:
	rm -rf obj
