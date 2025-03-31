#define SDL_MAIN_HANDLED
#include <bits/stdc++.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <fstream>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 640;
const int TILE_SIZE = 40;
const int TANK_SIZE = 30;
const int MOVE_DELAY = 40;
const double BULLET_SPEED = 0.25;
const int BULLET_SIZE = 20;
const int MAP_WIDTH = 800;
const int MAP_HEIGHT = 600;

int score = 0;
int enemiestank = 0;
bool running = true;
bool inMenu = true;
bool gameOver = false; // New flag to track game over state
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
SDL_Rect continueButtonRect;  // New button for continuing

TTF_Font* menuFont = nullptr;
SDL_Color textColor = { 255, 255, 255 };
SDL_Texture* startTextTexture = nullptr;
SDL_Rect startTextRect;
SDL_Texture* continueTextTexture = nullptr; // Text texture for continue button
SDL_Rect continueTextRect;

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
    int moveDirection; // 0: up, 1: down, 2: left, 3: right

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
        moveDirection = rand() % 4;  // Random initial direction
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
        int mapX = (newX + 20) / TILE_SIZE;
        int mapY = (newY + 20) / TILE_SIZE;
        int mapX2 = (newX) / TILE_SIZE;
        int mapY2 = (newY) / TILE_SIZE;

        if (newX < 0 || newX >= SCREEN_WIDTH || newY < 0 || newY >= SCREEN_HEIGHT) {
            return false;
        }

        return (mapY >= 0 && mapY < 20 && mapX >= 0 && mapX < 25 && map[mapY][mapX] == 0
            && mapY2 >= 0 && mapY2 < 20 && mapX2 >= 0 && mapX2 < 25 && map[mapY2][mapX2] == 0
            && mapY >= 0 && mapY < 20 && mapX2 >= 0 && mapX2 < 25 && map[mapY][mapX2] == 0
            && mapY2 >= 0 && mapY2 < 20 && mapX >= 0 && mapX < 25 && map[mapY2][mapX] == 0);
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
        if (currentTime - lastShotTime >= 1000) {
            enemyBullets.emplace_back(x + TANK_SIZE / 2 - BULLET_SIZE / 2,
                y + TANK_SIZE / 2 - BULLET_SIZE / 2,
                dirX * BULLET_SPEED, dirY * BULLET_SPEED);
            lastShotTime = currentTime;
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
    void drawHearts() {
        SDL_Rect heartRect = { 10, 10, 20, 20 };
        for (int i = 0; i < hp; ++i) {
            heartRect.x = 10 + i * 25;
            SDL_RenderCopy(renderer, heartTexture, NULL, &heartRect);
        }
    }
    Uint32 lastShotTime = 0;
    const Uint32 shootCooldown = 500;
    Tank(int startX, int startY) : startX(startX), startY(startY), x(startX), y(startY) {
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
        int mapX = (newX + 20) / 40;
        int mapY = (newY + 20) / 40;
        int mapX2 = (newX) / 40;
        int mapY2 = (newY) / 40;
        if (newX < 0 || newX >= SCREEN_WIDTH || newY < 0 || newY >= SCREEN_HEIGHT) {
            return false;
        }
        return (mapY >= 0 && mapY < 20 && mapX >= 0 && mapX < 25 && map[mapY][mapX] == 0
            && mapY2 >= 0 && mapY2 < 20 && mapX2 >= 0 && mapX2 < 25 && map[mapY2][mapX2] == 0
            && mapY >= 0 && mapY < 20 && mapX2 >= 0 && mapX2 < 25 && map[mapY][mapX2] == 0
            && mapY2 >= 0 && mapY2 < 20 && mapX >= 0 && mapX < 25 && map[mapY2][mapX] == 0);
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
            lastShotTime = currentTime;
        }
    }
    void draw() {
        SDL_RenderCopy(renderer, directionTexture, NULL, &rect);
    }
};

Tank player(TILE_SIZE, TILE_SIZE);

