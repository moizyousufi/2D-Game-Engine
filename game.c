// libraries being used for this project
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_image.h> // make sure to include the SDL_image library for sprites
#include <SDL2/SDL_mixer.h> // includes the SDL audio mixer
#include <pthread.h>
#include <sys/stat.h> // for mkdir

// macros for commonly used values to make easier readability
#define TILE_WIDTH 16
#define TILE_HEIGHT 16
#define X_OFFSET 8
#define MAP_ROWS 9
#define MAP_COLS 10
#define FPS 1200
#define X_RESOLUTION 160
#define Y_RESOLUTION 144
#define MOVEMENT_DELAY 150
#define RES_SCALE 8
#define MENU_ITEM_COUNT 3
#define SPRITE_FRAMES 2 // the frames per direction
#define ANIMATION_DELAY 125 // the millisecond delay between frames
#define FRAME_DELAY 1000 / FPS // frame delay for 60 fps and making sure CPU does not run 100%
#define MAX_GAME_TEXTURES 1000 // maximum number of textures that can be loaded for the game

// I didn't want to include math.h because I was purely dealing with integers
// instead I decided to use these trivial macros for min and maxing
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// global variable that will allow our threads to sync properly
int musicSelector = 0; // initial music selection

// Variables for sprite animation
int currentFrame = 0;
Uint32 lastAnimationFrame = 0;
int animationRowHeight = TILE_HEIGHT; // assumes that each animation is in a different row

// this will set up for our start menu
typedef enum { MENU, GAME } GameState;
GameState currentGameState = GAME;

// This will set up our menu options
typedef enum { SAVE, LOAD, EXIT } MenuState;
MenuState currentMenuState = SAVE;

// Variables for tracking character direction and state
typedef enum { RIGHT, LEFT, UP, DOWN, IDLE_RIGHT, IDLE_LEFT, IDLE_UP, IDLE_DOWN } Direction;

// Define map types
typedef enum { PERLLERT_TOWN, PKRMN_CTR, VILLAGE_RUINS } MapType;

// Structs for managing game data
typedef struct 
{
    int x, y;
    Direction direction;
    SDL_Texture *sprite;
} Player;

/**
 * This function will initialize SDL and SDL_image
 * 
 * @return void
*/
void initSDL () 
{
  // error checking for SDL and SDL_image
  if (SDL_Init(SDL_INIT_VIDEO) < 0) 
  {
    fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return;
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) 
  {
    fprintf(stderr, "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
    SDL_Quit();
    return;
  }
}

/**
 * This function will set up the window and renderer
 * 
 * @param window the window that will be set up
 * @param renderer the renderer that will be set up
 * 
 * @return void
 */
void setupWindow (SDL_Window** window, SDL_Renderer** renderer) 
{
  // resolution is scaled 8x higher than what will be rendered (1280x1152 screen for 160x144 game)
  *window = SDL_CreateWindow("Perkemerrrrrrnnnnnnn", 
                            SDL_WINDOWPOS_CENTERED, 
                            SDL_WINDOWPOS_CENTERED, 
                            X_RESOLUTION * RES_SCALE, 
                            Y_RESOLUTION * RES_SCALE, 
                            SDL_WINDOW_SHOWN);
  // make sure window runs successfully
  if (!(*window)) 
  {
    fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
    IMG_Quit();
    SDL_Quit();
    return;
  }

  *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);

  // make sure renderer runs successfully
  if (!(*renderer)) 
  {
    fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(*window);
    IMG_Quit();
    SDL_Quit();
    return;
  }

  // set up the render to match our desired resolution
  SDL_RenderSetLogicalSize(*renderer, X_RESOLUTION, Y_RESOLUTION); // resolution is 160x144
}

/**
 * This function will load the textures for the game
 * 
 * @param textures the array of textures to be loaded
 * @param renderer the renderer that will be used to load the textures
 * 
 * @return int the number of textures loaded
 */
int loadTextures (SDL_Texture** textures, SDL_Renderer** renderer) 
{
  // load the textures for the game
  int i = 0;
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/grass_grey.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/wall_grey.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/enter_pkrmrn_ctr.png");
    
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/enter_perllert_town.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/ctr_tile_top_right.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/ctr_tile_top_left.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/ctr_tile_bottom_right.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/ctr_tile_bottom_left.png");
    
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/ctr_wall1.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/ctr_wall2.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/ctr_wall3.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/ctr_wall4.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/village_exit.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/world/perllert1_exit.png");

  return i;
}

