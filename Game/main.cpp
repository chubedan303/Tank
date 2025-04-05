#define SDL_MAIN_HANDLED
#include <bits/stdc++.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <fstream>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 640;
const int TILE_SIZE = 40;
const int TANK_SIZE = 30;
const int MOVE_DELAY = 40;
const double BULLET_SPEED = 0.4;
const int BULLET_SIZE = 20;
const int MAP_WIDTH = 800;
const int MAP_HEIGHT = 600;

int score = 0;
int enemiestank = 0;
bool running = true;
bool inMenu = true;
bool gameOver = false;
bool twoPlayerMode = false;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

SDL_Texture* tankUpTexture = nullptr;
SDL_Texture* tankDownTexture = nullptr;
SDL_Texture* tankLeftTexture = nullptr;
SDL_Texture* tankRightTexture = nullptr;
SDL_Texture* bulletTexture = nullptr;
SDL_Texture* enemyTankUpTexture = nullptr;
SDL_Texture* enemyTankDownTexture = nullptr;
SDL_Texture* enemyTankRightTexture = nullptr;
SDL_Texture* enemyTankLeftTexture = nullptr;
SDL_Texture* groundTexture = nullptr;
SDL_Texture* wallTexture = nullptr;
SDL_Texture* steelWallTexture = nullptr;
SDL_Texture* heartTexture = nullptr;
SDL_Texture* texture = nullptr;
SDL_Texture* textureEnemyBullet = nullptr;
SDL_Texture* giftTexture = nullptr;
SDL_Texture* backgroundTexture = nullptr;

SDL_Texture* menuTexture = nullptr;
SDL_Rect startButtonRect;
SDL_Rect onePlayerButtonRect;
SDL_Rect twoPlayerButtonRect;
SDL_Texture* onePlayerTextTexture = nullptr;
SDL_Texture* twoPlayerTextTexture = nullptr;
SDL_Rect onePlayerTextRect;
SDL_Rect twoPlayerTextRect;

TTF_Font* menuFont = nullptr;
SDL_Color textColor = { 255, 255, 255 };
SDL_Texture* startTextTexture = nullptr;
SDL_Rect startTextRect;

Mix_Chunk* shootSound = nullptr;

SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image: " << path << " Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

int brickHealth[20][25] = { 0 };
int map[20][25];

void generateRandomMap(int map[20][25]) {
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 25; j++) {
            int randNum = rand() % 10;
            if (randNum < 7) {
                map[i][j] = 0;
            } else if (randNum < 9) {
                map[i][j] = 1;
            } else {
                map[i][j] = 2;
            }
            brickHealth[i][j] = 0;
        }
    }
    map[1][1] = map[0][1] = map[1][0] = map[0][0] = 0;
}

struct Bullet {
    double x, y, dx, dy;
    SDL_Rect rect;
    bool active;
    Bullet(double startX, double startY, double dirX, double dirY) {
        x = startX;
        y = startY;
        dx = dirX;
        dy = dirY;
        rect = { (int)x, (int)y, BULLET_SIZE, BULLET_SIZE };
        active = true;
    }
    SDL_Rect getRect() const {
        return rect;
    }
    void update() {
        if (active) {
            x += dx * BULLET_SPEED;
            y += dy * BULLET_SPEED;
            rect.x = x;
            rect.y = y;
            int mapX = x / TILE_SIZE;
            int mapY = y / TILE_SIZE;
            if (mapX >= 0 && mapX < 25 && mapY >= 0 && mapY < 20) {
                if (map[mapY][mapX] == 1) {
                    brickHealth[mapY][mapX]++;
                    if (brickHealth[mapY][mapX] >= 3) {
                        map[mapY][mapX] = 0;
                    }
                    active = false;
                }
                else if (map[mapY][mapX] == 2) {
                    active = false;
                }
            }
            else {
                active = false;
            }
        }
    }
    void draw() {
        if (active && bulletTexture) {
            SDL_RenderCopy(renderer, bulletTexture, NULL, &rect);
        }
    }
};

bool checkCollisionWithWall(const SDL_Rect& bulletRect) {
    int mapX = bulletRect.x / TILE_SIZE;
    int mapY = bulletRect.y / TILE_SIZE;
    if (mapY >= 0 && mapY < 20 && mapX >= 0 && mapX < 25) {
        if (map[mapY][mapX] == 1 || map[mapY][mapX] == 2) {
            return true;
        }
    }
    return false;
}

std::vector<Bullet> bullets;
std::vector<Bullet> enemyBullets;

struct EnemyTank {
    int x, y;
    int hp;
    int dirX, dirY;
    Uint32 lastShotTime;
    SDL_Rect rect;
    SDL_Texture* directionTexture;
    Uint32 lastMoveTime;
    int moveCounter;
    int moveDirection;
    Uint32 shootInterval;

