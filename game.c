// libraries being used for this project
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_image.h> // make sure to include the SDL_image library for sprites
#include <SDL2/SDL_mixer.h> // includes the SDL audio mixer
#include <pthread.h>

// macros for commonly used values to make easier readability
#define TILE_WIDTH 16
#define TILE_HEIGHT 16
#define X_OFFSET 8
#define MAP_ROWS 9
#define MAP_COLS 10
#define FPS 60
#define X_RESOLUTION 160
#define Y_RESOLUTION 144
#define MOVEMENT_DELAY 100
#define RES_SCALE 8
#define MENU_ITEM_COUNT 3

// I didn't want to include math.h because I was purely dealing with integers
// instead I decided to use these trivial macros for min and maxing
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// global variable that will allow our threads to sync properly
int musicSelector = 0; // initial music selection

// this will set up for our start menu
typedef enum { MENU, GAME } GameState;
GameState currentGameState = GAME;

// This will set up our menu options
typedef enum { SAVE, LOAD, EXIT } MenuState;
MenuState currentMenuState = SAVE;

void* game () 
{
  /******* PART 1: Initialize the window *******/
  if (SDL_Init(SDL_INIT_VIDEO) < 0) 
  {
    fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return NULL;
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) 
  {
    fprintf(stderr, "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
    SDL_Quit();
    return NULL;
  }

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Event event;
  int isRunning = 1, 
      x = (X_RESOLUTION - TILE_WIDTH) / 2, 
      y = (Y_RESOLUTION - TILE_HEIGHT) / 2;

  
  // resolution is scaled 8x higher than what will be rendered (1280x1152 screen for 160x144 game)
  window = SDL_CreateWindow("Perkemerrrrrrnnnnnnn", 
                            SDL_WINDOWPOS_CENTERED, 
                            SDL_WINDOWPOS_CENTERED, 
                            X_RESOLUTION * RES_SCALE, 
                            Y_RESOLUTION * RES_SCALE, 
                            SDL_WINDOW_SHOWN);
  

  // make sure window runs successfully
  if (!window) 
  {
    fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
    IMG_Quit();
    SDL_Quit();
    return NULL;
  }
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) 
  {
    fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return NULL;
  }

  SDL_RenderSetLogicalSize(renderer, X_RESOLUTION, Y_RESOLUTION); // resolution is 160x144



  /******* Part 2: Initialize the framerate, load the textures, and set up the maps *******/
  const int frameDelay = 1000 / FPS;
  Uint32 frameStart;
  int frameTime;

  Uint32 lastMoveTime = 0; // Time of the last movement

  // Load the menu textures
  SDL_Texture *menuSave = IMG_LoadTexture(renderer, "assets/textures/menu/menu_save.png");
  SDL_Texture *menuLoad = IMG_LoadTexture(renderer, "assets/textures/menu/menu_load.png");
  SDL_Texture *menuExit = IMG_LoadTexture(renderer, "assets/textures/menu/menu_exit.png");

  // store the menu textures onto an array for easier access
  //SDL_Texture *menuTextures[MENU_ITEM_COUNT] = {menuSave, menuLoad, menuExit};

  // Load the sprite image
  SDL_Texture *sprite = IMG_LoadTexture(renderer, "assets/textures/characters/mc.png");
  
  // Load the scene textures
  SDL_Texture *wallTexture = IMG_LoadTexture(renderer, "assets/textures/world/wall_grey.png");
  SDL_Texture *floorTexture = IMG_LoadTexture(renderer, "assets/textures/world/grass_grey.jpg");
  SDL_Texture *perllertRightExitTexture = IMG_LoadTexture(renderer, "assets/textures/world/enter_pkrmrn_ctr.png");
  
  SDL_Texture *pkrmrnLeftExitTexture = IMG_LoadTexture(renderer, "assets/textures/world/enter_perllert_town.png");
  SDL_Texture *ctrTopRight = IMG_LoadTexture(renderer, "assets/textures/world/ctr_tile_top_right.png");
  SDL_Texture *ctrTopLeft = IMG_LoadTexture(renderer, "assets/textures/world/ctr_tile_top_left.png");
  SDL_Texture *ctrBottomRight = IMG_LoadTexture(renderer, "assets/textures/world/ctr_tile_bottom_right.png");
  SDL_Texture *ctrBottomLeft = IMG_LoadTexture(renderer, "assets/textures/world/ctr_tile_bottom_left.png");
  
  SDL_Texture *ctrWall1 = IMG_LoadTexture(renderer, "assets/textures/world/ctr_wall1.png");
  SDL_Texture *ctrWall2 = IMG_LoadTexture(renderer, "assets/textures/world/ctr_wall2.png");
  SDL_Texture *ctrWall3 = IMG_LoadTexture(renderer, "assets/textures/world/ctr_wall3.png");
  SDL_Texture *ctrWall4 = IMG_LoadTexture(renderer, "assets/textures/world/ctr_wall4.png");

  int perllert_town_map[MAP_ROWS][MAP_COLS] = 
  {
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 1}, // 1 represents a wall
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 0 represents a walkable tile
    {1, 0, 0, 0, 0, 1, 1, 0, 0, 1}, 
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 1}, 
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 1}, 
    {1, 0, 1, 0, 0, 0, 1, 0, 0, 1}, 
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2}, // 2 represents an exit point
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2}, 
    {1, 0, 0, 1, 1, 1, 1, 1, 1, 1}, 
  };

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

  int map[MAP_ROWS][MAP_COLS];

  // Copy the map from the array to the map variable
  for (int i = 0; i < MAP_ROWS; ++i)
  {
    for (int j = 0; j < MAP_COLS; ++j)
    {
      map[i][j] = perllert_town_map[i][j];
    }
  }

  musicSelector = 1; // start with perllert town music

  /******* Part 3: Setup the game loop and main logic *******/
  while (isRunning) 
  {
    /***** Part 3a: initialize the loop, determine which screen to render *****/
    frameStart = SDL_GetTicks();
    
    // event handling
    while (SDL_PollEvent(&event)) 
    {
      switch(event.type)
      {
        // handle quit event
        case SDL_QUIT:
          isRunning = 0;
          break;
        case SDL_KEYDOWN:
          switch(event.key.keysym.sym)
          {
            // handle 'enter' input for switching between game and menu
            case SDLK_RETURN:
              switch(currentGameState)
              {
                case MENU:
                  switch (currentMenuState)
                  {
                    case SAVE:
                      printf("we've saved the game\n");
                      // save the game
                      break;
                    case LOAD:
                      printf("we've loaded the game\n");
                      // load the game
                      break;
                    case EXIT:
                      // exit the menu
                      currentGameState = GAME;
                      break;
                    default:
                      break;
                  }
                  break;
                case GAME:
                  currentGameState = MENU;
                  currentMenuState = SAVE;
                  break;
                default:
                  break;
              }
              break;
            // handle 'up' input for switching between menu options
            case SDLK_w:
              if (currentMenuState > 0) 
              {
                --currentMenuState;
              }
              break;
            // handle 'down' input for switching between menu options
            case SDLK_s:
              if (currentMenuState < MENU_ITEM_COUNT - 1) 
              {
                ++currentMenuState;
              }
              break;
          }
          break;
      }

      
    }
    
    // Clear the renderer
    SDL_RenderClear(renderer);

    // Render the scene based on the current state
    switch(currentGameState) 
    {
      case MENU:
        // show the appropriate menu
        switch(currentMenuState)
        {
          case SAVE:
            SDL_RenderCopy(renderer, menuSave, NULL, NULL);
            break;
          case LOAD:
            SDL_RenderCopy(renderer, menuLoad, NULL, NULL);
            break;
          case EXIT:
            SDL_RenderCopy(renderer, menuExit, NULL, NULL);
            break;
          default:
            break;
        }
        break;
      case GAME:
        /***** Part 3b: setup the map using the textures *****/
        // this loop will render the map based on the map array
        for (int row = 0; row < MAP_ROWS; ++row) 
        {
          for (int col = 0; col < MAP_COLS; ++col) 
          {
            SDL_Rect srcRect = {0, 0, TILE_WIDTH, TILE_HEIGHT}; // Source rectangle for the texture
            SDL_Rect destRect = {col * TILE_WIDTH, row * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}; // Destination rectangle on screen

            // Render the texture based on the map value
            switch(map[row][col])
            {
              case 0:
                // Draw floor
                SDL_RenderCopy(renderer, floorTexture, &srcRect, &destRect);
                break;
              case 1:
                // Draw wall
                SDL_RenderCopy(renderer, wallTexture, &srcRect, &destRect);
                break;
              case 2:
                // Draw right perllert town exit
                SDL_RenderCopy(renderer, perllertRightExitTexture, &srcRect, &destRect);
                break;
              case 3:
                // Draw left pkrmrn ctr exit
                SDL_RenderCopy(renderer, pkrmrnLeftExitTexture, &srcRect, &destRect);
                break;
              case 4:
                SDL_RenderCopy(renderer, ctrTopRight, &srcRect, &destRect);
                break;
              case 5:
                SDL_RenderCopy(renderer, ctrTopLeft, &srcRect, &destRect);
                break;
              case 6:
                SDL_RenderCopy(renderer, ctrBottomRight, &srcRect, &destRect);
                break;
              case 7:
                SDL_RenderCopy(renderer, ctrBottomLeft, &srcRect, &destRect);
                break;
              case 8:
                SDL_RenderCopy(renderer, ctrWall1, &srcRect, &destRect);
                break;
              case 9:
                SDL_RenderCopy(renderer, ctrWall2, &srcRect, &destRect);
                break;
              case 10:
                SDL_RenderCopy(renderer, ctrWall3, &srcRect, &destRect);
                break;
              case 11:
                SDL_RenderCopy(renderer, ctrWall4, &srcRect, &destRect);
                break;
              default:
                break;
            }
          }
        }

        /***** Part 3c: handle user input and acceptable time window for input *****/
        Uint32 currentTime = SDL_GetTicks();

        if (currentTime - lastMoveTime >= MOVEMENT_DELAY) 
        {
          // Handle keyboard input
          const Uint8 *state = SDL_GetKeyboardState(NULL);
          int moved = 0;

          int newX = x;
          int newY = y;

          int gridX = x / TILE_WIDTH;
          int gridY = y / TILE_HEIGHT;

          bool switchMap = false;
          int chooseMap = 1;

          if (state[SDL_SCANCODE_W]) 
          {
            newY -= TILE_HEIGHT; 
            moved = 1;
          }
          else if (state[SDL_SCANCODE_A]) 
          {
            newX -= TILE_WIDTH;
            moved = 1;

            // determine if we are at the exit point
            switch (map[gridY][gridX])
            {
              case 3:
                // setup changing map
                switchMap = true;
                chooseMap = 1;

                // setup starting coordinates
                newX = MAP_COLS * TILE_WIDTH;
                newY = y;
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
            newY += TILE_HEIGHT;
            moved = 1;
          }
          else if (state[SDL_SCANCODE_D]) 
          {
            newX += TILE_WIDTH; 
            moved = 1;

            // determine if we are at the exit point
            switch (map[gridY][gridX])
            {
              case 2:
                // setup changing map
                switchMap = true;
                chooseMap = 2;

                // setup starting coordinates
                newX = -TILE_WIDTH;
                newY = y;
                moved = 0;

                // setup music
                musicSelector = 2;
                break;
            }
          }

          // determine if we need to switch maps
          if(switchMap)
          {
            for (int i = 0; i < MAP_ROWS; ++i)
            {
              for (int j = 0; j < MAP_COLS; ++j)
              {
                switch(chooseMap)
                {
                  case 1:
                    map[i][j] = perllert_town_map[i][j];
                    break;
                  case 2:
                    map[i][j] = pkrmrn_ctr_map[i][j];
                    break;
                  default:
                    break;
                }
              }
            }   

            // apply our changed coordinates to the new map
            x = newX + X_OFFSET;
            y = newY;  

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
                // ensure the character is within map bounds
                x = newX;
                y = newY;
                break;
              default: // default is that the texture is a wall
                break;
            }
          }

          // determine if we have moved or not
          if (moved != 0)
          {
            // Update the last move time if we have moved for the delay
            lastMoveTime = currentTime;
          }
        }

        /***** Part 3d: Finalize changes to frame *****/
        // Render the sprite
        SDL_Rect srcRect = {0, 
                            0, 
                            TILE_WIDTH, 
                            TILE_HEIGHT}; // Assuming the sprite is 64x64 pixels and we want spawn at the center
        SDL_Rect destRect = {x - X_OFFSET, // for whatever reason, the sprite has an off by 8 issue, so I just fix it here
                            y, 
                            TILE_WIDTH, 
                            TILE_HEIGHT};
        SDL_RenderCopy(renderer, sprite, &srcRect, &destRect);
        break;
    }

    // present the renderer
    SDL_RenderPresent(renderer);

    // Framerate control
    frameTime = SDL_GetTicks() - frameStart;
    if (frameDelay > frameTime) 
    {
      SDL_Delay(frameDelay - frameTime);
    }
  }

  /******* Part 4: Cleanup *******/
  SDL_DestroyTexture(sprite);
  SDL_DestroyTexture(wallTexture);
  SDL_DestroyTexture(floorTexture);
  SDL_DestroyTexture(perllertRightExitTexture);
  SDL_DestroyTexture(pkrmrnLeftExitTexture);

  SDL_DestroyTexture(ctrTopRight);
  SDL_DestroyTexture(ctrTopLeft);
  SDL_DestroyTexture(ctrBottomRight);
  SDL_DestroyTexture(ctrBottomLeft);

  SDL_DestroyTexture(ctrWall1);
  SDL_DestroyTexture(ctrWall2);
  SDL_DestroyTexture(ctrWall3);
  SDL_DestroyTexture(ctrWall4);

  SDL_DestroyRenderer(renderer); 
  SDL_DestroyWindow(window);
  IMG_Quit(); 
  
  musicSelector = -1;
  return NULL;
}