struct GiftBox {
    int x, y;
    SDL_Rect rect;
    bool active;
    GiftBox(int startX, int startY) : x(startX), y(startY), active(true) {
        rect = { x, y, TILE_SIZE, TILE_SIZE };
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
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    window = SDL_CreateWindow("Tank", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    if (TTF_Init() == -1) {
        std::cerr << "Failed to initialize TTF: " << TTF_GetError() << std::endl;
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
    startButtonRect.y = SCREEN_HEIGHT / 2 - 100;  // Moved start button up
    startButtonRect.w = 200;
    startButtonRect.h = 60;

    continueButtonRect.x = SCREEN_WIDTH / 2 - 100;
    continueButtonRect.y = SCREEN_HEIGHT / 2 + 20; // Positioned below start
    continueButtonRect.w = 200;
    continueButtonRect.h = 60;

    menuFont = TTF_OpenFont("NothingYouCouldDo.ttf", 36);
    if (!menuFont) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
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

    SDL_Surface* continueSurface = TTF_RenderText_Solid(menuFont, "Continue", textColor);
    if (!continueSurface) {
        std::cerr << "Failed to render continue text: " << TTF_GetError() << std::endl;
        return false;
    }
    continueTextTexture = SDL_CreateTextureFromSurface(renderer, continueSurface);
    continueTextRect.w = continueSurface->w;
    continueTextRect.h = continueSurface->h;
    continueTextRect.x = continueButtonRect.x + (continueButtonRect.w - continueTextRect.w) / 2;
    continueTextRect.y = continueButtonRect.y + (continueButtonRect.h - continueTextRect.h) / 2;
    SDL_FreeSurface(continueSurface);

    return tankUpTexture && tankDownTexture && tankLeftTexture && tankRightTexture && menuTexture && startTextTexture && continueTextTexture && giftTexture && backgroundTexture;
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

bool isValidSpawnPosition(int x, int y, const Tank& player, const int map[20][25]) {
    SDL_Rect spawnRect = { x, y, TANK_SIZE, TANK_SIZE };
    if (SDL_HasIntersection(&spawnRect, &player.rect)) return false;

    int startMapX = x / TILE_SIZE;
    int startMapY = y / TILE_SIZE;
    int endMapX = (x + TANK_SIZE - 1) / TILE_SIZE;
    int endMapY = (y + TANK_SIZE - 1) / TILE_SIZE;

    for (int i = startMapY; i <= endMapY; ++i) {
        for (int j = startMapX; j <= endMapX; ++j) {
            if (i < 0 || i >= 20 || j < 0 || j >= 25) return false;
            if (map[i][j] != 0) return false;
        }
    }

    return true;
}

bool isValidSpawnPosition(int x, int y, const int map[20][25]) {
    int startMapX = x / TILE_SIZE;
    int startMapY = y / TILE_SIZE;
    int endMapX = (x + TILE_SIZE - 1) / TILE_SIZE;
    int endMapY = (y + TILE_SIZE - 1) / TILE_SIZE;

    for (int i = startMapY; i <= endMapY; ++i) {
        for (int j = startMapX; j <= endMapX; ++j) {
            if (i < 0 || i >= 20 || j < 0 || j >= 25) return false;
            if (map[i][j] != 0) return false;
        }
    }

    return true;
}

void spawnNewEnemy(Tank& player, int map[20][25]) {
    int x, y;
    bool validSpawn = false;
    const int MAX_ATTEMPTS = 1000;

    for (int i = 0; i < MAX_ATTEMPTS; ++i) {
        x = rand() % (SCREEN_WIDTH - TANK_SIZE);
        y = rand() % (SCREEN_HEIGHT - TANK_SIZE);

        if (isValidSpawnPosition(x, y, player, map)) {
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
            x = rand() % (SCREEN_WIDTH - TILE_SIZE);
            y = rand() % (SCREEN_HEIGHT - TILE_SIZE);

            if (isValidSpawnPosition(x, y, map)) {
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


void checkBulletCollisions(Tank& player, int map[20][25]) {
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
            spawnNewEnemy(player, map);
        } else {
            ++it_enemy;
        }
    }
}

void spawnEnemies() {
    for (int i = 0; i < 5; ++i) {
        spawnNewEnemy(player, map);
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
    SDL_Delay(4000); // Display Game Over for 4 seconds

    // Return to menu instead of closing
    gameOver = true;
    inMenu = true;
}

void drawMenu() {
    SDL_RenderCopy(renderer, menuTexture, NULL, NULL);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &startButtonRect);
    SDL_RenderCopy(renderer, startTextTexture, NULL, &startTextRect);

    // Only draw the Continue button if a game can be loaded
    std::ifstream saveFile("savegame.txt");
    if (saveFile.good() && !gameOver) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &continueButtonRect);
        SDL_RenderCopy(renderer, continueTextTexture, NULL, &continueTextRect);
    }
    saveFile.close();
}

void saveGame(const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << score << std::endl;
        file << enemiestank << std::endl;
        file << player.x << std::endl;
        file << player.y << std::endl;
        file << player.hp << std::endl;

        for (int i = 0; i < 20; ++i) {
            for (int j = 0; j < 25; ++j) {
                file << map[i][j] << " ";
            }
            file << std::endl;
        }
        file << enemies.size() << std::endl;
        for (const auto& enemy : enemies) {
            file << enemy.x << " " << enemy.y << " " << enemy.hp << " " << enemy.dirX << " " << enemy.dirY << std::endl;
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
            file >> x >> y >> hp >> dirX >> dirY;
            EnemyTank enemy(x, y);
            enemy.hp = hp;
            enemy.dirX = dirX;
            enemy.dirY = dirY;
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
}

int main(int argc, char* argv[]) {
    srand(time(0));

    if (!init()) return 1;

    player.startX = TILE_SIZE;
    player.startY = TILE_SIZE;
    player.resetPosition();


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
                    if (e.button.x > startButtonRect.x && e.button.x < startButtonRect.x + startButtonRect.w &&
                        e.button.y > startButtonRect.y && e.button.y < startButtonRect.y + startButtonRect.h) {
                        inMenu = false;
                        gameOver = false;
                        resetGame();
                        generateRandomMap(map);
                        spawnEnemies();
                        giftSpawnTime = SDL_GetTicks() + giftSpawnDelay;
                        player.resetPosition();
                        player.hp = 3;

                    }
                     // Handle Continue button click
                    if (e.button.x > continueButtonRect.x && e.button.x < continueButtonRect.x + continueButtonRect.w &&
                        e.button.y > continueButtonRect.y && e.button.y < continueButtonRect.y + continueButtonRect.h) {
                        if (loadGame("savegame.txt")) {
                            inMenu = false;
                            gameOver = false;
                        }
                    }
                }
            }
            else {
                if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                    player.shoot();
                }
                if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_S) {
                    saveGame("savegame.txt");
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
                if (keystate[SDL_SCANCODE_LEFT]) player.move(-1, 0, tankLeftTexture);
                else if (keystate[SDL_SCANCODE_RIGHT]) player.move(1, 0, tankRightTexture);
                else if (keystate[SDL_SCANCODE_UP]) player.move(0, -1, tankUpTexture);
                else if (keystate[SDL_SCANCODE_DOWN]) player.move(0, 1, tankDownTexture);
                lastMoveTime = currentTime;
            }

            for (auto it = enemyBullets.begin(); it != enemyBullets.end();) {
                it->update();
                if (checkCollisionWithWall(it->rect)) {
                    it = enemyBullets.erase(it);
                    continue;
                }
                if (SDL_HasIntersection(&it->rect, &player.rect)) {
                    player.hp -= 1;
                    if (player.hp > 0)
                    {
                        player.resetPositionAtSpawn();
                    }
                    else {
                        showGameOver(score);
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

            checkBulletCollisions(player, map);

            if (giftBox == nullptr && currentTime >= giftSpawnTime) {
                spawnGiftBox();
            }

            if (giftBox != nullptr && giftBox->active && SDL_HasIntersection(&player.rect, &giftBox->rect)) {
                player.hp = std::min(player.hp + 1, player.maxHp);
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
            for (auto& enemy : enemies) enemy.draw();
            for (auto& bullet : bullets) bullet.draw();
            for (auto& bullet : enemyBullets) bullet.draw();
            player.drawHearts();

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
    SDL_DestroyTexture(continueTextTexture);
    SDL_DestroyTexture(giftTexture);

    TTF_CloseFont(menuFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    if (giftBox != nullptr) {
        delete giftBox;
    }

    SDL_Quit();

    return 0;
}