    EnemyTank(int startX, int startY) {
        x = startX;
        y = startY;
        hp = 1;
        dirX = 0;
        dirY = 1;
        lastShotTime = SDL_GetTicks();
        directionTexture = enemyTankDownTexture;
        rect = { x, y, TANK_SIZE, TANK_SIZE };
        lastMoveTime = SDL_GetTicks();
        moveCounter = 0;
        moveDirection = rand() % 4;
        shootInterval = 2000 + rand() % 2001;
    }
    void updateBullets() {
        for (auto it = enemyBullets.begin(); it != enemyBullets.end();) {
            it->update();
            if (!it->active) {
                it = enemyBullets.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    bool canMove(int dx, int dy) {
        int newX = x + dx * 4;
        int newY = y + dy * 4;

        // Check for out-of-bounds movement
        if (newX < 0 || newX + TANK_SIZE > SCREEN_WIDTH || newY < 0 || newY + TANK_SIZE > SCREEN_HEIGHT) {
            return false;
        }

        int mapXStart = newX / TILE_SIZE;
        int mapYStart = newY / TILE_SIZE;
        int mapXEnd = (newX + TANK_SIZE - 1) / TILE_SIZE;
        int mapYEnd = (newY + TANK_SIZE - 1) / TILE_SIZE;

        for (int i = mapYStart; i <= mapYEnd; ++i) {
            for (int j = mapXStart; j <= mapXEnd; ++j) {
                if (i < 0 || i >= 20 || j < 0 || j >= 25) continue; // Ignore out-of-bounds tiles

                if (map[i][j] != 0) {
                    return false;
                }
            }
        }
        return true;
    }

    void move(int dx, int dy) {
        if (canMove(dx, dy)) {
            x += dx * 4;
            y += dy * 4;
            rect.x = x;
            rect.y = y;
        }
    }

    void moveLikeHuman() {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastMoveTime >= MOVE_DELAY * 2) {
            int directions[4][2] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };

            dirX = directions[moveDirection][0];
            dirY = directions[moveDirection][1];

            if (dirX == 0 && dirY == -1) directionTexture = enemyTankUpTexture;
            else if (dirX == 0 && dirY == 1) directionTexture = enemyTankDownTexture;
            else if (dirX == -1 && dirY == 0) directionTexture = enemyTankLeftTexture;
            else if (dirX == 1 && dirY == 0) directionTexture = enemyTankRightTexture;

            if (canMove(dirX, dirY)) {
                move(dirX, dirY);
                moveCounter++;
            } else {
                moveCounter = 0;
                moveDirection = rand() % 4;
            }

            if (moveCounter > 20) {
                moveCounter = 0;
                moveDirection = rand() % 4;
            }

            lastMoveTime = currentTime;
        }
    }


    void shootIfReady() {
        Uint32 currentTime = SDL_GetTicks();

        if (currentTime - lastShotTime >= shootInterval) {
            enemyBullets.emplace_back(x + TANK_SIZE / 2 - BULLET_SIZE / 2,
                y + TANK_SIZE / 2 - BULLET_SIZE / 2,
                dirX * BULLET_SPEED, dirY * BULLET_SPEED);
            if (shootSound != nullptr) {
                Mix_PlayChannel(-1, shootSound, 0);
            }
            lastShotTime = currentTime;
            shootInterval = 2000 + (rand() % 3) * 1000;
        }
    }
    void draw() {
        if (hp > 0) {
            SDL_RenderCopy(renderer, directionTexture, NULL, &rect);
        }
    }
};

std::vector<EnemyTank> enemies;

struct Tank {
    int x, y;
    SDL_Rect rect;
    SDL_Texture* directionTexture;
    int dirX, dirY;
    int hp = 3;
    const int maxHp = 5;
    int startX, startY;
    int heartXOffset; // Offset for drawing hearts
    void drawHearts() {
        SDL_Rect heartRect = { 10 + heartXOffset, 10, 20, 20 };
        for (int i = 0; i < hp; ++i) {
            heartRect.x = 10 + heartXOffset + i * 25;
            SDL_RenderCopy(renderer, heartTexture, NULL, &heartRect);
        }
    }
    Uint32 lastShotTime = 0;
    const Uint32 shootCooldown = 500;
    Tank(int startX, int startY, int heartOffset = 0) : startX(startX), startY(startY), x(startX), y(startY), heartXOffset(heartOffset) {
        rect = { x, y, TANK_SIZE, TANK_SIZE };
        directionTexture = tankDownTexture;
        dirX = 0;
        dirY = -1;
    }

    void resetPosition() {
        x = startX;
        y = startY;
        rect.x = x;
        rect.y = y;
        dirX = 0;
        dirY = -1;
        directionTexture = tankDownTexture;
    }