/**
 * This function will load the textures for the menu
 * 
 * @param textures the array of textures to be loaded
 * @param renderer the renderer that will be used to load the textures
 * 
 * @return int the number of textures loaded
 */
int loadMenuTextures (SDL_Texture** textures, SDL_Renderer** renderer) 
{
  // load the textures for the menu
  int i = 0;
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/menu/menu_save.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/menu/menu_load.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/menu/menu_load_error.png");
  textures[i++] = IMG_LoadTexture(*renderer, "assets/textures/menu/menu_exit.png");

  return i;
}

/**
 * This function will destroy the textures for the game
 * 
 * @param textures the array of textures to be destroyed
 * @param textureCount the number of textures to be destroyed
 * 
 * @return void
 */
void destroyTextures (SDL_Texture** textures, int textureCount) 
{
  for (int i = 0; i < textureCount; ++i) 
  {
    SDL_DestroyTexture(textures[i]);
  }
}

/**
 * This function will load the map into the game
 * 
 * @param map the map that will be loaded
 * @param mapType the type of map that will be loaded
 */
void loadMap(int map[MAP_ROWS][MAP_COLS], MapType mapType) 
{
  switch (mapType) 
  {
    case PERLLERT_TOWN: 
    {
      // Map layout for Perllert Town
      int perllert_town_map[MAP_ROWS][MAP_COLS] = 
      {
        {1, 1, 1, 1, 1, 1, 1, 1, 12, 1}, // 1 represents a wall, 12 represents an exit point
        {1, 0, 0, 0, 0, 0, 0, 0, 0,  1}, // 0 represents a walkable tile
        {1, 0, 0, 0, 0, 1, 1, 0, 0,  1}, 
        {1, 0, 0, 0, 0, 0, 1, 0, 0,  1}, 
        {1, 0, 0, 0, 0, 0, 1, 0, 0,  1}, 
        {1, 0, 1, 0, 0, 0, 1, 0, 0,  1}, 
        {1, 0, 0, 0, 0, 0, 0, 0, 0,  2}, // 2 represents an exit point
        {1, 0, 0, 0, 0, 0, 0, 0, 0,  2}, 
        {1, 0, 0, 1, 1, 1, 1, 1, 1,  1}, 
      };
      memmove(map, perllert_town_map, sizeof(perllert_town_map));
      break;
    }
    case PKRMN_CTR:
    {
      // Map layout for Perkemern Center
      int pkrmrn_ctr_map[MAP_ROWS][MAP_COLS] = 
      {
        {9, 11, 11, 11, 9, 9, 11, 11, 11, 9}, 
        {9, 5,  4,  5,  9, 9, 4,  5,  4,  9}, 
        {8, 10, 10, 10, 8, 8, 10, 10, 10, 8}, 
        {4, 5,  4,  5,  4, 5, 4,  5,  4,  5}, 
        {6, 7,  6,  7,  6, 7, 6,  7,  6,  7}, 
        {4, 5,  4,  5,  4, 5, 4,  5,  4,  5}, 
        {3, 7,  6,  7,  6, 7, 6,  7,  6,  7}, // 3 represents exit point
        {3, 5,  4,  5,  4, 5, 4,  5,  4,  5}, 
        {6, 7,  6,  7,  6, 7, 6,  7,  6,  7}, 
      };
      memmove(map, pkrmrn_ctr_map, sizeof(pkrmrn_ctr_map));
      break;
    }
    case VILLAGE_RUINS:
    {
      // Map layout for Village Ruins
      int village_ruins_map[MAP_ROWS][MAP_COLS] = 
      {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, 
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 13 represents exit point
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 
        {1, 1, 1, 1, 1, 1, 1, 1, 13, 1}, 
      };
      memmove(map, village_ruins_map, sizeof(village_ruins_map));
      break;
    }
  }
}

