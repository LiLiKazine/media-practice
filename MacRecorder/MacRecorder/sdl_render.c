//
//  sdl_render.c
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/17.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#include "sdl_render.h"

void create_renderer() {
    
    int quit = 1;
    
    SDL_Window *window = NULL;
    SDL_Renderer *render = NULL;
    SDL_Event event;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    
    window = SDL_CreateWindow("SDL2 Window", 200, 200, 640, 480, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("Failed to Create Window.\n");
        goto __exit;
    }
    
    render = SDL_CreateRenderer(window, -1, 0);
    if (!render) {
        SDL_Log("Failed to Create Render.\n");
        goto __exit;
    }
    
    SDL_SetRenderDrawColor(render, 128, 128, 128, 255);
    SDL_RenderClear(render);
    SDL_RenderPresent(render);
    
    do {
        SDL_WaitEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
                quit = 0;
                break;
            default:
                SDL_Log("event type is: %d", event.type);
        }
    } while (quit);
    
__exit:
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}
