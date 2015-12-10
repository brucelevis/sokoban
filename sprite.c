#include "sprite.h"
#include "main.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

extern u32 tile_w;
extern u32 tile_h;

static const char* sprite_filenames[NUM_SPRITES] = 
{
#define SPRITE(name,a,b) #name,
    SPRITE_LIST
#undef SPRITE
};
static const u32 sprite_center[NUM_SPRITES*2] = 
{
#define SPRITE(name,center_x,center_y) center_x, center_y,
    SPRITE_LIST
#undef SPRITE
};
static u32 sprite_size[NUM_SPRITES*2] = {};
static u8 *sprite_pixels[NUM_SPRITES] = {};
static SDL_Surface *sprite_surf[NUM_SPRITES] = {};
static SDL_Texture *sprite_texture[NUM_SPRITES] = {};

static const u32 rmask = 0x000000ff;
static const u32 gmask = 0x0000ff00;
static const u32 bmask = 0x00ff0000;
static const u32 amask = 0xff000000;


void load_sprites()
{
    char sprite_filename_buffer[128];
    for(int i=0;i<NUM_SPRITES;i++){
        sprintf(sprite_filename_buffer,"data/%s.png",sprite_filenames[i]);
        FILE *f = fopen(sprite_filename_buffer,"rb");
        if(f){
            s32 w,h,c;
            sprite_pixels[i] = stbi_load_from_file(f,&w,&h,&c,4);
            sprite_surf[i] = SDL_CreateRGBSurfaceFrom(sprite_pixels[i],w,h,32,
                    w*4,rmask,gmask,bmask,amask);
            sprite_texture[i] = SDL_CreateTextureFromSurface(renderer,
                    sprite_surf[i]);
            sprite_size[i*2] = w;
            sprite_size[i*2+1] = h;
            fclose(f);
        }
    }
}

void draw_sprite(u32 s, float x, float y, Camera camera)
{
    SDL_Rect dest_rect = {(x-sprite_center[s*2]-(sprite_size[s*2]-tile_w)*.5f)*camera.scale+camera.offset_x
        ,(y-sprite_center[s*2+1]-sprite_size[s*2+1]+tile_h)*camera.scale+camera.offset_y
        ,sprite_size[s*2]*camera.scale ,sprite_size[s*2+1]*camera.scale};
    SDL_RenderCopy(renderer,sprite_texture[s],0,&dest_rect);
}

void draw_sprite_clipped(u32 s, float x, float y, Camera camera)
{
    SDL_Rect dest_rect = {x*camera.scale+camera.offset_x
        ,y*camera.scale+camera.offset_y
        ,tile_w*camera.scale ,tile_h*camera.scale};
    SDL_Rect src_rect = {0,0,tile_w,tile_h};
    SDL_RenderCopy(renderer,sprite_texture[s],&src_rect,&dest_rect);
}

void draw_sprite_at_tile(u32 s, float x, float y, Camera camera)
{
    draw_sprite(s,x*tile_w,y*tile_h, camera);
}