/**
 * This function will save the game state to a file
 * 
 * @param x the x position of the player
 * @param y the y position of the player
 * @param currentMap the current map that the player is on
 * @param musicSelector the current music that is playing
 * 
 * @return void
 */
void saveGame (int x, int y, char* currentMap) 
{
  // Try to create a directory for the save file (if it doesn't exist)
  // this works because mkdir doesn't do anything if the directory already exists
  mkdir("save_data", 0777); // 0777 permissions mean everyone can read/write/execute

  // open file from the directory
  // this will create the file if it doesn't exist
  FILE *saveFile = fopen("save_data/save.txt", "w");
  if (saveFile == NULL) 
  {
      fprintf(stderr, "Error opening or creating save file!\n");
      return;
  }

  // save the current map
  fprintf(saveFile, "map: %s\n", currentMap);

  // save the music state
  fprintf(saveFile, "music: %d\n", musicSelector);

  // convert the player's position to grid coordinates and save
  int gridX = (x - X_OFFSET) / TILE_WIDTH;
  int gridY = y / TILE_HEIGHT;
  fprintf(saveFile, "xpos: %d\n", gridX);
  fprintf(saveFile, "ypos: %d\n", gridY);

  fclose(saveFile);
}

/**
 * This function will load the game state from a file
 * 
 * @param loadError the load error variable to determine whether we are in the load error state or not
 * @param player the player struct, intended for the main character
 * @param map the map that will be loaded in association with the current map name
 * @param chooseMap the variable to determine which map to next load
 * 
 * @return void
 */
void loadGame(bool *loadError, Player *player, int map[MAP_ROWS][MAP_COLS], int *chooseMap)   
{
  // load the game
  FILE* saveFile = fopen("save_data/save.txt", "r");
  char line[100]; // Array to hold each line of the file

  // Check if the file exists
  if (saveFile == NULL) 
  {
    *loadError = true;
    return;
  }

  // Read the file line by line
  while (fgets(line, sizeof(line), saveFile) != NULL) 
  {
    // set up our checks in the save file
    char mapPrefix[] = "map: ";
    char musicPrefix[] = "music: ";
    char xposPrefix[] = "xpos: ";
    char yposPrefix[] = "ypos: ";

    // check to see if the line is a map line
    if(strncmp(mapPrefix, line, strlen(mapPrefix)) == 0)
    {
      // ensure that the line has a value
      if(strlen(line) <= strlen(mapPrefix))
      {
        *loadError = true;
        printf("map prefix has no value\n");
        break;
      }

      // we will select the map based on the string after the prefix
      char* mapChoice = line + strlen(mapPrefix);
      
      // now we have the map choice, we can change the map variable
      // 18 is the number of characters in "perllert_town_map", not magic number
      if(strncmp("perllert_town_map", mapChoice, strlen("perllert_town_map")) == 0)
      {
        loadMap(map, PERLLERT_TOWN);
        *chooseMap = 1;
      }
      else if(strncmp("pkrmrn_ctr_map", mapChoice, strlen("pkrmrn_ctr_map")) == 0)
      {
        loadMap(map, PKRMN_CTR);
        *chooseMap = 2;
      }
      else if(strncmp("village_ruins_map", mapChoice, strlen("village_ruins_map")) == 0)
      {
        loadMap(map, VILLAGE_RUINS);
        *chooseMap = 3;
      }
    }
    // check to see if the line is a music line
    else if(strncmp(musicPrefix, line, strlen(musicPrefix)) == 0)
    {
      // ensure that the line has a value
      if(strlen(line) <= strlen(musicPrefix))
      {
        *loadError = true;
        printf("music prefix has no value\n");
        break;
      }

      char musicChoice[100];
      for (int i = (int) strlen(musicPrefix); i < (int) strlen(line) - 1; ++i)
      {                    
        // we will select the music based on the value after the prefix
        musicChoice[i - strlen(musicPrefix)] = line[i];
      }

      musicSelector = atoi(musicChoice);
    }
    // check to see if the line is an xpos line
    else if(strncmp(xposPrefix, line, strlen(xposPrefix)) == 0)
    {
      // ensure that the line has a value
      if(strlen(line) <= strlen(xposPrefix))
      {
        *loadError = true;
        printf("xpos prefix has no value\n");
        break;
      }

      char xChoice[100];
      for (int i = (int) strlen(xposPrefix); i < (int) strlen(line) - 1; ++i)
      {
        // we will select the x pos based on the value after the prefix
        xChoice[i - strlen(xposPrefix)] = line[i];
      }
      // reading in xpos
      (*player).x = atoi(xChoice) * TILE_WIDTH + X_OFFSET;

    }
    // check to see if the line is a ypos line
    else if(strncmp(yposPrefix, line, strlen(yposPrefix)) == 0)
    {
      // ensure that the line has a value
      if(strlen(line) <= strlen(yposPrefix))
      {
        *loadError = true;
        printf("ypos prefix has no value\n");
        break;
      }

      char yChoice[100];
      for (int i = (int) strlen(yposPrefix); i < (int) strlen(line) - 1; ++i)
      {
        // we will select the x pos based on the value after the prefix
        yChoice[i - strlen(yposPrefix)] = line[i];
      }
      // reading in ypos
      (*player).y = atoi(yChoice) * TILE_HEIGHT;
    }
  }
  // close the file for safety
  fclose(saveFile);
  
}

