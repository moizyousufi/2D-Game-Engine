# Define the compiler
CC = g++

# Define any compile-time flags
CFLAGS = -Wall -Wextra -std=c++11 `sdl2-config --cflags` `pkg-config --cflags SDL2_mixer`

# Define any directories containing header files
INCLUDES = 

# Define library paths in addition to /usr/lib
LFLAGS = `sdl2-config --libs` `pkg-config --libs SDL2_mixer`

# Define any libraries to link into executable
LIBS = -lSDL2_image -pthread -lSDL2_mixer

# Automatically set the source files to game.cpp
SRCS = game.cpp

# Define the object files
OBJS = $(SRCS:.cpp=.o)

# Define the executable file 
MAIN = game

.PHONY: depend clean

all:    $(MAIN)
	@echo  My program has been compiled

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# Update the compilation rule to reflect C++ source files (.cpp)
.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^
