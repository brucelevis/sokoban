#pragma once
#include "main.h"

#define SPRITE_LIST \
SPRITE(hero,        0, 0) \
SPRITE(tile,        0, 0) \
SPRITE(high_tile,  0, 0) \
SPRITE(crate,  0, 0) \

typedef struct 
{
    float scale;
    float offset_x;
    float offset_y;
}Camera;

enum
{
#define SPRITE(name,a,b) SPRITE_##name,
    SPRITE_LIST
#undef SPRITE
    NUM_SPRITES
};

void load_sprites();
void draw_sprite(u32 s, float x, float y, Camera camera);
void draw_sprite_clipped(u32 s, float x, float y, Camera camera);
void draw_sprite_at_tile(u32 s, float x, float y, Camera camera);
