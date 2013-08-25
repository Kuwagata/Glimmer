CC=g++
DEBUG=-g
CFLAGS=-Wall -c $(DEBUG)
LFLAGS=-Wall $(DEBUG)
LIBS=-lglut -lGL -lGLU -lGLEW -lm
SOURCES=cpu.cpp data.cpp fbo.cpp feeder.cpp glimmer.cpp graph.cpp mmio.c shader.cpp sh_center.cpp sh_dist.cpp sh_dist_d.cpp sh_dist_g.cpp sh_force.cpp sh_force_sum.cpp sh_rand.cpp sh_sort.cpp sh_stress.cpp texture.cpp vbo.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=glimmer

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LFLAGS) $(OBJECTS) -o $@ $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

clean:
	 rm -rf *.o glimmer
