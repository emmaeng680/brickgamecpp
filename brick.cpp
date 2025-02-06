#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
// added headers

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <SDL2/SDL_mixer.h>

Mix_Chunk* hitBrick = NULL;
Mix_Chunk* hitPaddle = NULL;
Mix_Chunk* gameOver = NULL;
Mix_Chunk* winSound = NULL; 

enum GameState { PLAYING, GAME_OVER, WIN };
GameState gameState = PLAYING;



#define WIDTH 620
#define HEIGHT 720
#define SPEED 9
#define FONT_SIZE 32
#define BALL_SPEED 8
#define SIZE 16
#define COL 7
#define ROW 5
#define PI 3.14
#define SPACING 16

SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font* font;
bool running;
int frameCount, timerFPS, lastFrame, fps;
SDL_Color color;
SDL_Rect paddle, ball, lives, brick;
float velY, velX;
int liveCount;
bool bricks[ROW*COL];

int score = 0;  // ✅ Initialize score to 0


void resetBricks() {
    for(int i=0; i<COL*ROW; i++) bricks[i]=1;
    liveCount=3;
    paddle.x=(WIDTH/2)-(paddle.w/2);
    ball.y=paddle.y-(paddle.h*4);
    velY=BALL_SPEED/2;
    velX=0;
    ball.x=WIDTH/2-(SIZE/2);

    // ✅ Reset text color to white when restarting
    color.r = 255;
    color.g = 255;
    color.b = 255;
    score = 0;
}

void setBricks(int i) {
 brick.x=(((i%COL)+1)*SPACING)+((i%COL)*brick.w)-(SPACING/2);
 brick.y=brick.h*3+(((i%ROW)+1)*SPACING)+((i%ROW)*brick.h)-(SPACING/2);
}

void write(std::string text, int x, int y) {
    SDL_Surface *surface;
    SDL_Texture *texture;
    const char* t = text.c_str();
    surface = TTF_RenderText_Solid(font, t, color);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    lives.w = surface->w;
    lives.h = surface->h;
    
    // Adjust text centering
    lives.x = x - (lives.w / 2) + 100;  // Shift slightly to the right
    lives.y = y - (lives.h / 2);
    
    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &lives);
    SDL_DestroyTexture(texture);
}



void gameOverScreen();
void winScreen();



void update() {
    if (gameState != PLAYING) return;  // Stop updates if game is over or won

    if (liveCount <= 0) {
        Mix_PlayChannel(-1, gameOver, 0);
        gameState = GAME_OVER;
        return;
    }

    if (SDL_HasIntersection(&ball, &paddle)) {
        Mix_PlayChannel(-1, hitPaddle, 0);
        double rel = (paddle.x + (paddle.w / 2)) - (ball.x + (SIZE / 2));
        double norm = rel / (paddle.w / 2);
        double bounce = norm * (5 * PI / 12);
        velY = -BALL_SPEED * cos(bounce);
        velX = BALL_SPEED * -sin(bounce);
    }

    if (ball.y <= 0) velY = -velY;
    if (ball.y + SIZE >= HEIGHT) {
        liveCount--;
        if (liveCount <= 0) {
            Mix_PlayChannel(-1, gameOver, 0);
            gameState = GAME_OVER;
            return;
        } else {
            ball.x = WIDTH / 2 - SIZE / 2;
            ball.y = paddle.y - paddle.h * 4;
            velX = 0;
            velY = -BALL_SPEED / 2;
        }
    }

    if (ball.x <= 0 || ball.x + SIZE >= WIDTH) velX = -velX;
    ball.x += velX;
    ball.y += velY;

    bool allBricksGone = true;
    for (int i = 0; i < COL * ROW; i++) {
        setBricks(i);
        if (SDL_HasIntersection(&ball, &brick) && bricks[i]) {
            bricks[i] = 0;
            score += 1;
            Mix_PlayChannel(-1, hitBrick, 0);

            if (ball.x >= brick.x) { velX = -velX; ball.x -= 20; }
            if (ball.x <= brick.x) { velX = -velX; ball.x += 20; }
            if (ball.y <= brick.y) { velY = -velY; ball.y -= 20; }
            if (ball.y >= brick.y) { velY = -velY; ball.y += 20; }
        }
        if (bricks[i]) allBricksGone = false;
    }

    if (allBricksGone) {
        Mix_PlayChannel(-1, winSound, 0);
        gameState = WIN;
    }
}



void input() {
    SDL_Event e;
    const Uint8 *keystates = SDL_GetKeyboardState(NULL);

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;

        // Restart game on "R" key press
        if (gameState == GAME_OVER || gameState == WIN) {
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r) {
                resetBricks();
                gameState = PLAYING;
            }
            // Restart game on touch (tap) input
            if (e.type == SDL_FINGERDOWN) {
                resetBricks();
                gameState = PLAYING;
            }
        }

        // Handle touch events for paddle movement
        if (gameState == PLAYING) {
            if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
                float touchX = e.tfinger.x * WIDTH;  // Convert normalized touch position
                paddle.x = touchX - paddle.w / 2;   // Center paddle to touch position
            }
        }
    }

    // Check keyboard input for movement (outside SDL_PollEvent loop)
    if (gameState == PLAYING) {
        if (keystates[SDL_SCANCODE_LEFT]) {
            paddle.x -= SPEED;
        }
        if (keystates[SDL_SCANCODE_RIGHT]) {
            paddle.x += SPEED;
        }
    }

    // Keep paddle within screen bounds
    if (paddle.x < 0) paddle.x = 0;
    if (paddle.x + paddle.w > WIDTH) paddle.x = WIDTH - paddle.w;
}