void* music()
{
  // Initialize SDL
  SDL_Init(SDL_INIT_AUDIO);

  // Initialize SDL_mixer
  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

  // Load music tracks
  Mix_Music *music1 = Mix_LoadMUS("assets/audio/perllert_town_music.mp3");
  Mix_Music *music2 = Mix_LoadMUS("assets/audio/perkemern_center.mp3");
  

  bool running = true; // Control variable for the main loop

  while (running) 
  {
    // Check for events or conditions that might change musicSelector
    // Example: musicSelector = getMusicSelectionFromEvent();

    static int currentPlaying = 0; // Keep track of what is currently playing

    if (currentPlaying != musicSelector) 
    {
      // Stop current music
      Mix_HaltMusic();

      SDL_Delay(250);

      // Play new music based on musicSelector
      switch (musicSelector) 
      {
        // in -1 case, we get the signal that game thread has closed
        // now we would want to close the music thread 
        case -1:
          // end the looping to close the thread
          running = false;
          break;
        case 0:
          break;
        // perllert town music case
        case 1:
          Mix_PlayMusic(music1, -1);
          break;
        // perkemern center case
        case 2:
          Mix_PlayMusic(music2, -1);
          break;
      }

      currentPlaying = musicSelector;
    }

    SDL_Delay(100); // Delay to prevent this loop from running too fast
  }

  // Cleanup
  Mix_FreeMusic(music1);
  Mix_FreeMusic(music2);
  Mix_CloseAudio();

  return NULL;
}

int main () //(int argc, char* argv[])
{
  pthread_t threads[2];

  pthread_create(&threads[0], NULL, game, NULL);
  pthread_create(&threads[1], NULL, music, NULL);

  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);
  
  // SDL_Quit is called here to prevent a forced shutdown of the other thread
  // That would cause concurrency issues
  SDL_Quit();

  return 0;
}