    void resetPositionAtSpawn() {
        x = TILE_SIZE; // (1 * TILE_SIZE) for x coordinate
        y = TILE_SIZE; // (1 * TILE_SIZE) for y coordinate
        rect.x = x;
        rect.y = y;
        dirX = 0;
        dirY = -1;
        directionTexture = tankDownTexture;
    }

    bool canMove(int dx, int dy) {
        int newX = x + dx * 4;
        int newY = y + dy * 4;

        // Check for out-of-bounds movement
        if (newX < 0 || newX + TANK_SIZE > SCREEN_WIDTH || newY < 0 || newY + TANK_SIZE > SCREEN_HEIGHT) {
            return false;
        }

        int mapXStart = newX / TILE_SIZE;
        int mapYStart = newY / TILE_SIZE;
        int mapXEnd = (newX + TANK_SIZE - 1) / TILE_SIZE;
        int mapYEnd = (newY + TANK_SIZE - 1) / TILE_SIZE;

        for (int i = mapYStart; i <= mapYEnd; ++i) {
            for (int j = mapXStart; j <= mapXEnd; ++j) {
                if (i < 0 || i >= 20 || j < 0 || j >= 25) continue; // Ignore out-of-bounds tiles

                if (map[i][j] != 0) {
                    return false;
                }
            }
        }

        return true;
    }


    void move(int dx, int dy, SDL_Texture* newTexture) {
        directionTexture = newTexture;
        dirX = dx;
        dirY = dy;
        if (canMove(dx, dy)) {
            x += dx * 4;
            y += dy * 4;
            rect.x = x;
            rect.y = y;
        }
    }
    void shoot() {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastShotTime >= shootCooldown) {
            bullets.emplace_back(x + TANK_SIZE / 2 - BULLET_SIZE / 2,
                y + TANK_SIZE / 2 - BULLET_SIZE / 2,
                dirX, dirY);
            if (shootSound != nullptr) {
                Mix_PlayChannel(-1, shootSound, 0);
            }
            lastShotTime = currentTime;
        }
    }
    void draw() {
        SDL_RenderCopy(renderer, directionTexture, NULL, &rect);
    }
};

Tank player(TILE_SIZE, TILE_SIZE, 0); // Player 1, hearts start at x=10
Tank player2(SCREEN_WIDTH - 4*TILE_SIZE, TILE_SIZE, 200); // Player 2, hearts start at x=200 (adjust as needed)

struct GiftBox {
    int x, y;
    SDL_Rect rect;
    bool active;
    GiftBox(int startX, int startY) : x(startX), y(startY), active(true) {
        rect = { x, y, TILE_SIZE / 4 * 3, TILE_SIZE / 4 * 3 };
    }

    void draw(SDL_Renderer* renderer, SDL_Texture* texture) {
        if (active && texture) {
            SDL_RenderCopy(renderer, texture, NULL, &rect);
        }
    }
};

