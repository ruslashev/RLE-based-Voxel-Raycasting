src = \
      src/alloc.cpp \
      src/Core.cpp \
      src/GL_Main.cpp \
      src/RayMap.cpp \
      src/Rle4.cpp \
      src/VecMath.cpp \
      src/glsl.cpp \
      src/main.cpp

obj = $(src:src/%.cpp=obj/%.o) obj/Cuda_Main.o

cflags = \
         -I inc \
         -I src \
         -Wno-write-strings \
         -fpermissive \
         -g \
         -w

lflags = -lglut -lGLEW -lGL -L /usr/local/cuda-11.8/targets/x86_64-linux/lib -lcudart

nflags = -I inc -ccbin g++-11 -w -Xptxas -fastimul --maxrregcount=64 --use_fast_math -lineinfo

cxx = g++

bin = main

all: $(bin)
	./$(bin)

$(bin): $(obj)
	@echo ld $@
	@$(cxx) $^ -o $@    $(lflags)

obj/%.o: src/%.cpp
	@echo cxx $<
	@mkdir -p $(@D)
	@$(cxx) $^ -o $@ -c $(cflags)

obj/Cuda_Main.o: src/Cuda_Main.cu
	@echo nvcc $^
	@mkdir -p $(@D)
	@nvcc   $^ -o $@ -c $(nflags)

clean:
	rm -rf obj
