#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<time.h>
#include<stdlib.h>
#include<stdio.h>

#define SCREEN_W 400
#define SCREEN_H 400
#define TITLE "Hall Puzzle"

#define UP_MASK 1
#define RIGHT_MASK 2
#define DOWN_MASK 4
#define LEFT_MASK 8
#define FULL_MASK 15
#define UP_DIR 0
#define RIGHT_DIR 1
#define DOWN_DIR 2
#define LEFT_DIR 3
#define UNDEFINED_DIR 4

#define CROSS_POSS_N 1
#define CROSS_POSS_D 3

typedef struct {
    char *field;
    int width, height;
    SDL_Renderer *render;
    SDL_Texture *texture;
} Map;


int _random(int n){
    return rand() % n;
}

int poss(int n, int d){
    return _random(d) < n;
}

int choose_mask(char inv){
    inv = ~inv & FULL_MASK;
    int n = _random(__builtin_popcount(inv)) + 1;
    // printf("\t%i %i\n", n, inv);
    char cnt = 1;
    if (cnt & inv){
        n--;
    }
    while (n){
        cnt <<= 1;
        if (cnt & inv){
            n--;
        }
    }
    return cnt;
}

void get_rand_pos(int *x, int *y, int *places, int *rand_places){
    int n = _random(*places);
    *x = rand_places[2 * n];
    *y = rand_places[2 * n + 1];
    (*places)--;
    rand_places[2 * n] = rand_places[2 * (*places)];
    rand_places[2 * n + 1] = rand_places[2 * (*places) + 1];
}

void swap(Map *map, int num1, int num2){
    int temp = (map -> field)[num1];
    (map -> field)[num1] = (map -> field)[num2];
    (map -> field)[num2] = temp;
}

void init_map(Map *map, int width, int height, SDL_Renderer *render){
    (map -> field) = malloc(width * height);
    (map -> width) = width;
    (map -> height) = height;
    int i, x, y;
    int left = width * height;
    int places = width * height;
    int rand_places[places * 2];
    char been[width * height];
    char mask, occ;
    for (i=0;i < width * height;i++){
        (map -> field)[i] = 0;
        been[i] = 0;
        rand_places[2 * i] = i % width;
        rand_places[2 * i + 1] = i / width;
    }
    x = _random(width);
    y = _random(height);
    while (left){
        if (!places){
            for (i=0;i < width * height;i++){
                rand_places[2 * i] = i % width;
                rand_places[2 * i + 1] = i / width;
            }
            places = width * height;
        }
        occ = (!y || been[(y - 1) * width + x]) |
              ((x == width - 1 || been[y * width + x + 1]) << 1) |
              ((y == height - 1 || been[(y + 1) * width + 1]) << 2) |
              ((!x || been[y * width + x - 1]) << 3);
        if (occ == FULL_MASK){
            if (poss(CROSS_POSS_N, CROSS_POSS_D)){
                occ = (map -> field)[y * width + x] | (!y) |
                                           ((x == width - 1) << 1) |
                                           ((y == height - 1) << 2) |
                                           ((!x) << 3);
            } else {
                get_rand_pos(&x, &y, &places, rand_places);
                continue;
            }
            if (occ == FULL_MASK){
                get_rand_pos(&x, &y, &places, rand_places);
                continue;
            }
        }
        if (left < width * height && !((map -> field)[y * width + x])){
            get_rand_pos(&x, &y, &places, rand_places);
            continue;
        }
        if (!(been[y * width + x])){
            left--;
            been[y * width + x] = 1;
        }
        mask = choose_mask(occ);
        (map -> field)[y * width + x] |= mask;
        if (mask == UP_MASK){
            y--;
            (map -> field)[y * width + x] |= DOWN_MASK;
        }
        if (mask == RIGHT_MASK){
            x++;
            (map -> field)[y * width + x] |= LEFT_MASK;
        }
        if (mask == DOWN_MASK){
            y++;
            (map -> field)[y * width + x] |= UP_MASK;
        }
        if (mask == LEFT_MASK){
            x--;
            (map -> field)[y * width + x] |= RIGHT_MASK;
        }
        if (!(been[y * width + x])){
            left--;
            been[y * width + x] = 1;
        }
    } 
    for (i = 0;i < width * height;i++){
        swap(map, _random(width * height), _random(width * height));
    } 
    map -> render = render;
    map -> texture = SDL_CreateTextureFromSurface(render,
                                                  IMG_Load("halls.png"));
}


void blit_map(Map *map){
    SDL_Renderer *render = map -> render;
    SDL_Texture *texture = map -> texture;
    SDL_Rect src, dest;
    int width = (map -> width), height = (map -> height), block_w, block_h;
    src.w = 20;
    src.h = 20;
    src.y = 0;
    block_w = SCREEN_W / width;
    block_h = SCREEN_H / height;
    dest.w = block_w;
    dest.h = block_h;
    int x, y;
    for (y = 0;y < height;y++){
        dest.x = 0;
        dest.y = y * block_h;
        for (x = 0;x < width;x++){
            src.x = (map -> field)[y * width + x] * 20;
            dest.x = x * block_w;
            SDL_RenderCopy(render, texture, &src, &dest);
        }
    }
}