GiftBox* giftBox = nullptr;
Uint32 giftSpawnTime = 0;
const Uint32 giftSpawnDelay = 10000;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;
    window = SDL_CreateWindow("Tank", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    if (TTF_Init() == -1) {
        std::cerr << "Failed to initialize TTF: " << TTF_GetError() << std::endl;
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError();
        return false;
    }

    shootSound = Mix_LoadWAV("shoot.wav");
    if (shootSound == nullptr) {
        std::cerr << "Failed to load shoot sound: " << Mix_GetError() << std::endl;
        return false;
    }

    IMG_Init(IMG_INIT_PNG);
    bulletTexture = IMG_LoadTexture(renderer, "bulletblue.png");
    heartTexture = IMG_LoadTexture(renderer, "tankheart.png");
    wallTexture = IMG_LoadTexture(renderer, "wall.png");
    steelWallTexture = IMG_LoadTexture(renderer, "steelwall.png");
    enemyTankUpTexture = IMG_LoadTexture(renderer, "tankup.png");
    enemyTankDownTexture = IMG_LoadTexture(renderer, "tankdown.png");
    enemyTankRightTexture = IMG_LoadTexture(renderer, "tankright.png");
    enemyTankLeftTexture = IMG_LoadTexture(renderer, "tankleft.png");
    tankUpTexture = IMG_LoadTexture(renderer, "tankerup.png");
    tankDownTexture = IMG_LoadTexture(renderer, "tankerdown.png");
    tankLeftTexture = IMG_LoadTexture(renderer, "tankerleft.png");
    tankRightTexture = IMG_LoadTexture(renderer, "tankerright.png");
    giftTexture = loadTexture("box.png", renderer);
    backgroundTexture = loadTexture("background.png", renderer);

    menuTexture = loadTexture("menu.png", renderer);

    startButtonRect.x = SCREEN_WIDTH / 2 - 100;
    startButtonRect.y = SCREEN_HEIGHT / 2 - 100;
    startButtonRect.w = 200;
    startButtonRect.h = 60;

    onePlayerButtonRect.x = SCREEN_WIDTH / 2 - 150;
    onePlayerButtonRect.y = SCREEN_HEIGHT / 2 - 50;
    onePlayerButtonRect.w = 300;
    onePlayerButtonRect.h = 60;

    twoPlayerButtonRect.x = SCREEN_WIDTH / 2 - 150;
    twoPlayerButtonRect.y = SCREEN_HEIGHT / 2 + 50;
    twoPlayerButtonRect.w = 300;
    twoPlayerButtonRect.h = 60;

    menuFont = TTF_OpenFont("NothingYouCouldDo.ttf", 36);
    if (!menuFont) {
        std::cerr << "Failed to initialize TTF: " << TTF_GetError() << std::endl;
        return false;
    }

    SDL_Surface* textSurface = TTF_RenderText_Solid(menuFont, "Start Game", textColor);
    if (!textSurface) {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        return false;
    }
    startTextTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    startTextRect.w = textSurface->w;
    startTextRect.h = textSurface->h;
    startTextRect.x = startButtonRect.x + (startButtonRect.w - startTextRect.w) / 2;
    startTextRect.y = startButtonRect.y + (startButtonRect.h - startTextRect.h) / 2;
    SDL_FreeSurface(textSurface);

    SDL_Surface* onePlayerSurface = TTF_RenderText_Solid(menuFont, "1 Player", textColor);
    if (!onePlayerSurface) {
        std::cerr << "Failed to render '1 Player' text: " << TTF_GetError() << std::endl;
        return false;
    }
    onePlayerTextTexture = SDL_CreateTextureFromSurface(renderer, onePlayerSurface);
    onePlayerTextRect.w = onePlayerSurface->w;
    onePlayerTextRect.h = onePlayerSurface->h;
    onePlayerTextRect.x = onePlayerButtonRect.x + (onePlayerButtonRect.w - onePlayerTextRect.w) / 2;
    onePlayerTextRect.y = onePlayerButtonRect.y + (onePlayerButtonRect.h - onePlayerTextRect.h) / 2;
    SDL_FreeSurface(onePlayerSurface);

    SDL_Surface* twoPlayerSurface = TTF_RenderText_Solid(menuFont, "2 Players", textColor);
    if (!twoPlayerSurface) {
        std::cerr << "Failed to render '2 Players' text: " << TTF_GetError() << std::endl;
        return false;
    }
    twoPlayerTextTexture = SDL_CreateTextureFromSurface(renderer, twoPlayerSurface);
    twoPlayerTextRect.w = twoPlayerSurface->w;
    twoPlayerTextRect.h = twoPlayerSurface->h;
    twoPlayerTextRect.x = twoPlayerButtonRect.x + (twoPlayerButtonRect.w - twoPlayerTextRect.w) / 2;
    twoPlayerTextRect.y = twoPlayerButtonRect.y + (twoPlayerButtonRect.h - twoPlayerTextRect.h) / 2;
    SDL_FreeSurface(twoPlayerSurface);

    return tankUpTexture && tankDownTexture && tankLeftTexture && tankRightTexture && menuTexture && startTextTexture && giftTexture && backgroundTexture && shootSound && onePlayerTextTexture && twoPlayerTextTexture;
}

void drawMap(SDL_Renderer* renderer) {
    SDL_Rect tileRect;
    tileRect.w = TILE_SIZE;
    tileRect.h = TILE_SIZE;
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 25; j++) {
            tileRect.x = j * TILE_SIZE;
            tileRect.y = i * TILE_SIZE;
            SDL_Texture* textureToRender = NULL;
            if (map[i][j] == 0) {
                textureToRender = groundTexture;
            }
            else if (map[i][j] == 1) {
                textureToRender = wallTexture;
            }
            else if (map[i][j] == 2) {
                textureToRender = steelWallTexture;
            }
            if (textureToRender) {
                SDL_RenderCopy(renderer, textureToRender, NULL, &tileRect);
            }
        }
    }
}

bool isValidSpawnPosition(int x, int y, const Tank& player,const Tank& player2, const int map[20][25]) {
    SDL_Rect spawnRect = { x, y, TANK_SIZE, TANK_SIZE };
    if (SDL_HasIntersection(&spawnRect, &player.rect)|| SDL_HasIntersection(&spawnRect, &player2.rect)) return false;

    int mapXStart = x / TILE_SIZE;
    int mapYStart = y / TILE_SIZE;
    int mapXEnd = (x + TANK_SIZE - 1) / TILE_SIZE;
    int mapYEnd = (y + TANK_SIZE - 1) / TILE_SIZE;

    for (int i = mapYStart; i <= mapYEnd; ++i) {
        for (int j = mapXStart; j <= mapXEnd; ++j) {
            if (i < 0 || i >= 20 || j < 0 || j >= 25) return false;
            if (map[i][j] != 0) return false;
        }
    }

    return true;
}

