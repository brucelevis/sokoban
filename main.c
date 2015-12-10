#include "main.h"
#include <stdlib.h>
#include <stdio.h>

#include "editor.h"
#include "menu.h"
#include "game.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "easing.h"

static int GAME_QUIT = 0;
void quit_game()
{
    GAME_QUIT = 1;
}
SDL_Renderer *renderer;

GameState editor_state;
GameState menu_state;
GameState game_state;

static GameState *current_state;

u32 window_w = 1200;
u32 window_h = 800;
u32 tile_w = 168;
u32 tile_h = 124;

float tmp1 = 1.0f;
float tmp2 = 1.0f;

struct Tweener
{
    float  orig;
    float  value;
    u32    duration;
    u32    time;
    u32    type;
    float *subject;
    Tweener *next;
};

void add_tweener(float *subject, float value, u32 duration, u32 type)
{

    u8 found_existing = 0;
    Tweener *current = current_state->tweeners_head;
    Tweener *previous = 0;
    //NOTE(Vidar): replace existing tweener if it points at the same variable
    while(current != 0) {
        if(current->subject == subject){
            found_existing = 1;
            current->orig     = *subject;
            current->value    = value;
            current->duration = duration;
            current->time     = 0;
            current->type     = type;
            break;
        }
        current = current->next;
    }

    if(!found_existing){
        Tweener *e  = malloc(sizeof(Tweener));
        e->orig     = *subject;
        e->value    = value;
        e->duration = duration;
        e->time     = 0;
        e->type     = type;
        e->subject  = subject;
        e->next     = 0;
        if(current_state->tweeners_tail != 0){
            current_state->tweeners_tail->next = e;
        }
        current_state->tweeners_tail = e;
        if(current_state->tweeners_head == 0){
            current_state->tweeners_head = e;
        }
    }
}


float apply_tweening(float t, u32 type){
    switch(type){
        case TWEEN_LINEARINTERPOLATION: return LinearInterpolation(t);
        case TWEEN_QUADRATICEASEIN:     return QuadraticEaseIn(t);
        case TWEEN_QUADRATICEASEOUT:    return QuadraticEaseOut(t);
        case TWEEN_QUADRATICEASEINOUT:  return QuadraticEaseInOut(t);
        case TWEEN_CUBICEASEIN:         return CubicEaseIn(t);
        case TWEEN_CUBICEASEOUT:        return CubicEaseOut(t);
        case TWEEN_CUBICEASEINOUT:      return CubicEaseInOut(t);
        case TWEEN_QUARTICEASEIN:       return QuarticEaseIn(t);
        case TWEEN_QUARTICEASEOUT:      return QuarticEaseOut(t);
        case TWEEN_QUARTICEASEINOUT:    return QuarticEaseInOut(t);
        case TWEEN_QUINTICEASEIN:       return QuinticEaseIn(t);
        case TWEEN_QUINTICEASEOUT:      return QuinticEaseOut(t);
        case TWEEN_QUINTICEASEINOUT:    return QuinticEaseInOut(t);
        case TWEEN_SINEEASEIN:          return SineEaseIn(t);
        case TWEEN_SINEEASEOUT:         return SineEaseOut(t);
        case TWEEN_SINEEASEINOUT:       return SineEaseInOut(t);
        case TWEEN_CIRCULAREASEIN:      return CircularEaseIn(t);
        case TWEEN_CIRCULAREASEOUT:     return CircularEaseOut(t);
        case TWEEN_CIRCULAREASEINOUT:   return CircularEaseInOut(t);
        case TWEEN_EXPONENTIALEASEIN:   return ExponentialEaseIn(t);
        case TWEEN_EXPONENTIALEASEOUT:  return ExponentialEaseOut(t);
        case TWEEN_EXPONENTIALEASEINOUT:return ExponentialEaseInOut(t);
        case TWEEN_ELASTICEASEIN:       return ElasticEaseIn(t);
        case TWEEN_ELASTICEASEOUT:      return ElasticEaseOut(t);
        case TWEEN_ELASTICEASEINOUT:    return ElasticEaseInOut(t);
        case TWEEN_BACKEASEIN:          return BackEaseIn(t);
        case TWEEN_BACKEASEOUT:         return BackEaseOut(t);
        case TWEEN_BACKEASEINOUT:       return BackEaseInOut(t);
        case TWEEN_BOUNCEEASEIN:        return BounceEaseIn(t);
        case TWEEN_BOUNCEEASEOUT:       return BounceEaseOut(t);
        case TWEEN_BOUNCEEASEINOUT:     return BounceEaseInOut(t);
        default:                        assert(0); return 0.f;
    }
}