/**
 * This function will handle the events for the game, including user input and save handling
 * 
 * @param isRunning the control variable for the main loop
 * @param currentGameState the current game state
 * @param currentMenuState the current menu state
 * @param player the player struct, intended for the main character
 * @param loadError the load error variable to determine whether we are in the load error state or not
 * @param event the event that will be handled
 * @param currentMapName the current map name
 * @param map the map that will be loaded in association with the current map name
 * @param chooseMap the variable to determine which map to next load
 * 
 * @return void
 */
void HandleEvents(int* isRunning, GameState* currentGameState, MenuState* currentMenuState, Player* player, 
                  bool* loadError, SDL_Event* event, char* currentMapName, int map[MAP_ROWS][MAP_COLS], int* chooseMap) 
{
  // event handling
    while (SDL_PollEvent(event)) 
    {
      switch((*event).type)
      {
        // handle quit event
        case SDL_QUIT:
          *isRunning = 0;
          break;
        // handle key press from user
        case SDL_KEYDOWN:
          switch((*event).key.keysym.sym)
          {
            // handle 'enter' input for switching between game and menu
            case SDLK_RETURN:
              switch(*currentGameState)
              {
                // consider when we are dealing with the menu
                case MENU:
                  // handle the save system 
                  switch (*currentMenuState)
                  {
                    // handle save case
                    case SAVE:
                      // save the game by calling our saveGame function
                      saveGame((*player).x, (*player).y, currentMapName);
                      break;
                    // handle exit menu case
                    case EXIT:
                      // simply change the game state to exit the game
                      *currentGameState = GAME;
                      break;
                    // handle load case
                    case LOAD:
                      // check to see whether we are in the load error state or not
                      if (*loadError) 
                      {
                        *currentMenuState = LOAD;
                        *loadError = false;
                        break;
                      }

                      // load the game by calling our loadGame function
                      loadGame(loadError, player, map, chooseMap);
                      
                      break;
                    default:
                      break;
                  }
                  break;
                // consider when we are dealing with the game
                case GAME:
                  // simply change to the menu state
                  *currentGameState = MENU;
                  *currentMenuState = SAVE;
                  break;
                default:
                  break;
              }
              break;
            // handle 'up' input for switching between menu options
            case SDLK_w:
              if (*currentMenuState > 0 && !(*loadError)) 
              {
                --(*currentMenuState);
              }
              break;
            // handle 'down' input for switching between menu options
            case SDLK_s:
              if ((*currentMenuState) < MENU_ITEM_COUNT - 1 && !(*loadError)) 
              {
                ++(*currentMenuState);
              }
              break;
          }
          break;
      }
    }
}

/**
 * This function will calculate the source rectangle for the sprite animation
 * 
 * @param srcRect the source rectangle for the sprite
 * @param direction the direction that the sprite is facing
 * @param currentFrame the current frame of the sprite animation
 * 
 * @return void
 */
