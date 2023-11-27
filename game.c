// libraries being used for this project
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_image.h> // Make sure to include the SDL_image library
#include <SDL2/SDL_mixer.h> // includes the SDL audio mixer
#include <pthread.h>

// macros for commonly used values for readability
#define TILE_WIDTH 16
#define TILE_HEIGHT 16
#define MAP_ROWS 9
#define MAP_COLS 10
#define FPS 60
#define X_RESOLUTION 160
#define Y_RESOLUTION 144
#define MOVEMENT_DELAY 100
#define RES_SCALE 8

// I didn't want to include math.h because I was purely dealing with integers
// instead I decided to use these trivial macros for min and maxing
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// global variable that will allow our threads to sync properly
int musicSelector = 1; // initial music selection

void* game () 
{
  // PART 1: Initialize the window
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

  SDL_Init(SDL_INIT_EVERYTHING);
  IMG_Init(IMG_INIT_PNG); // Initialize SDL_image

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


  
  
  // Part 2: Initialize the framerate, textures, and map
  
  const int frameDelay = 1000 / FPS;
  Uint32 frameStart;
  int frameTime;

  Uint32 lastMoveTime = 0; // Time of the last movement

  // Load the sprite image
  SDL_Texture *sprite = IMG_LoadTexture(renderer, "assets/textures/characters/mc.png");
  
  // Load the scene textures
  SDL_Texture *wallTexture = IMG_LoadTexture(renderer, "assets/textures/world/wall_grey.png");
  SDL_Texture *floorTexture = IMG_LoadTexture(renderer, "assets/textures/world/grass_grey.jpg");
  SDL_Texture *exitTexture = IMG_LoadTexture(renderer, "assets/textures/world/enter_pkrmrn_ctr.png");

  int perllert_town_map[MAP_ROWS][MAP_COLS] = 
  {
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 1}, // 1 represents a wall
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 0 represents a walkable tile
    {1, 0, 0, 0, 0, 1, 1, 0, 0, 1}, 
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 1}, 
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 1}, 
    {1, 0, 1, 0, 0, 0, 1, 0, 0, 1}, 
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2}, 
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2}, 
    {1, 0, 0, 1, 1, 1, 1, 1, 1, 1}, 
  };

  int map[MAP_ROWS][MAP_COLS];

  for (int i = 0; i < MAP_ROWS; ++i)
  {
    for (int j = 0; j < MAP_COLS; ++j)
    {
      map[i][j] = perllert_town_map[i][j];
    }
  }




  // Part 3: Setup the game loop and main logic

  while (isRunning) 
  {
    // Part 3a: initialize the loop
    frameStart = SDL_GetTicks();
    
    // event handling
    while (SDL_PollEvent(&event)) 
    {
      if (event.type == SDL_QUIT) 
      {
        isRunning = 0;
      }
    }
    
    // Clear the renderer
    SDL_RenderClear(renderer);

    // Part 3b: setup the map using the textures

    for (int row = 0; row < MAP_ROWS; ++row) 
    {
      for (int col = 0; col < MAP_COLS; ++col) 
      {
        SDL_Rect srcRect = {0, 0, TILE_WIDTH, TILE_HEIGHT}; // Source rectangle for the texture
        SDL_Rect destRect = {col * TILE_WIDTH, row * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT}; // Destination rectangle on screen

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
            // Draw exits
            SDL_RenderCopy(renderer, exitTexture, &srcRect, &destRect);
            break;
          default:
            break;
        }
      }
    }


    // Part 3c: handle user input and acceptable time window for input

    Uint32 currentTime = SDL_GetTicks();

    if (currentTime - lastMoveTime >= MOVEMENT_DELAY) 
    {
      // Handle keyboard input
      const Uint8 *state = SDL_GetKeyboardState(NULL);
      int moved = 0;

      int newX = x;
      int newY = y;

      if (state[SDL_SCANCODE_W]) 
      {
        newY -= TILE_HEIGHT; 
        moved = 1;
      }
      else if (state[SDL_SCANCODE_A]) 
      {
        newX -= TILE_WIDTH;
        moved = 1;
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
      }
      

      if (newX >= 0 && 
          // we do not subtract TILE_WIDTH because we already account for x position
          newX <= (MAP_COLS * TILE_WIDTH) && //- TILE_WIDTH + TILE_WIDTH &&
          newY >= 0 && 
          newY <= ((MAP_ROWS * TILE_HEIGHT) - TILE_HEIGHT))
      {
        int gridX = newX / TILE_WIDTH;
        int gridY = newY / TILE_HEIGHT;

        switch(map[gridY][gridX])
        {
          case 0:
          case 2:
            // ensure the character is within map bounds;
            x = newX;
            y = newY;
            break;
          case 1:
          default:
            break;
        }
      }

      if (moved != 0)
      {
        lastMoveTime = currentTime;
      }
    
    }
    
    

    // Part 3d: Finalize changes to frame

    // Render the sprite
    SDL_Rect srcRect = {0, 
                        0, 
                        TILE_WIDTH, 
                        TILE_HEIGHT}; // Assuming the sprite is 64x64 pixels and we want spawn at the center
    SDL_Rect destRect = {x - 8, // for whatever reason, the sprite has an off by 8 issue, so I just fix it here
                         y, 
                         TILE_WIDTH, 
                         TILE_HEIGHT};
    SDL_RenderCopy(renderer, sprite, &srcRect, &destRect);

    // Present the renderer
    SDL_RenderPresent(renderer);

    // Frame rate control
    frameTime = SDL_GetTicks() - frameStart;
    if (frameDelay > frameTime) 
    {
      SDL_Delay(frameDelay - frameTime);
    }
  }

  // Part 4: Cleanup

  SDL_DestroyTexture(sprite);
  SDL_DestroyTexture(wallTexture);
  SDL_DestroyTexture(floorTexture);
  SDL_DestroyTexture(exitTexture);
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

      SDL_Delay(1500);

      // Play new music based on musicSelector
      switch (musicSelector) 
      {
        // in -1 case, we get the signal that game thread has closed
        // now we would want to close the music thread 
        case -1:
          // Cleanup
          Mix_FreeMusic(music1);
          Mix_FreeMusic(music2);
          Mix_CloseAudio();
          SDL_Quit();
          return NULL;
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

  // this code never really runs, but is here just in case
  // Cleanup
  Mix_FreeMusic(music1);
  Mix_FreeMusic(music2);
  Mix_CloseAudio();
  SDL_Quit();

  return NULL;
}

int main () //(int argc, char* argv[])
{
  pthread_t threads[2];

  pthread_create(&threads[0], NULL, game, NULL);
  pthread_create(&threads[1], NULL, music, NULL);

  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);

  return 0;
}