void updateTweeners(u32 delta_ticks)
{
    Tweener *current = current_state->tweeners_head;
    Tweener *previous = 0;
    while(current != 0) {
        u8 free_current = 0;
        current->time += delta_ticks;
        if(current->time > current->duration){
            if(previous != 0){
                previous->next = current->next;
            }else{
                current_state->tweeners_head = current->next;
            }
            if(current_state->tweeners_tail == current){
                current_state->tweeners_tail = previous;
            }
            current->time = current->duration;
            free_current = 1;
        }
        float t = (float)current->time/(float)current->duration;
        float x = apply_tweening(t,current->type);
        *current->subject = (1.f-x)*current->orig
            + x*(current->value);
        if(free_current){
            Tweener *tmp = current;
            current = current->next;
            free(tmp);
        }else{
            previous = current;
            current = current->next;
        }
    }
}

void screen2pixels(float x, float y, u32 *x_out, u32 *y_out)
{
    *x_out = x*window_w;
    *y_out = y*window_h;
}

void pixels2screen(u32 x, u32 y, float *x_out, float *y_out)
{
    *x_out = (float)x/(float)window_w;
    *y_out = (float)y/(float)window_h;
}

TTF_Font *hud_font = NULL;
TTF_Font *menu_font = NULL;

void draw_text(TTF_Font *font,SDL_Color font_color, const char *message,s32 x,
        s32 y, float center_x, float center_y, float scale){
    SDL_Surface *surf = TTF_RenderText_Blended(font, message, font_color);
	if (surf != 0){
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surf);
        if (texture != 0){
            int iw, ih;
            SDL_QueryTexture(texture, NULL, NULL, &iw, &ih);
            float w = (float)iw*scale;
            float h = (float)ih*scale;
            SDL_Rect dest_rect = {x-center_x*w,y-center_y*h,w,h};
            SDL_RenderCopy(renderer,texture,NULL,&dest_rect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surf);
	}

}


static u32 last_tick = 0;
static float last_dt = 0;
static const u32 num_prev_ticks = 16;
static u32 current_prev_ticks = 0;
static u32 prev_ticks[num_prev_ticks] = {};

static void draw_fps()
{
    float dt = 0.f;
    for(int i=0;i<num_prev_ticks;i++){
        dt += prev_ticks[i]/1000.f;
    }
    dt *= 1.f/(float)num_prev_ticks;
    char fps_buffer[32] = {};
    SDL_Color font_color = {0,0,0,255};
    sprintf(fps_buffer,"fps: %1.2f",1.f/dt);
    u32 x,y;
    screen2pixels(1.f,0.f,&x,&y);
    draw_text(hud_font,font_color,fps_buffer,x,y,1.f,0.f,1.f);
}

static void main_callback(void * vdata)
{
    u32 current_tick = SDL_GetTicks();
    u32 delta_ticks = current_tick - last_tick;
    float dt = (float)delta_ticks/1000.f;
    last_tick = current_tick;
    last_dt = dt;
    current_prev_ticks = (current_prev_ticks + 1) % num_prev_ticks;
    prev_ticks[current_prev_ticks] = delta_ticks;

    // Input
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type){
            case SDL_QUIT: {
                quit_game();
            }break;
            case SDL_WINDOWEVENT: {
                switch (event.window.event){
                    case SDL_WINDOWEVENT_RESIZED:
                        window_w = event.window.data1;
                        window_h = event.window.data2;
                    break;
                    default:break;
                }
            }break;
            case SDL_KEYDOWN: {
                switch(event.key.keysym.sym){
                    case SDLK_1:
                        current_state = &menu_state;
                        break;
                    case SDLK_2:
                        current_state = &editor_state;
                        break;
                    case SDLK_3:
                        current_state = &game_state;
                        break;
                    case SDLK_a:
                        add_tweener(&tmp1,1.f,1800,TWEEN_EXPONENTIALEASEOUT);
                        break;
                    case SDLK_s:
                        add_tweener(&tmp1,8.f,800,TWEEN_BOUNCEEASEOUT);
                        break;
                }
            }break;
        }
        current_state->input(current_state->data,event);
    }
    // Update
    updateTweeners(delta_ticks);
    current_state->update(current_state->data,dt);
    // Render
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    current_state->draw(current_state->data);
    draw_fps();
    SDL_RenderPresent(renderer);
}

int main(int argc, char** argv) {
    printf("tile_w: %d\n",tile_w);
    printf("tile_h: %d\n",tile_h);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Ludum Dare 34",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_w, window_h,
        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if(window == 0){
    }
    
    renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(renderer == 0){
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    if(TTF_Init() != 0){
        printf("Could not init font\n");
    }
    hud_font  = TTF_OpenFont("data/font.ttf", 24);
    menu_font = TTF_OpenFont("data/font.ttf", 64);

    editor_state = create_editor_state(window_w,window_h);
    menu_state   = create_menu_state();
    game_state   = create_game_state();
    current_state   = &game_state;

    add_tweener(&tmp1,5.f,1800,TWEEN_BACKEASEOUT);
    add_tweener(&tmp2,3.f,1600,TWEEN_CUBICEASEOUT);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(main_callback, 0, 0, 1);
#else
    while (!GAME_QUIT) {
        main_callback(0);
    }
#endif
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