void calculateSrcRect(SDL_Rect *srcRect, Direction direction, int currentFrame) 
{
    srcRect->w = TILE_WIDTH;
    srcRect->h = TILE_HEIGHT;

    // Idle frames are all in the first row, walking frames are in subsequent rows
    switch (direction) 
    {
        case IDLE_RIGHT:
            srcRect->x = 0 * TILE_WIDTH;
            srcRect->y = 0;
            break;
        case IDLE_LEFT:
            srcRect->x = 1 * TILE_WIDTH;
            srcRect->y = 0;
            break;
        case IDLE_UP:
            srcRect->x = 2 * TILE_WIDTH;
            srcRect->y = 0;
            break;
        case IDLE_DOWN:
            srcRect->x = 3 * TILE_WIDTH;
            srcRect->y = 0;
            break;
        case RIGHT:
            srcRect->x = currentFrame * TILE_WIDTH;
            srcRect->y = 1 * TILE_HEIGHT;
            break;
        case LEFT:
            srcRect->x = currentFrame * TILE_WIDTH;
            srcRect->y = 2 * TILE_HEIGHT;
            break;
        case UP:
            srcRect->x = currentFrame * TILE_WIDTH;
            srcRect->y = 3 * TILE_HEIGHT;
            break;
        case DOWN:
            srcRect->x = currentFrame * TILE_WIDTH;
            srcRect->y = 4 * TILE_HEIGHT;
            break;
    }
}

/**
 * This function will render the scene based on the current state
 * 
 * @param renderer the renderer that will be used to render the scene
 * @param currentGameState the current game state
 * @param currentMenuState the current menu state
 * @param player the player struct, intended for the main character
 * @param loadError the load error variable to determine whether we are in the load error state or not
 * @param currentMapName the current map name
 * @param map the map that will be loaded in association with the current map name
 * @param menuTextures the textures for the menu
 * @param gameTextures the textures for the game
 * @param lastMoveTime the time of the last movement
 * @param chooseMap the variable to determine which map to next load
 * 
 * @return void
 */
