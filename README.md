# 2D Game Engine

### Installation

To run this game engine, you must install the SDL-related libraries.

For Debian (MathLAN included) / Ubuntu Linux Versions, run the following commands:

```
sudo apt-get update
sudo apt-get install libsdl2-2.0
sudo apt-get install libsdl2-dev
sudo apt-get install libsdl2-image-dev
sudo apt-get install libsdl2-mixer-dev
```
From there, download the game engine files. If it is in a zip file, then extract that into its own folder.

Using the `cd` command, navigate to the folder containing `game.c` and `makefile` and run:

```
make
```

Once the program has compiled, run:

```
./game
```

### Running the Base Game 

The controls are simple.

Type 'w' to move the player sprite up.
Type 's' to move the player sprite down.
Type 'a' to move the player sprite left.
Type 'd' to move the player sprite right.

Type 'ENTER' to switch menus, and type 'w' or 's' to navigate menus accordingly.

Move the player sprite towards an exit point to switch to a different map.

The player cannot run past walls nor can the player move further from the edge of the window.
