#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define COLOR(n) ((n >> 16) & 0xff), ((n >> 8) & 0xff), ((n)&0xff)
#define GRAY(n) ((n)&0xff), ((n)&0xff), ((n)&0xff)

#define PI 3.1415926536

typedef enum { K_UP, K_DOWN, K_LEFT, K_RIGHT } Keys;
bool key_pressed[32];

typedef enum { S_TOP, S_RIGHT, S_LEFT, S_BOTTOM } Sides;

float slen(float a, float b) { return a * a + b * b; }
float len(float a, float b) { return sqrt(slen(a, b)); }

float maxf(float a, float b) { return (a > b) ? a : b; }
float minf(float a, float b) { return (a > b) ? b : a; }

float lerp(float a, float b, float t) { return a + (b - a) * t; }
float clamp(float a, float b, float v) {
    return minf(maxf(minf(a, b), v), maxf(a, b));
}
float lerpc(float a, float b, float t) { return clamp(a, b, lerp(a, b, t)); }

/* float dot(a_x, a_y, b_x, b_y) { return a_x * b_x + a_y * b_y; } */

float raycast(float ox, float oy, float dx, float dy, char* grid, int gw,
              int gh, char* hit_char, Sides* side, float* hit_x, float* hit_y) {
    int gx = (ox), gy = (oy);

    int sx = (dx >= 0) ? 1 : -1;
    int sy = (dy >= 0) ? 1 : -1;

    float x1 = INFINITY, y1 = INFINITY;

    if (sx >= 0) gx++;
    for (int x = gx; x >= 0 && x < gw; x += sx) {
        float _y = dy / dx * (x - ox) + oy;
        int y = _y;
        if (y < 0 || y >= gh) break;
        if (grid[x - (sx < 0) + y * gw] != '.') {
            x1 = x;
            y1 = _y;
            break;
        }
    }

    float l1 = slen(x1 - ox, y1 - oy);

    float x2 = INFINITY, y2 = INFINITY;

    if (sy >= 0) gy++;
    for (int y = gy; y >= 0 && y < gh; y += sy) {
        float _x = dx / dy * (y - oy) + ox;
        int x = _x;
        if (x < 0 || x >= gw) break;
        if (grid[x + (y - (sy < 0)) * gw] != '.') {
            y2 = y;
            x2 = _x;
            break;
        }
    }

    float l2 = slen(x2 - ox, y2 - oy);

    if (l1 == INFINITY && l2 == INFINITY) {
        return 100;
    }

    if (l1 < l2) {
        *hit_char = grid[(int)(x1 - (sx < 0)) + (int)y1 * gw];
        *side = (sx > 0) ? S_LEFT : S_RIGHT;
        *hit_x = x1;
        *hit_y = y1;
        return sqrt(l1);
    } else {
        *hit_char = grid[(int)x2 + (int)(y2 - (sy < 0)) * gw];
        *side = (sy > 0) ? S_TOP : S_BOTTOM;
        *hit_x = x2;
        *hit_y = y2;
        return sqrt(l2);
    }
}

SDL_Texture* generate_floor_gradient(SDL_Renderer* renderer, int col1,
                                     int col2) {
    int pixels[2] = {col1, col2};
    SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(
        pixels, 1, 2, 24, 4, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
#ifndef __EMSCRIPTEN__
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
#endif

    return tex;
}

bool player_check_collision(float player_x, float player_y, float player_r,
                            char* grid, int gw, int gh) {
    return grid[(int)(player_x + player_r) + (int)(player_y + player_r) * gw] !=
               '.' ||
           grid[(int)(player_x - player_r) + (int)(player_y + player_r) * gw] !=
               '.' ||
           grid[(int)(player_x + player_r) + (int)(player_y - player_r) * gw] !=
               '.' ||
           grid[(int)(player_x - player_r) + (int)(player_y - player_r) * gw] !=
               '.';
}

typedef struct {
    float x, y;
    SDL_Texture* tex;
} Sprite;

// global_variables
int window_height = 800, window_width = 1200;
const int texture_size = 128;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Surface* surf;
bool quit = 0;
SDL_Event event;
int grid_w = 16;
int grid_h = 11;

char grid[] =
    "aaaaaaaaaaaaaaaa"
    "addddddddddd...a"
    "ad....bbbb.....a"
    "ad....bcbb...aaa"
    "adc............a"
    "a..............a"
    "a..bcb.....b...a"
    "a..b.......b...a"
    "a..b...a......ba"
    "a......a.....bba"
    "aaaaaaaaaaaaaaaa";
float player_x = 4, player_y = 4;
float player_a = PI;
float FOV = PI / 2;

Sprite sprites[10];
int sprite_count = 0;

int last_ticks;

SDL_Texture* terracotta_tex;
SDL_Texture* weird_stone_tex;
SDL_Texture* red_stone_tex;
SDL_Texture* magic_ball_tex;
SDL_Texture* cat_tex;
SDL_Texture* leopard_tex;
SDL_Texture* floor_tex;
//

void main_loop() {
    int ticks_now = SDL_GetTicks();
    float dt = (ticks_now - last_ticks) / 1000.f;
    last_ticks = ticks_now;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                quit = 1;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                bool pressed = event.type == SDL_KEYDOWN;
                switch (event.key.keysym.sym) {
                    case SDLK_a:
                    case SDLK_LEFT:
                        key_pressed[K_LEFT] = pressed;
                        break;
                    case SDLK_d:
                    case SDLK_RIGHT:
                        key_pressed[K_RIGHT] = pressed;
                        break;
                    case SDLK_w:
                    case SDLK_UP:
                        key_pressed[K_UP] = pressed;
                        break;
                    case SDLK_s:
                    case SDLK_DOWN:
                        key_pressed[K_DOWN] = pressed;
                        break;
                }
            } break;
        }
    }

    SDL_GetWindowSize(window, &window_width, &window_height);

    SDL_SetRenderDrawColor(renderer, GRAY(80), 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, COLOR(0x584734), 255);
    SDL_Rect lower_screen_half_rect = {.x = 0,
                                       .y = window_height / 2,
                                       .w = window_width,
                                       .h = window_height / 2};