void render(SDL_Renderer** renderer, GameState* currentGameState, MenuState* currentMenuState, Player* player, 
            bool* loadError, char** currentMapName, int map[MAP_ROWS][MAP_COLS], SDL_Texture** menuTextures, 
            SDL_Texture** gameTextures, Uint32* lastMoveTime, int* chooseMap)
{
  // Render the scene based on the current state
    switch(*currentGameState) 
    {
      // render the menu case
      case MENU:
        // simply show the appropriate menu
        switch(*currentMenuState)
        {
          case SAVE:
            SDL_RenderCopy(*renderer, menuTextures[0], NULL, NULL);
            break;
          case LOAD:
            if(*loadError) SDL_RenderCopy(*renderer, menuTextures[2], NULL, NULL);
            else SDL_RenderCopy(*renderer, menuTextures[1], NULL, NULL);
            break;
          case EXIT:
            SDL_RenderCopy(*renderer, menuTextures[3], NULL, NULL);
            break;
          default:
            break;
        }
        break;
      // render the game case
      case GAME:
        // setup the map using the textures 
        // this loop will render the map based on the map array
        for (int row = 0; row < MAP_ROWS; ++row) 
        {
          for (int col = 0; col < MAP_COLS; ++col) 
          {
            SDL_Rect srcRect = {0, 0, TILE_WIDTH, TILE_HEIGHT}; // Source rectangle for the texture
            SDL_Rect destRect = {col * TILE_WIDTH, row * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}; // Destination rectangle on screen

            // Render the texture based on the map value
            SDL_RenderCopy(*renderer, gameTextures[map[row][col]], &srcRect, &destRect);    
          }
        }

        // handle user input and acceptable time window for input 
        Uint32 currentTime = SDL_GetTicks();

        // Check if enough time has passed since the last move
        if (currentTime - (*lastMoveTime) >= MOVEMENT_DELAY) 
        {
          // Handle keyboard input
          const Uint8 *state = SDL_GetKeyboardState(NULL);
          int moved = 0;

          // track our new coordinates
          int newX = (*player).x;
          int newY = (*player).y;

          // track our grid position
          int gridX = (*player).x / TILE_WIDTH;
          int gridY = (*player).y / TILE_HEIGHT;

          // track whether we need to switch maps or not
          bool switchMap = false;

          // animation setup
          Uint32 currentTime = SDL_GetTicks();

          // determine which direction we are moving
          if (state[SDL_SCANCODE_W]) 
          {
            // update animation variables
            (*player).direction = UP;

            // case we are moving up
            newY -= TILE_HEIGHT; 
            moved = 1;

            // determine if we are at an exit point
            switch (map[gridY][gridX])
            {
              case 12:
                // setup changing map
                switchMap = true;
                *chooseMap = 3;

                // setup starting coordinates
                newX = (*player).x - X_OFFSET;
                newY = MAP_ROWS * TILE_HEIGHT;
                moved = 0;

                // setup music
                musicSelector = 3;
                break;
              default:
                break;
            }
          }
          else if (state[SDL_SCANCODE_A]) 
          {
            // update animation variables
            (*player).direction = LEFT;

            // case we are moving left
            newX -= TILE_WIDTH;
            moved = 1;

            // determine if we are at an exit point
            switch (map[gridY][gridX])
            {
              case 3:
                // setup changing map
                switchMap = true;
                *chooseMap = 1;

                // setup starting coordinates
                newX = MAP_COLS * TILE_WIDTH;
                newY = (*player).y;
                moved = 0;

                // setup music
                musicSelector = 1;
                break;
              default:
                break;
            }
          }
          else if (state[SDL_SCANCODE_S]) 
          {
            // update animation variables
            (*player).direction = DOWN;

            // case we are moving down
            newY += TILE_HEIGHT;
            moved = 1;

            // determine if we are at an exit point
            switch (map[gridY][gridX])
            {
              case 13:
                // setup changing map
                switchMap = true;
                *chooseMap = 1;

                // setup starting coordinates
                newX = (*player).x - X_OFFSET;
                newY = -TILE_HEIGHT;
                moved = 0;

                // setup music
                musicSelector = 1;
                break;
              default:
                break;
            }
          }
          else if (state[SDL_SCANCODE_D]) 
          {
            // update animation variables
            (*player).direction = RIGHT;

            // case we are moving right
            newX += TILE_WIDTH; 
            moved = 1;

            // determine if we are at an exit point
            switch (map[gridY][gridX])
            {
              case 2:
                // setup changing map
                switchMap = true;
                *chooseMap = 2;

                // setup starting coordinates
                newX = -TILE_WIDTH;
                newY = (*player).y;
                moved = 0;

                // setup music
                musicSelector = 2;
                break;
            }
          }

          // Reset to idle state if no movement keys are pressed
          if (!(state[SDL_SCANCODE_W] || state[SDL_SCANCODE_A] || state[SDL_SCANCODE_S] || state[SDL_SCANCODE_D]))
          {
              currentFrame = 0; // reset animation frame for idle
              switch((*player).direction)
              {
                case UP:
                  (*player).direction = IDLE_UP;
                  break;
                case LEFT:
                  (*player).direction = IDLE_LEFT;
                  break;
                case DOWN:
                  (*player).direction = IDLE_DOWN;
                  break;
                case RIGHT:
                  (*player).direction = IDLE_RIGHT;
                  break;
                default:
                  break;
              }
          }

          // determine which map we are on, and change the currentMapName variable appropriately
          switch(*chooseMap)
          {
            case 1:
              *currentMapName = "perllert_town_map";
              break;
            case 2:
              *currentMapName = "pkrmrn_ctr_map";
              break;
            case 3:
              *currentMapName = "village_ruins_map";
              break;
            default:
              break;
          } 

          // determine if we need to switch maps
          if(switchMap)
          {
            switch(*chooseMap)
            {
              case 1:
                loadMap(map, PERLLERT_TOWN);
                break;
              case 2:
                loadMap(map, PKRMN_CTR);
                break;
              case 3:
                loadMap(map, VILLAGE_RUINS);
                break;
              default:
                break;
            }

            // apply our changed coordinates to the new map
            (*player).x = newX + X_OFFSET;
            (*player).y = newY;  

            // reset the switch map variable
            switchMap = false;
          }
          // otherwise, we are just moving around the map
          else if (newX >= 0 && 
              // we do not subtract TILE_WIDTH because we already account for x position
              newX <= (MAP_COLS * TILE_WIDTH) && //- TILE_WIDTH + TILE_WIDTH &&
              newY >= 0 && 
              newY <= ((MAP_ROWS * TILE_HEIGHT) - TILE_HEIGHT))
          {
            // determine the grid position of the new coordinates
            int newGridX = newX / TILE_WIDTH;
            int newGridY = newY / TILE_HEIGHT;

            // determine if the new position is a wall or not
            switch(map[newGridY][newGridX])
            {
              case 0:
              case 2:
              case 3:
              case 4:
              case 5:
              case 6:
              case 7:
              case 12:
              case 13:
                // ensure the character is within map bounds
                (*player).x = newX;
                (*player).y = newY;
                break;
              default: // default is that the texture is a wall
                break;
            }
          }

          // determine if we have moved or not
          if (moved != 0)
          {
            // Update the last move time if we have moved for the delay
            *lastMoveTime = currentTime;
          }
        }

        // Finalize changes to frame 
        if ((*player).direction != IDLE_RIGHT && (*player).direction != IDLE_LEFT && 
            (*player).direction != IDLE_UP && (*player).direction != IDLE_DOWN) 
        {
          // Update the frame if the character is not idle
          if (currentTime - lastAnimationFrame >= ANIMATION_DELAY) 
          {
            currentFrame = (currentFrame + 1) % SPRITE_FRAMES;
            lastAnimationFrame = currentTime;
          }
        } 
        else 
        {
          // Reset to the first frame when idle
          currentFrame = 0;
        }
        
        SDL_Rect srcRect;
        calculateSrcRect(&srcRect, (*player).direction, currentFrame);

        // Render the sprite
        SDL_Rect destRect = {(*player).x - X_OFFSET, // for whatever reason, the sprite has an off by 8 issue, so I just fix it here
                             (*player).y, 
                             TILE_WIDTH, 
                             TILE_HEIGHT};
        SDL_RenderCopy(*renderer, (*player).sprite, &srcRect, &destRect);
        break;
    }

}

