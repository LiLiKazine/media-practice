//
//  sdl_render.c
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/17.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#include "sdl_render.h"

#define REFRESH_EVENT   (SDL_USEREVENT + 1)
#define QUIT_EVENT  (SDL_USEREVENT + 2)

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

static int thread_exit = 0;

int refresh_video_timer(void *udata) {
    thread_exit = 0;
    while (!thread_exit) {
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(40);
    }
    thread_exit = 0;
    SDL_Event event;
    event.type = QUIT_EVENT;
    SDL_PushEvent(&event);
    return 0;
}

void render_video(const char* dst) {
    FILE *video_fd = NULL;
    
    SDL_Event event;
    SDL_Rect rect;
    
    Uint32 pixformat = 0;
    
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    
    SDL_Thread *timer_thread = NULL;
    
    int w_width = 640, w_height = 480;
    const int video_width = 640, video_height = 480;
    
    Uint8 *video_pos = NULL;
//    Uint8 *video_end = NULL;
    
//    unsigned int remain_len = 0;
    size_t video_buff_len = 0;
//    size_t blank_space_len = 0;
    Uint8 *video_buf = NULL;
    
    const unsigned int yuv_frame_len = video_width * video_height * 12 / 8;
    unsigned int temp_yuv_frame_Len = yuv_frame_len;
    
    if (yuv_frame_len & 0xF) {
        temp_yuv_frame_Len = (yuv_frame_len & 0xFFF0) + 0x10;
    }
    
    if (SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Could not initialize SDL - %s\n", SDL_GetError());
        return;
    }
    
    window = SDL_CreateWindow("YUV Player",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              w_width, w_height,
                              SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Failed to create window - %s\n", SDL_GetError());
       goto __FAIL;
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    
    pixformat = SDL_PIXELFORMAT_IYUV;
    
    texture = SDL_CreateTexture(renderer,
                                pixformat,
                                SDL_TEXTUREACCESS_STREAMING,
                                video_width,
                                video_height);
    video_buf = (Uint8*)malloc(temp_yuv_frame_Len);
    if (!video_buf) {
        SDL_Log("Failed to alloc yuv frame space.\n");
        goto __FAIL;
    }
    
    video_fd = fopen(dst, "r");
    if (!video_fd) {
        SDL_Log("Failed to open yuv file.\n");
        goto __FAIL;
    }
    
    if ((video_buff_len = fread(video_buf, 1, yuv_frame_len, video_fd)) <= 0) {
        SDL_Log("Failed to read data from yuv file!\n");
        goto __FAIL;
    }
    
    video_pos = video_buf;
    
    timer_thread = SDL_CreateThread(refresh_video_timer,
                                    NULL,
                                    NULL);
    
    do {
        SDL_WaitEvent(&event);
        if (event.type == REFRESH_EVENT) {
            SDL_UpdateTexture(texture, NULL, video_pos, video_width);
            
            //FIX: If window is resize
            rect.x = 0;
            rect.y = 0;
            rect.w = w_width;
            rect.h = w_height;
            
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_RenderPresent(renderer);
            
            if ((video_buff_len = fread(video_buf, 1, yuv_frame_len, video_fd)) <= 0) {
                thread_exit = 1;
                continue;
            }
        } else if (event.type == SDL_WINDOWEVENT) {
            //If Resize
            SDL_GetWindowSize(window, &w_width, &w_height);
        } else if (event.type == SDL_QUIT) {
            thread_exit = 1;
        } else if (event.type == QUIT_EVENT) {
            break;
        }
    } while (1);
    
__FAIL:
    if (video_buf) {
        free(video_buf);
    }
    
    if (video_fd) {
        fclose(video_fd);
    }
    
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}