#ifndef __EMSCRIPTEN__
    SDL_RenderCopy(renderer, floor_tex, NULL, &lower_screen_half_rect);
#else
	SDL_RenderFillRect(renderer,&lower_screen_half_rect);
#endif


    // Process controlls

    const float turning_speed = 3;
    const float movement_speed = 4;

    if (key_pressed[K_LEFT]) {
        player_a -= turning_speed * dt;
    }
    if (key_pressed[K_RIGHT]) {
        player_a += turning_speed * dt;
    }
    float forward_x = cos(player_a);
    float forward_y = sin(player_a);
    float right_x = -forward_y;
    float right_y = forward_x;

    if (key_pressed[K_UP]) {
        player_x += forward_x * movement_speed * dt;
        if (player_check_collision(player_x, player_y, 0.1, grid, grid_w,
                                   grid_h))
            player_x -= forward_x * movement_speed * dt;

        player_y += forward_y * movement_speed * dt;
        if (player_check_collision(player_x, player_y, 0.1, grid, grid_w,
                                   grid_h))
            player_y -= forward_y * movement_speed * dt;
    }
    if (key_pressed[K_DOWN]) {
        player_x -= forward_x * movement_speed * dt;
        if (player_check_collision(player_x, player_y, 0.1, grid, grid_w,
                                   grid_h))
            player_x += forward_x * movement_speed * dt;

        player_y -= forward_y * movement_speed * dt;
        if (player_check_collision(player_x, player_y, 0.1, grid, grid_w,
                                   grid_h))
            player_y += forward_y * movement_speed * dt;
    }

    // Do the actual rendering

    int columns = window_width / 3;
    float depth_buf[columns];
    float aspect_ratio = window_width / (float)window_height;
    float column_w = window_width / (float)columns;
    float cx = 0;
    float perp_x = -right_x * aspect_ratio + forward_x  //* cos(FOV / 2)
        ;
    float perp_y = -right_y * aspect_ratio + forward_y  //* cos(FOV / 2)
        ;
    float jump_x = right_x * 2 / columns * aspect_ratio;
    float jump_y = right_y * 2 / columns * aspect_ratio;

    for (int i = 0; i < columns; i++) {
        char hit_char;
        Sides side;
        float hit_x, hit_y;
        float dist = raycast(player_x, player_y, perp_x, perp_y, grid, grid_w,
                             grid_h, &hit_char, &side, &hit_x, &hit_y);
        dist /= len(perp_x, perp_y);
        depth_buf[i] = dist;

        float height = 500 / dist;

        SDL_Texture* tex;

        switch (hit_char) {
            case 'a':
                tex = terracotta_tex;
                break;
            case 'b':
                tex = weird_stone_tex;
                break;
            case 'c':
                tex = red_stone_tex;
                break;
            case 'd':
                tex = leopard_tex;
                break;
        }

        SDL_FRect rect = {.x = cx,
                          .y = (window_height - height) / 2,
                          .w = column_w,
                          .h = height};

        SDL_Rect sample_rect = {.y = 0, .w = 1, .h = texture_size};
        if (side == S_LEFT || side == S_RIGHT) {
            sample_rect.x = texture_size * (hit_y - (int)hit_y);
        } else {
            sample_rect.x = texture_size * (hit_x - (int)hit_x);
        }

        switch (side) {
            case S_TOP:
            case S_RIGHT:
                SDL_SetTextureColorMod(tex,
                                       GRAY((int)lerpc(255, 50, dist / 10)));
                break;
            case S_BOTTOM:
            case S_LEFT:
                SDL_SetTextureColorMod(tex,
                                       GRAY((int)lerpc(200, 50, dist / 10)));
                break;
        }

        SDL_RenderCopyF(renderer, tex, &sample_rect, &rect);

        perp_x += jump_x;
        perp_y += jump_y;
        cx += column_w;
    }
    // render a minimap
    for (int x = 0; x < grid_w; x++) {
        for (int y = 0; y < grid_h; y++) {
            switch (grid[x + y * grid_w]) {
                case '.':
                    SDL_SetRenderDrawColor(renderer, COLOR(0), 255);
                    break;
                case 'a':
                    SDL_SetRenderDrawColor(renderer, COLOR(0xD66621), 255);
                    break;
                case 'b':
                    SDL_SetRenderDrawColor(renderer, COLOR(0xB6727B), 255);
                    break;
                case 'c':
                    SDL_SetRenderDrawColor(renderer, COLOR(0x5B0000), 255);
                    break;
                case 'd':
                    SDL_SetRenderDrawColor(renderer, COLOR(0xC98F72), 255);
                    break;
            }
            SDL_Rect rect = {.x = x * 10, .y = y * 10, .w = 10, .h = 10};
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    for (int i = 0; i < sprite_count; i++) {
        float soff_x = sprites[i].x - player_x;
        float soff_y = sprites[i].y - player_y;

        float dist = len(soff_x, soff_y);

        float rot_y = soff_x * cos(-player_a) - soff_y * sin(-player_a);
        float rot_x = soff_x * sin(-player_a) + soff_y * cos(-player_a);

        float height = 500 / rot_y;
        if (rot_y < 0) continue;
        float center_x = window_width * (1 + rot_x / rot_y / aspect_ratio) / 2;

        int start_x = maxf(0, center_x - height / 2.f) / column_w * column_w;
        int end_x =
            minf(center_x + height / 2.f, window_width) / column_w * column_w;
        int width = end_x - start_x;

        if (start_x > window_width || end_x < 0) continue;

        for (int i = start_x / column_w; start_x < end_x; i++) {
            if (depth_buf[i] > rot_y) break;
            start_x += column_w;
            width -= column_w;
        }
        for (int i = end_x / column_w; start_x < end_x; i--) {
            if (depth_buf[i] > rot_y) break;
            end_x -= column_w;
            width -= column_w;
        }

        SDL_FRect rect = {.x = start_x,
                          .y = (window_height - height) / 2,
                          .w = width,
                          .h = height};

        SDL_Rect sample_rect = {
            .x = (start_x - (center_x - height / 2.f)) / height * texture_size,
            .y = 0,
            .w = (end_x - start_x) / height * texture_size,
            .h = texture_size};
        SDL_RenderCopyF(renderer, sprites[i].tex, &sample_rect, &rect);
    }

    SDL_Rect rect = {
        .x = player_x * 10 - 5, .y = player_y * 10 - 5, .w = 10, .h = 10};
    SDL_SetRenderDrawColor(renderer, COLOR(0x00ffff), 255);
    SDL_RenderDrawRect(renderer, &rect);
    SDL_RenderDrawLineF(renderer, player_x * 10, player_y * 10,
                        (player_x + forward_x) * 10,
                        (player_y + forward_y) * 10);

    SDL_RenderPresent(renderer);
}

int main() {
    printf("[INFO] Hello, Raycasting!\n");

    if (SDL_Init(SDL_INIT_VIDEO)) {
        printf("[ERROR] SDL failed to init: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow(
        "Raycaster", 10, 10, window_width, window_height,
        SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load textures

    surf = SDL_LoadBMP("assets/terracotta.bmp");
    terracotta_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    surf = SDL_LoadBMP("assets/weird_stone.bmp");
    weird_stone_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    surf = SDL_LoadBMP("assets/red_stone.bmp");
    red_stone_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    surf = SDL_LoadBMP("assets/magic_ball.bmp");
    magic_ball_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    surf = SDL_LoadBMP("assets/cat.bmp");
    cat_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    surf = SDL_LoadBMP("assets/leopard.bmp");
    leopard_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    floor_tex = generate_floor_gradient(renderer, 0x31281d, 0x584734);

    Sprite magic_ball_sprite = {
        .x = 2.5f,
        .y = 2.5f,
        .tex = magic_ball_tex,
    };
    sprites[sprite_count++] = magic_ball_sprite;

    magic_ball_sprite.x = 4.5f;
    magic_ball_sprite.y = 7.5f;
    sprites[sprite_count++] = magic_ball_sprite;

    magic_ball_sprite.x = 13.5f;
    magic_ball_sprite.y = 8.5f;
    sprites[sprite_count++] = magic_ball_sprite;

    Sprite cat_sprite = {
        .x = 8.5f,
        .y = 6.5f,
        .tex = cat_tex,
    };
    sprites[sprite_count++] = cat_sprite;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (!quit) {
        main_loop();
    }
#endif

    SDL_DestroyTexture(terracotta_tex);
    SDL_DestroyTexture(weird_stone_tex);
    SDL_DestroyTexture(red_stone_tex);
    SDL_DestroyTexture(magic_ball_tex);
    SDL_DestroyTexture(cat_tex);
    SDL_DestroyTexture(leopard_tex);
    SDL_DestroyTexture(floor_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