int check_pos(Map *map, int x, int y, char dir, char *been){
    int width = (map -> width), height = (map -> height);
    been[y * width + x] = 1;
    char tile = (map -> field)[y * width + x];
    if (dir != UP_DIR && tile & UP_MASK){
        if (!y || !((map -> field)[(y - 1) * width + x] & DOWN_MASK)){
            return 1;  //hall leads to nowhere
        }
        if (!(been[(y - 1) * width + x]) &&
            check_pos(map, x, y - 1, DOWN_DIR, been)){
            return 1;
        }
    }
    if (dir != RIGHT_DIR && tile & RIGHT_MASK){
        if (x == width - 1 || !((map -> field)[y * width + x + 1] & LEFT_MASK)){
            return 1;  //hall leads to nowhere
        }
        if (!(been[y * width + x + 1]) &&
            check_pos(map, x + 1, y, LEFT_DIR, been)){
            return 1;
        }
    }
    if (dir != DOWN_DIR && tile & DOWN_MASK){
        if (y == height - 1 || !((map -> field)[(y + 1) * width + x] & UP_MASK)){
            return 1;  //hall leads to nowhere
        }
        if (!(been[(y + 1) * width + x]) &&
            check_pos(map, x, y + 1, UP_DIR, been)){
            return 1;
        }
    }
    if (dir != LEFT_DIR && tile & LEFT_MASK){
        if (!x || !((map -> field)[y * width + x - 1] & RIGHT_MASK)){
            return 1;  //hall leads to nowhere
        }
        if (!(been[y * width + x - 1]) &&
            check_pos(map, x - 1, y, RIGHT_DIR, been)){
            return 1;
        }
    }
 
    return 0;
}


int check_map(Map *map){
    int width = (map -> width), height = (map -> height);
    char been[width * height];
    int i;
    for (i=0;i < width * height;i++){
        been[i] = 0;
    }
    if (check_pos(map, 0, 0, UNDEFINED_DIR, been)){
        return 0;
    }
    for (i=0;i < width * height;i++){
        if (!(been[i])){
            return 0;
        }
    }
    return 1;
}


int init(SDL_Window **window, SDL_Surface **screen){
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    *window = SDL_CreateWindow(TITLE, 0, 0, SCREEN_W, SCREEN_H,
                               SDL_WINDOW_SHOWN);
    if (window == NULL){
        printf("SDL_Error: %s\n", SDL_GetError());
        return 1;
    } 

    *screen = SDL_GetWindowSurface(*window);
    IMG_Init(IMG_INIT_PNG);
    srand(time(NULL));

    return 0;
}


int loop(SDL_Window *window, SDL_Surface *screen){
    int width, height, block_w, block_h;
    SDL_Renderer *render = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *fronts[4];
    fronts[0] = SDL_CreateTextureFromSurface(render, IMG_Load("easy.png"));
    fronts[1] = SDL_CreateTextureFromSurface(render, IMG_Load("medium.png"));
    fronts[2] = SDL_CreateTextureFromSurface(render, IMG_Load("hard.png"));
    fronts[3] = SDL_CreateTextureFromSurface(render, IMG_Load("extreme.png"));
    SDL_Texture *end = SDL_CreateTextureFromSurface(render, IMG_Load("end.png"));
    SDL_SetRenderDrawBlendMode(render, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(render, 20, 50, 50, 255);
    SDL_Rect m_rect;
    Map map;
    int run = 1, marked = 0, mark_num, state = 0, difficulty = 0;
    SDL_Event e;
    while (run){
        while (SDL_PollEvent(&e)){
            if (e.type == SDL_QUIT){
                run = 0;
            } else if (e.type == SDL_MOUSEBUTTONDOWN){
                if (state){
                    if (e.button.button == SDL_BUTTON_RIGHT){
                        marked = 0;
                    } else if (marked){
                        swap(&map, mark_num, e.button.y / block_h * width +
                                             e.button.x / block_w);
                        marked = 0;
                    } else {
                        mark_num = e.button.y / block_h * width + e.button.x / block_w;
                        m_rect.x = e.button.x - e.button.x % block_w;
                        m_rect.y = e.button.y - e.button.y % block_h;
                        marked = 1;
                    }
                    if (check_map(&map)){
                        state = 2;
                    }
                } else {
                    if (e.button.x < 200){
                        switch (difficulty){
                            case 0:
                                width = 4;
                                height = 4;
                                break;
                            case 1:
                                width = 5;
                                height = 5;
                                break;
                            case 2:
                                width = 10;
                                height = 10;
                                break;
                            case 3:
                                width = 20;
                                height = 20;
                                break;
                        }
                        block_w = SCREEN_W / width;
                        block_h = SCREEN_H / height;
                        init_map(&map, width, height, render);
                        m_rect.w = block_w;
                        m_rect.h = block_h;
                        state = 1;
                    } else {
                        difficulty++;
                        difficulty %= 4;
                    }
                }
            }
        }
        SDL_RenderClear(render);
        if (state == 2){
            SDL_RenderCopy(render, end, NULL, NULL);
        } else if (state){
            blit_map(&map);
            if (marked){
                SDL_SetRenderDrawColor(render, 0, 0, 255, 70);
                SDL_RenderFillRect(render, &m_rect);
                SDL_SetRenderDrawColor(render, 20, 50, 50, 255);
            }
        } else {
            SDL_RenderCopy(render, fronts[difficulty], NULL, NULL);
        }

        SDL_RenderPresent(render);
    }
}


int main(int argc, char* args[]){
    SDL_Window *window = NULL;
    SDL_Surface *screen = NULL; 

    if (init(&window, &screen)){
        printf("Initialization failed!");
        return 1;
    }

    loop(window, screen);

    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