bool isValidPlayer2Spawn(int x, int y, const Tank& player, const int map[20][25]) {
    SDL_Rect spawnRect = { x, y, TANK_SIZE, TANK_SIZE };
    if (SDL_HasIntersection(&spawnRect, &player.rect)) return false;

    int mapXStart = x / TILE_SIZE;
    int mapYStart = y / TILE_SIZE;
    int mapXEnd = (x + TANK_SIZE - 1) / TILE_SIZE;
    int mapYEnd = (y + TANK_SIZE - 1) / TILE_SIZE;

    for (int i = mapYStart; i <= mapYEnd; ++i) {
        for (int j = mapXStart; j <= mapXEnd; ++j) {
            if (i < 0 || i >= 20 || j < 0 || j >= 25) return false;
            if (map[i][j] != 0) return false;
        }
    }

    return true;
}


//Check if a given x,y coordinate (pixel) is a valid spawn position for the giftbox.

bool isValidGiftSpawnPosition(int x, int y, const int map[20][25]) {
    int mapX = x / TILE_SIZE;
    int mapY = y / TILE_SIZE;

    // Check if the map tile is within the bounds of the map
    if (mapX < 0 || mapX >= 25 || mapY < 0 || mapY >= 20) {
        return false;
    }

    // Check if the tile at map[mapY][mapX] is equal to 0 (empty ground)
    return map[mapY][mapX] == 0;
}


void spawnNewEnemy(Tank& player, Tank& player2, int map[20][25]) {
    int x, y;
    bool validSpawn = false;
    const int MAX_ATTEMPTS = 1000;

    for (int i = 0; i < MAX_ATTEMPTS; ++i) {
        x = rand() % (SCREEN_WIDTH - TANK_SIZE);
        y = rand() % (SCREEN_HEIGHT - TANK_SIZE);

        // Kiểm tra với cả player1 và player2
        if (isValidSpawnPosition(x, y, player,player2, map)) {
            validSpawn = true;
            break;
        }
    }

    if (validSpawn) {
        enemies.emplace_back(x, y);
        enemiestank++;
    }
    else {
        std::cerr << "Không tìm thấy vị trí spawn hợp lệ sau " << MAX_ATTEMPTS << " lần thử!" << std::endl;
    }
}

void spawnGiftBox() {
    if (giftBox == nullptr) {
        int x, y;
        bool validSpawn = false;
        const int MAX_ATTEMPTS = 1000;

        for (int i = 0; i < MAX_ATTEMPTS; ++i) {
            x = (rand() % 25) * TILE_SIZE; // ensure alignment with tile grid
            y = (rand() % 20) * TILE_SIZE; // ensure alignment with tile grid

            if (isValidGiftSpawnPosition(x, y, map)) {
                validSpawn = true;
                break;
            }
        }

        if (validSpawn) {
            giftBox = new GiftBox(x, y);
            giftBox->active = true;
        }
        else {
            std::cerr << "Không tìm thấy vị trí spawn quà hợp lệ sau " << MAX_ATTEMPTS << " lần thử!" << std::endl;
        }
    }
}


void checkBulletCollisions(Tank& player, Tank& player2, int map[20][25]) {
    for (auto it_enemy = enemies.begin(); it_enemy != enemies.end();) {
        bool enemyHit = false;
        for (auto it_bullet = bullets.begin(); it_bullet != bullets.end();) {
            SDL_Rect bulletRect = it_bullet->getRect();
            if (it_bullet->active && SDL_HasIntersection(&bulletRect, &it_enemy->rect)) {
                it_bullet->active = false;
                it_enemy->hp--;
                enemyHit = true;
                it_bullet = bullets.erase(it_bullet);
                break;
            } else {
                ++it_bullet;
            }
        }

        if (enemyHit && it_enemy->hp <= 0) {
            score++;
            enemiestank--;
            it_enemy = enemies.erase(it_enemy);
            spawnNewEnemy(player, player2, map);
        } else {
            ++it_enemy;
        }
    }
}

void spawnEnemies(Tank& player, Tank& player2) {
    for (int i = 0; i < 5; ++i) {
        spawnNewEnemy(player, player2, map);
    }
}

