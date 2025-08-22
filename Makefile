CC=gcc
LIBS=-lSDL2 -lSDL2main -lm -lGL -lGLU
INCL=-I/usr/include/SDL2

OBJS= \
	ccb.c \
	tnfs_math.c \
	tnfs_base.c \
	tnfs_collision_2d.c \
	tnfs_collision_3d.c \
	tnfs_camera.c \
	tnfs_fiziks.c \
	tnfs_ai.c \
	tnfs_files.c \
	tnfs_gfx.c \
	tnfs_front.c \
	tnfs_main.c
	
all: compile

clean:
	rm -f build/*
	mkdir -p build

compile_debug:
	gcc $(OBJS) $(INCL) $(LIBS) -O2 -g3 -Wall -o build/tnfs 

compile:
	gcc $(OBJS) $(INCL) $(LIBS) -g3 -Wall -o build/tnfs 
	
run:
	build/tnfs
