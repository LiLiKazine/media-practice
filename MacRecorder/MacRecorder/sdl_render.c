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
    SDL_Texture *texture = NULL;
    
    SDL_Rect rect;
    rect.w = 30;
    rect.h = 30;
    
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
    
    texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640, 480);
    if (!texture) {
        SDL_Log("Failed to Create Texture.\n");
        goto __exit;
    }
    do {
        SDL_WaitEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
                quit = 0;
                break;
            default:
                SDL_Log("event type is: %d", event.type);
        }
                
        rect.x = rand() % 640;
        rect.y = rand() % 480;
        SDL_SetRenderTarget(render, texture);
        SDL_SetRenderDrawColor(render, 0, 0, 0, 0);
        SDL_RenderClear(render);
        SDL_RenderDrawRect(render, &rect);
        SDL_SetRenderDrawColor(render, 255, 0, 0, 0);
        SDL_RenderFillRect(render, &rect);
        
        SDL_SetRenderTarget(render, NULL);
        SDL_RenderCopy(render, texture, NULL, NULL);
        SDL_RenderPresent(render);
    } while (quit);
    
__exit:
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    if (render) {
        SDL_DestroyRenderer(render);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}