/**
 * This thread function will run the game
 * 
 * @return void
 */
void* game () 
{
  // Initialize SDL
  initSDL();

  // declare the window and renderer
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Event event;
  
  // set up the window and renderer
  setupWindow(&window, &renderer);

  int isRunning = 1; // control variable for the main loop
  Player mainCharacter = {(X_RESOLUTION - TILE_WIDTH) / 2, // default x position
                          (Y_RESOLUTION - TILE_HEIGHT) / 2, // default y position
                          IDLE_DOWN, // default direction
                          IMG_LoadTexture(renderer, "assets/textures/characters/mc.png")}; // default texture

  // Initialize the framerate, load the textures, and set up the maps
  Uint32 frameStart; // Time at the start of the frame
  Uint32 lastMoveTime = 0; // Time of the last movement

  // Load the menu textures
  SDL_Texture *menuTextures[MAX_GAME_TEXTURES];
  int menuTextureCount = loadMenuTextures(menuTextures, &renderer);
  
  // Load ALL the scene textures
  SDL_Texture *gameTextures[MAX_GAME_TEXTURES];
  int textureCount = loadTextures(gameTextures, &renderer);

  // set up the map variable and its naming convention
  int map[MAP_ROWS][MAP_COLS];
  char* currentMapName = "perllert_town_map";

  // set up the load error variable for save handling
  bool loadError = false;

  // Copy the map from the array to the map variable, initialize settings
  loadMap(map, PERLLERT_TOWN);
  musicSelector = 1; // start with perllert town music
  int chooseMap = 1; // determine which map to load, start with perllert town map

  
  // PURELY FOR TRACKING ACTUAL FPS
  int frameCount = 0;
  float fps = 0;
  Uint32 startTicks = SDL_GetTicks();
  

  // setup the game loop and main logic 
  while (isRunning) 
  {
    // initialize the loop, determine which screen to render 
    frameStart = SDL_GetTicks();
    
    // handle events
    // this will handle the user input and determine which screen (game or menu) to render
    HandleEvents(&isRunning, &currentGameState, &currentMenuState, &mainCharacter, 
                 &loadError, &event, currentMapName, map, &chooseMap);
        
    // Clear the renderer
    SDL_RenderClear(renderer);

    
    // render the scene
    render(&renderer, &currentGameState, &currentMenuState, &mainCharacter, 
           &loadError, &currentMapName, map, menuTextures, gameTextures, &lastMoveTime, &chooseMap);

    // present the renderer
    SDL_RenderPresent(renderer);

    
    // PURELY FOR TRACKING ACTUAL FPS
    frameCount++;
    if (SDL_GetTicks() - startTicks >= 1000) { // Every second
        fps = frameCount / ((SDL_GetTicks() - startTicks) / 1000.0f);
        frameCount = 0;
        startTicks = SDL_GetTicks();

        // Display or use the FPS value
        printf("FPS: %.2f\n", fps);
    }
    


    // Framerate control
    int frameTime = SDL_GetTicks() - frameStart;
    if (FRAME_DELAY > frameTime) 
    {
      SDL_Delay(FRAME_DELAY - frameTime);
    }
  }

  // Cleanup 
  SDL_DestroyTexture(mainCharacter.sprite);
  destroyTextures(menuTextures, menuTextureCount);
  destroyTextures(gameTextures, textureCount);

  SDL_DestroyRenderer(renderer); 
  SDL_DestroyWindow(window);
  
  // set the music selector to -1 to signal the music thread to close
  musicSelector = -1;
  return NULL;
}