void showGameOver(int finalScore) {
    TTF_Font* font = TTF_OpenFont("NothingYouCouldDo.ttf", 128);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Color whiteColor = { 255, 255, 255 };
    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, "GAME OVER", whiteColor);
    SDL_Surface* surfaceScore = TTF_RenderText_Solid(font, ("Score: " + std::to_string(finalScore)).c_str(), whiteColor);
    SDL_Texture* message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
    SDL_Texture* score = SDL_CreateTextureFromSurface(renderer, surfaceScore);
    SDL_FreeSurface(surfaceMessage);
    SDL_FreeSurface(surfaceScore);
    SDL_Rect messageRect = { SCREEN_WIDTH / 2 - surfaceMessage->w / 2, SCREEN_HEIGHT / 3, surfaceMessage->w, surfaceMessage->h };
    SDL_Rect scoreRect = { SCREEN_WIDTH / 2 - surfaceScore->w / 2, SCREEN_HEIGHT / 2, surfaceScore->w, surfaceScore->h };
    SDL_RenderCopy(renderer, message, NULL, &messageRect);
    SDL_RenderCopy(renderer, score, NULL, &scoreRect);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(message);
    SDL_DestroyTexture(score);
    TTF_CloseFont(font);
    SDL_Delay(4000);

    gameOver = true;
    inMenu = true;
}

void drawMenu() {
    SDL_RenderCopy(renderer, menuTexture, NULL, NULL);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &onePlayerButtonRect);
    SDL_RenderCopy(renderer, onePlayerTextTexture, NULL, &onePlayerTextRect);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &twoPlayerButtonRect);
    SDL_RenderCopy(renderer, twoPlayerTextTexture, NULL, &twoPlayerTextRect);

}

void saveGame(const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << score << std::endl;
        file << enemiestank << std::endl;
        file << player.x << std::endl;
        file << player.y << std::endl;
        file << player.hp << std::endl;
        file << player2.x << std::endl;
        file << player2.y << std::endl;
        file << player2.hp << std::endl;
        file << twoPlayerMode << std::endl;

        for (int i = 0; i < 20; ++i) {
            for (int j = 0; j < 25; ++j) {
                file << map[i][j] << " ";
            }
            file << std::endl;
        }
        file << enemies.size() << std::endl;
        for (const auto& enemy : enemies) {
            file << enemy.x << " " << enemy.y << " " << enemy.hp << " " << enemy.dirX << " " << enemy.dirY << " " << enemy.lastShotTime << " " << enemy.shootInterval << std::endl;
        }

        file.close();
        std::cout << "Game saved to " << filename << std::endl;
    } else {
        std::cerr << "Unable to open file for saving." << std::endl;
    }
}

bool loadGame(const std::string& filename) {
    std::ifstream file(filename);
    if (file.is_open()) {
        file >> score;
        file >> enemiestank;
        file >> player.x;
        file >> player.y;
        file >> player.hp;
        player.rect.x = player.x;
        player.rect.y = player.y;
        file >> player2.x;
        file >> player2.y;
        file >> player2.hp;
        player2.rect.x = player2.x;
        player2.rect.y = player2.y;
        file >>        twoPlayerMode;

        for (int i = 0; i < 20; ++i) {
            for (int j = 0; j < 25; ++j) {
                file >> map[i][j];
            }
        }

        int enemyCount;
        file >> enemyCount;
                enemies.clear();
        for (int i = 0; i < enemyCount; ++i) {
            int x, y, hp, dirX, dirY;
            Uint32 lastShotTimeLoad;
            Uint32 shootIntervalLoad;
            file >> x >> y >> hp >> dirX >> dirY >> lastShotTimeLoad >> shootIntervalLoad;
            EnemyTank enemy(x, y);
            enemy.hp = hp;
            enemy.dirX = dirX;
            enemy.dirY = dirY;
            enemy.lastShotTime = lastShotTimeLoad; //load lastShotTime from file
            enemy.shootInterval = shootIntervalLoad;
            enemy.rect.x = x;
            enemy.rect.y = y;
            enemies.push_back(enemy);
        }
        file.close();
        std::cout << "Game loaded from " << filename << std::endl;
        return true;
    } else {
        std::cerr << "Unable to open file for loading." << std::endl;
        return false;
    }
}

void resetGame() {
    score = 0;
    enemiestank = 0;
    enemies.clear();
    bullets.clear();
    enemyBullets.clear();
    player.hp = 3;
    player2.hp = 3;
}

