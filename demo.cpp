#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#include "LightMask.hpp"

#include <vector>
#include <iostream>

constexpr int WIDTH = 80;
constexpr int HEIGHT = 60;
constexpr int ZOOM = 10;
#define OFFSET(x, y) ((WIDTH * 4 * (y)) + (x) * 4)

void generateNoise(std::vector<float>& walls)
{
    int width = WIDTH;
    int height = HEIGHT;

    int num_paths = 20;
    int pathlength = 500;
    int x = width / 2;
    int y = height / 2;

    for (int p = 0; p < num_paths; p++)
    {
        for (int i = 0; i < pathlength; i++)
        {
            int dir = rand()%4;
            switch (dir)
            {
                case 0: x -= 1; break;
                case 1: x += 1; break;
                case 2: y -= 1; break;
                case 3: y += 1; break;
            }
            if (y <= 1 || y >= height-1) y = height/2;
            if (x <= 1 || x >= width-1) x = width/2;

            y = std::min(height-1, std::max(1, y));
            x = std::min(width-1,  std::max(1, x));
            walls[x + y * width] = 0.0;
        }
    }
}

int main()
{
    //
    // Init SDL
    //

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    std::vector<uint8_t> pixels;

    SDL_Init( SDL_INIT_EVERYTHING );
    atexit( SDL_Quit );

    window = SDL_CreateWindow(
        "SDL2",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WIDTH * ZOOM, HEIGHT * ZOOM,
        SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    SDL_RendererInfo info;
    SDL_GetRendererInfo( renderer, &info );

    texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    pixels = std::vector<uint8_t>( WIDTH * HEIGHT * 4, 0 );
    bool quit = false;
    SDL_Event e;
    int mousex = WIDTH/2;
    int mousey = HEIGHT/2;



    //
    // Init Lightmask Variables
    //

    // The lightmask itself
    LightMask lightmask(WIDTH, HEIGHT);
    // Intensity: How far light spreads
    lightmask.setIntensity(40.0f);
    // Ambient: Ambient light
    lightmask.setAmbient(0.4f);

    // Vector representing wall opacities (1.0: solid, 0.0: clear)
    std::vector<float> walls(WIDTH * HEIGHT, 1.0f);
    // Generate cave-like noise using random walk
    generateNoise(walls);

    //
    // Render Loop
    //

    while (!quit)
    {
        // Check for exit event
        while( SDL_PollEvent( &e ) != 0 )
        {
            // User requests quit
            if ( e.type == SDL_QUIT )
            {
                quit = true;
            }
        }

        // Get mouse location
        SDL_GetMouseState(&mousex, &mousey);
        mousex /= ZOOM;
        mousey /= ZOOM;


        // 
        // LightMask
        //

        // Reset the light mask
        lightmask.reset();
        // Add a light at the location of the mouse
        lightmask.addLight(mousex, mousey, 1.0f);
        // Compute the mask
        lightmask.computeMask(walls);


        //
        // Copy lightmask into pixel positions
        //

        for (unsigned int i = 0; i < 4 * WIDTH * HEIGHT; i+=4)
        {
            const int mask_index = i/4;
            const int brightness = (int)(lightmask.mask[mask_index] * 255.0);

            pixels[ i + 0 ] = brightness;
            pixels[ i + 1 ] = brightness;
            pixels[ i + 2 ] = brightness * walls[mask_index]; // So we can see where the walls are
            pixels[ i + 3 ] = brightness;
        }


        //
        // Render
        //

        SDL_SetRenderDrawColor( renderer, 0, 0, 0, SDL_ALPHA_OPAQUE );
        SDL_RenderClear( renderer );

        SDL_UpdateTexture
        (
            texture,
            NULL,
            &pixels[0],
            WIDTH * 4
        );


        SDL_RenderCopy( renderer, texture, NULL, NULL );
        SDL_RenderPresent( renderer );
    }

    // Clean up
    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_Quit();

    return 0;
}