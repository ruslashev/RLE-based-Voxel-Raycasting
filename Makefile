src = \
      src/alloc.cc \
      src/gl_main.cc \
      src/ray_map.cc \
      src/rle4.cc \
      src/glsl.cc \
      src/main.cc

obj = $(src:src/%.cc=obj/%.o) obj/cuda_main.o

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

obj/%.o: src/%.cc
	@echo cxx $<
	@mkdir -p $(@D)
	@$(cxx) $^ -o $@ -c $(cflags)

obj/cuda_main.o: src/cuda_main.cu
	@echo nvcc $^
	@mkdir -p $(@D)
	@nvcc   $^ -o $@ -c $(nflags)

clean:
	rm -rf obj