void render() {
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 255);
    SDL_RenderClear(renderer);

    if (gameState == GAME_OVER) {
        color.r = 255; color.g = 0; color.b = 0;
        write("GAME OVER", WIDTH / 2 - 40, HEIGHT / 2);
        write("Press R to Restart", WIDTH / 2 - 105, HEIGHT / 2 + 50);
    } 
    else if (gameState == WIN) {
        color.r = 0; color.g = 255; color.b = 0;
        write("YOU WIN!", WIDTH / 2, HEIGHT / 2);
        write("Press R to Restart", WIDTH / 2 - 70, HEIGHT / 2 + 50);
    }
    else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &paddle);
        SDL_RenderFillRect(renderer, &ball);
        color.r = 255; color.g = 255; color.b = 255;
        write("Score: " + std::to_string(score), FONT_SIZE + 20, FONT_SIZE * 1.5);
        write(std::to_string(liveCount), WIDTH / 2 - FONT_SIZE / 5, FONT_SIZE * 1.5);

        for (int i = 0; i < COL * ROW; i++) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            if (i % 2 == 0) SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            if (bricks[i]) {
                setBricks(i);
                SDL_RenderFillRect(renderer, &brick);
            }
        }
    }
    SDL_RenderPresent(renderer);
}


void gameOverScreen() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    color.r = 255; color.g = 0; color.b = 0;

    // Get text width & height for centering
    SDL_Surface* surface = TTF_RenderText_Solid(font, "GAME OVER", color);
    int textW = surface->w;
    int textH = surface->h;
    SDL_FreeSurface(surface);

    surface = TTF_RenderText_Solid(font, "Press R to Restart", color);
    int textW2 = surface->w;
    int textH2 = surface->h;
    SDL_FreeSurface(surface);

    // Centered positions
    int centerX = WIDTH / 2 - textW / 5;
    int centerY = HEIGHT / 2 - textH / 2;
    int centerX2 = WIDTH / 2 - textW2 / 6;
    int centerY2 = HEIGHT / 2 + 50 - textH2 / 2;

    // Display centered text
    write("GAME OVER", centerX, centerY);
    write("Press R to Restart", centerX2, centerY2);

    SDL_RenderPresent(renderer);

    SDL_Event e;
    bool waiting = true;
    while (waiting) {
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) exit(0);
            if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r) {
                resetBricks(); // Restart game
                waiting = false; // Exit loop and return to main loop
            }
        }
    }
    color.r = 255;
color.g = 255;
color.b = 255;
}


void winScreen() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    color.r = 0; color.g = 255; color.b = 0;  // Green text for Win Screen

    write("YOU WIN!", WIDTH / 2 - 100, HEIGHT / 2);
    write("Press R to Restart", WIDTH / 2 - 250, HEIGHT / 2 + 50);

    SDL_RenderPresent(renderer);

    SDL_Event e;
    bool waiting = true;
    while (waiting) {
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) exit(0);
            if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r) {
                resetBricks(); // Restart game
                waiting = false; // Exit loop and return to main loop
            }
        }
    }

    // ✅ Reset text color after win screen
    color.r = 255;
    color.g = 255;
    color.b = 255;
}




void unlockAudio() {
    Mix_AllocateChannels(16);
    Mix_PlayChannel(-1, hitBrick, 0);
    Mix_HaltChannel(-1);
}

void main_loop() {
    input();
    update();
    render();
}

int main() {
    // if (SDL_Init(SDL_INIT_EVERYTHING) < 0) std::cout << "Failed at SDL_Init()" << std::endl;
    if (SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer) < 0) 
        std::cout << "Failed at SDL_CreateWindowAndRenderer()" << std::endl;
    
    SDL_SetWindowTitle(window, "Brick Breaker");
    TTF_Init();
    font = TTF_OpenFont("font.ttf", FONT_SIZE);

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cout << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
    }

    hitBrick = Mix_LoadWAV("./brick_hit.wav");
    hitPaddle = Mix_LoadWAV("./paddle_hit.wav");
    gameOver = Mix_LoadWAV("./game_over.wav");
    winSound = Mix_LoadWAV("./win_sound.wav");

    if (hitBrick == NULL || hitPaddle == NULL || gameOver == NULL || winSound == NULL) {
        std::cout << "Failed to load sound effects! SDL_mixer Error: " << Mix_GetError() << std::endl;
    }

    unlockAudio();  // ✅ Ensures mobile browsers allow sound playback

    running = true;
    static int lastTime = 0;
    color.r = color.g = color.b = 255;
    paddle.h = 12; 
    paddle.w = WIDTH / 4;
    paddle.y = HEIGHT - paddle.h - 32;
    ball.w = ball.h = SIZE;
    brick.w = (WIDTH - (SPACING * COL)) / COL;
    brick.h = 22;

    resetBricks();

    #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(main_loop, 0, 1);
    #else
        while (running) {
            lastFrame = SDL_GetTicks();
            input();
            update();
            render();
        }
    #endif

    Mix_FreeChunk(hitBrick);
    Mix_FreeChunk(hitPaddle);
    Mix_FreeChunk(gameOver);
    Mix_FreeChunk(winSound);
    Mix_CloseAudio();

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TTF_Quit();

    return 0;
}