/**
 * This thread function will run the music
 * 
 * @return void
 */
void* music() 
{
  // Initialize SDL
  SDL_Init(SDL_INIT_AUDIO);

  // Initialize SDL_mixer
  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

  // Load music tracks
  Mix_Music *perllert_town_music = Mix_LoadMUS("assets/audio/perllert_town_music.wav");
  Mix_Music *perkemern_center = Mix_LoadMUS("assets/audio/perkemern_center.wav");
  Mix_Music *village_ruins_music = Mix_LoadMUS("assets/audio/village_ruins_music.wav");
  
  bool running = true; // control variable for the main loop

  // set up a loop to keep the music playing
  while (running) 
  {
    // check for events or conditions that might change musicSelector
    static int currentPlaying = 0; // Keep track of what is currently playing

    // see if there's been a change in music selection
    if (currentPlaying != musicSelector) 
    {
      // stop current music
      Mix_HaltMusic();

      // delay briefly to prevent the music from starting too fast
      SDL_Delay(20);

      // Play new music based on musicSelector
      switch (musicSelector) 
      {
        // in -1 case, we get the signal that game thread has closed
        // now we would want to close the music thread, so we set running to false to break loop
        case -1:
          // end the looping to close the thread
          running = false;
          break;
        case 0:
          break;
        // perllert town music
        case 1:
          Mix_PlayMusic(perllert_town_music, -1);
          break;
        // perkemern center music
        case 2:
          Mix_PlayMusic(perkemern_center, -1);
          break;
        // village ruins music
        case 3:
          Mix_PlayMusic(village_ruins_music, -1);
          break;
      }

      // update the currentPlaying variable to match the musicSelector
      currentPlaying = musicSelector;
    }

    SDL_Delay(100); // Delay to prevent this loop from running too fast
  }

  // Cleanup
  Mix_FreeMusic(perllert_town_music);
  Mix_FreeMusic(perkemern_center);
  Mix_FreeMusic(village_ruins_music);
  Mix_CloseAudio();

  return NULL;
}

/**
 * This is the main function that will run the game by creating two threads
 * 
 * @return 0
 */
int main () //(int argc, char* argv[])
{
  // create two threads to run in parallel
  pthread_t threads[2];

  // game thread for handling the game and user input
  int ret = pthread_create(&threads[0], NULL, game, NULL);
  // music thread for handling the music
  int ret1 = pthread_create(&threads[1], NULL, music, NULL);

  // error check for failed thread launch
  if (ret != 0 || ret1 != 0) 
  {
    fprintf(stderr, "pthread_create failed: %s\n", strerror(ret));
    // Handle error (e.g., try again, abort, etc.)
  }

  // join the threads to prevent the program from closing before the threads are done
  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);
  
  // SDL_Quit is called here to prevent a forced shutdown of the other thread
  // that could potentially cause concurrency issues if we quit before thread closing
  IMG_Quit(); 
  SDL_Quit();

  return 0;
}
