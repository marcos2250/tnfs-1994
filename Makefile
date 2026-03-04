CC=gcc

ifeq ($(OS),Windows_NT)
LIBS=-L"C:/Dev-Cpp/lib" -lSDL2 -lSDL2main -lm -lopengl32 -lglu32
INCL=-I"C:/Dev-Cpp/include/SDL2"  
else
LIBS=-lSDL2 -lSDL2main -lm -lGL -lGLU
INCL=-I/usr/include/SDL2
endif

BUILD_MODE ?= release

CFLAGS=-Wall
ifeq ($(BUILD_MODE),debug)
	CFLAGS += -g3 -O0
else
	CFLAGS += -O2
endif

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
	tnfs_sfx.c \
	tnfs_front.c \
	tnfs_main.c
	
all: tnfs

clean:
	rm -f build/*
	mkdir -p build

debug:
	@$(MAKE) BUILD_MODE=debug tnfs

release:
	@$(MAKE) BUILD_MODE=release tnfs
	
tnfs:
	$(CC) $(OBJS) $(INCL) $(LIBS) $(CFLAGS) -o build/tnfs 
	
run:
	build/tnfs
