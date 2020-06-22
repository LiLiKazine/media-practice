//
//  pcm_player.c
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/22.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#include "pcm_player.h"

#define BLOCK_SIZE 4096000

static size_t buffer_len = 0;
static Uint8* audio_buf = NULL;
static Uint8* audio_pos = NULL;


void read_audio_data(void *userdata, Uint8 *stream, int len) {
    if (buffer_len == 0) {
        return;
    }
    SDL_memset(stream, 0, len);
    len = (len < buffer_len) ? len : buffer_len;
    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    
    audio_pos += len;
    buffer_len -= len;
    printf("++++ %u, %u, %zu\n", *audio_pos, *audio_buf, buffer_len);
}

void play_pcm(const char *src) {
    
    int ret = 0;
    FILE* audio_fd = NULL;
    SDL_AudioSpec spec;
    
    ret = SDL_Init(SDL_INIT_AUDIO);
    if (ret) {
        SDL_Log("Failed to initial!\n");
        return;
    }
    
    audio_fd = fopen(src, "r");
    if (!audio_fd) {
        SDL_Log("Cannot open audio file: %s.\n", src);
        goto __FAIL;
    }
    audio_buf = (Uint8*)malloc(BLOCK_SIZE);
    if (!audio_buf) {
        SDL_Log("Failed to alloc memory.\n");
        goto __FAIL;
    }
    
    spec.freq = 44100;
    spec.channels = 2;
    spec.format = AUDIO_S16SYS;
    spec.callback = read_audio_data;
    spec.userdata = NULL;
    
    ret = SDL_OpenAudio(&spec, NULL);
    if (ret) {
        SDL_Log("Failed to open audio device.\n");
        goto __FAIL;
    }

    SDL_PauseAudio(0);
    
    do {
        buffer_len = fread(audio_buf, 1, BLOCK_SIZE, audio_fd);
        audio_pos = audio_buf;
        while (audio_pos < (audio_pos + buffer_len)) {
            SDL_Delay(1);
        }
    } while (buffer_len != 0);
    
    SDL_CloseAudio();
    
__FAIL:

    if (audio_buf) {
        free(audio_buf);
    }
    if (audio_fd) {
        fclose(audio_fd);
    }
    SDL_Quit();
}