int main(int argc, char* argv[]) {
    srand(time(0));

    if (!init()) return 1;

    player.startX = TILE_SIZE;
    player.startY = TILE_SIZE;
    player.resetPosition();
    player2.startX = SCREEN_WIDTH - 4*TILE_SIZE; // Góc trên bên phải (trừ đi kích thước tank)
    player2.startY = TILE_SIZE;
    player2.resetPosition();


    Uint32 lastMoveTime = 0;
    giftSpawnTime = SDL_GetTicks() + giftSpawnDelay;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            if (inMenu) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    if (e.button.x > onePlayerButtonRect.x && e.button.x < onePlayerButtonRect.x + onePlayerButtonRect.w &&
                        e.button.y > onePlayerButtonRect.y && e.button.y < onePlayerButtonRect.y + onePlayerButtonRect.h) {
                        inMenu = false;
                        gameOver = false;
                        twoPlayerMode = false;
                        resetGame();
                        generateRandomMap(map);
                        spawnEnemies(player, player2);
                        giftSpawnTime = SDL_GetTicks() + giftSpawnDelay;
                        player.resetPosition();
                        player.hp = 3;
                        player2.resetPosition();
                        player2.hp = 3;

                    }

                    else if (e.button.x > twoPlayerButtonRect.x && e.button.x < twoPlayerButtonRect.x + twoPlayerButtonRect.w &&
                             e.button.y > twoPlayerButtonRect.y && e.button.y < twoPlayerButtonRect.y + twoPlayerButtonRect.h) {
                        inMenu = false;
                        gameOver = false;
                        twoPlayerMode = true;
                        resetGame();
                        generateRandomMap(map);
                        spawnEnemies(player, player2);
                        giftSpawnTime = SDL_GetTicks() + giftSpawnDelay;
                        player.resetPosition();
                         // Set Player 1 spawn
                        player.startX = TILE_SIZE;
                        player.startY = TILE_SIZE;
                        player.x = player.startX;
                        player.y = player.startY;
                        player.rect.x = player.x;
                        player.rect.y = player.y;
                        player.dirX = 0;
                        player.dirY = -1;
                        player.directionTexture = tankDownTexture;

                        // Set Player 2 spawn in the other corner
                        player2.startX = SCREEN_WIDTH - 4 * TILE_SIZE;
                        player2.startY = TILE_SIZE;
                        player2.x = player2.startX;
                        player2.y = player2.startY;
                        player2.rect.x = player2.x;
                        player2.rect.y = player2.y;
                        player2.dirX = 0;
                        player2.dirY = -1;
                        player2.directionTexture = tankDownTexture;

                        player.hp = 3;
                        player2.hp = 3;

                    }
                }
            }
            else {
                if (e.type == SDL_KEYDOWN) {
                    if (!twoPlayerMode) {
                        if (e.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                            player.shoot();
                        }
                    } else{
                        if (e.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                            player.shoot();
                        }
                        //Thay doi o day
                        if (e.key.keysym.scancode == SDL_SCANCODE_J) {
                            player2.shoot();
                        }
                    }
                    if (e.key.keysym.scancode == SDL_SCANCODE_S) {
                        saveGame("savegame.txt");
                    }
                    if(e.key.keysym.scancode == SDL_SCANCODE_L){
                        loadGame("savegame.txt");
                    }
                }
            }
        }

        if (inMenu) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            drawMenu();
            SDL_RenderPresent(renderer);
        }
        else {
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime - lastMoveTime >= MOVE_DELAY) {
                const Uint8* keystate = SDL_GetKeyboardState(NULL);
                if (!twoPlayerMode) {
                    if (keystate[SDL_SCANCODE_LEFT]) player.move(-1, 0, tankLeftTexture);
                    else if (keystate[SDL_SCANCODE_RIGHT]) player.move(1, 0, tankRightTexture);
                    else if (keystate[SDL_SCANCODE_UP]) player.move(0, -1, tankUpTexture);
                    else if (keystate[SDL_SCANCODE_DOWN]) player.move(0, 1, tankDownTexture);
                } else {
                    if (keystate[SDL_SCANCODE_LEFT]) player.move(-1, 0, tankLeftTexture);
                    else if (keystate[SDL_SCANCODE_RIGHT]) player.move(1, 0, tankRightTexture);
                    else if (keystate[SDL_SCANCODE_UP]) player.move(0, -1, tankUpTexture);
                    else if (keystate[SDL_SCANCODE_DOWN]) player.move(0, 1, tankDownTexture);

                    if (keystate[SDL_SCANCODE_A]) player2.move(-1, 0, tankLeftTexture);
                    else if (keystate[SDL_SCANCODE_D]) player2.move(1, 0, tankRightTexture);
                    else if (keystate[SDL_SCANCODE_W]) player2.move(0, -1, tankUpTexture);
                    else if (keystate[SDL_SCANCODE_S]) player2.move(0, 1, tankDownTexture);
                }
                lastMoveTime = currentTime;
            }

            for (auto it = enemyBullets.begin(); it != enemyBullets.end();) {
                it->update();
                if (checkCollisionWithWall(it->rect)) {
                    it = enemyBullets.erase(it);
                    continue;
                }
                if (SDL_HasIntersection(&it->rect, &player.rect)) {
                    player.hp -= 1; // Player loses a life
                    if (player.hp > 0) {
                       player.resetPositionAtSpawn(); // Reset player position
                    } else {
                       showGameOver(score); // Game Over if no lives left
                       if (twoPlayerMode) {
                           inMenu = true;
                       }
                    }
                    it = enemyBullets.erase(it);
                    continue;
                }
                 if (SDL_HasIntersection(&it->rect, &player2.rect)) {
                    player2.hp -= 1; // Player loses a life
                    if (player2.hp > 0) {
                       player2.resetPositionAtSpawn(); // Reset player position
                    } else {
                       showGameOver(score); // Game Over if no lives left
                       if (twoPlayerMode) {
                           inMenu = true;
                       }
                    }
                    it = enemyBullets.erase(it);
                    continue;
                }
                int mapX = it->x / TILE_SIZE;
                int mapY = it->y / TILE_SIZE;
                if (mapY >= 0 && mapY < 20 && mapX >= 0 && mapX < 25) {
                    if (map[mapY][mapX] == 1) {
                        brickHealth[mapY][mapX]++;
                        if (brickHealth[mapY][mapX] >= 5) {
                            map[mapY][mapX] = 0;
                        }
                        it = enemyBullets.erase(it);
                        continue;
                    }
                    else if (map[mapY][mapX] == 2) {
                        it = enemyBullets.erase(it);
                        continue;
                    }
                }
                ++it;
            }
             // Check collision between player and enemies
            for (auto it_enemy = enemies.begin(); it_enemy != enemies.end();) {
                if (SDL_HasIntersection(&player.rect, &it_enemy->rect)) {
                    player.hp -= 1; // Player loses a life
                    if (player.hp > 0) {
                       player.resetPositionAtSpawn(); // Reset player position
                    } else {
                       showGameOver(score); // Game Over if no lives left
                       if (twoPlayerMode) {
                           inMenu = true;
                       }
                    }
                    break; // Only lose one life per frame if colliding with multiple enemies
                }
                if (SDL_HasIntersection(&player2.rect, &it_enemy->rect)) {
                    player2.hp -= 1; // Player loses a life
                    if (player2.hp > 0) {
                       player2.resetPositionAtSpawn(); // Reset player position
                     } else {
                       showGameOver(score); // Game Over if no lives left
                       if (twoPlayerMode) {
                           inMenu = true;
                       }
                     }
                    break; // Only lose one life per frame if colliding with multiple enemies
                }
                ++it_enemy;
            }

            for (auto& enemy : enemies) {
                enemy.moveLikeHuman();
                enemy.shootIfReady();
                enemy.updateBullets();
            }

            for (auto it = bullets.begin(); it != bullets.end();) {
                it->update();
                if (!it->active) it = bullets.erase(it);
                else ++it;
            }

            checkBulletCollisions(player, player2, map);

            if (giftBox == nullptr && currentTime >= giftSpawnTime) {
                spawnGiftBox();
            }

            if (giftBox != nullptr && giftBox->active && (SDL_HasIntersection(&player.rect, &giftBox->rect) || SDL_HasIntersection(&player2.rect, &giftBox->rect))) {
                if (SDL_HasIntersection(&player.rect, &giftBox->rect))
                {
                     player.hp = std::min(player.hp + 1, player.maxHp);
                } else {
                     player2.hp = std::min(player2.hp + 1, player2.maxHp);
                }
                giftBox->active = false;
                delete giftBox;
                giftBox = nullptr;
                giftSpawnTime = SDL_GetTicks() + giftSpawnDelay;
            }
            SDL_Rect backgroundRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            SDL_RenderCopy(renderer, backgroundTexture, NULL, &backgroundRect);

            drawMap(renderer);
            player.draw();
            if (twoPlayerMode) {
                player2.draw();
            }
            for (auto& enemy : enemies) enemy.draw();
            for (auto& bullet : bullets) bullet.draw();
            for (auto& bullet : enemyBullets) bullet.draw();
            player.drawHearts();
            if (twoPlayerMode) {
               player2.drawHearts();
            }

            if (giftBox != nullptr && giftBox->active) {
                giftBox->draw(renderer, giftTexture);
            }

            SDL_RenderPresent(renderer);
        }
    }

    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(tankDownTexture);
    SDL_DestroyTexture(tankUpTexture);
    SDL_DestroyTexture(tankRightTexture);
    SDL_DestroyTexture(tankLeftTexture);
    SDL_DestroyTexture(enemyTankUpTexture);
    SDL_DestroyTexture(enemyTankDownTexture);
    SDL_DestroyTexture(enemyTankRightTexture);
    SDL_DestroyTexture(enemyTankLeftTexture);
    SDL_DestroyTexture(groundTexture);
    SDL_DestroyTexture(wallTexture);
    SDL_DestroyTexture(steelWallTexture);
    SDL_DestroyTexture(menuTexture);
    SDL_DestroyTexture(startTextTexture);
    SDL_DestroyTexture(giftTexture);
    SDL_DestroyTexture(onePlayerTextTexture);
    SDL_DestroyTexture(twoPlayerTextTexture);

    TTF_CloseFont(menuFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    if (giftBox != nullptr) {
        delete giftBox;
    }

    Mix_FreeChunk(shootSound);
    Mix_Quit();
    SDL_Quit();

    return 0;
}
