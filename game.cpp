#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <semaphore.h>

using namespace std;
using namespace sf;

pthread_mutex_t mtx;

sem_t enemySem;
sem_t cherrySem;
sem_t MainGame;

int playerSpeed = 200;
int enemySpeed = 0;

bool ifcherry = false;
bool eatghost = false;

float boostTime[4];
bool ifboost[4];
bool boosting[4];

float eatTime = 0;

struct EnemyThreadArgs {
    int& enemyX;
    int& enemyY;
    int (*maze)[32];
    int& speed;
    int& i;
};

struct PlayerThreadArgs {
    int& pacmanX;
    int& pacmanY;
    int (*maze)[32];
    int& score;
};

struct collisionThreadArgs{
    sf::Sprite& pacman;
    sf::Sprite& enemy;
    int& enemyX;
    int& enemyY;
    int& pacmanX;
    int& pacmanY;
    int (*maze)[32];
    int& lives;
    int& speed;
};

void drawMaze(sf::RenderWindow& window, int maze[29][32], sf::RectangleShape& wall, sf::CircleShape& food, sf::Sprite (&cherry)[4], sf::Sprite (&boost)[2]) {
    pthread_mutex_lock(&mtx);
    window.clear(sf::Color::Black);

    int count = 0;
    int boostcount = 0;

    for (int y = 0; y < 29; y++) {
        for (int x = 0; x < 32; x++) {
            if (maze[y][x] == 0) {
                wall.setPosition(x * 20.f, y * 20.f);
                window.draw(wall);
            } else if (maze[y][x] == 2) {
                food.setPosition(x * 20.f + 8.f, y * 20.f + 8.f);
                window.draw(food);
            } else if (maze[y][x] == 3) {
                cherry[count].setPosition(x * 20.f + 5.f, y * 20.f + 5.f);
                window.draw(cherry[count]);
                count++;
            } else if (maze[y][x] == 4) {
                boost[boostcount].setPosition(x * 20.f + 5.f, y * 20.f + 5.f);
                window.draw(boost[boostcount]);
                boostcount++;
            }
        }
    }

    pthread_mutex_unlock(&mtx);
}


// Function to draw the player
void drawPlayer(sf::RenderWindow& window, sf::Sprite& pacmanSprite, int pacmanX, int pacmanY) {
    pthread_mutex_lock(&mtx);

    pacmanSprite.setPosition(pacmanX * 20.f + 6.f, pacmanY * 20.f + 5.f);
    window.draw(pacmanSprite);

    pthread_mutex_unlock(&mtx);
}

// Function to handle player movement
void* movePlayer(void* arg) {
    PlayerThreadArgs* args = static_cast<PlayerThreadArgs*>(arg);

    int& pacmanX = args->pacmanX;
    int& pacmanY = args->pacmanY;
    int (*maze)[32] = args->maze;
    int& score = args->score;

    while (true) {
        pthread_mutex_lock(&mtx);
    
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            if (pacmanY > 0 && (maze[pacmanY - 1][pacmanX] == 1 || maze[pacmanY - 1][pacmanX] == 2 || maze[pacmanY - 1][pacmanX] == 3 || maze[pacmanY - 1][pacmanX] == 4))
                pacmanY--;
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            if (pacmanY < 28 && (maze[pacmanY + 1][pacmanX] == 1 || maze[pacmanY + 1][pacmanX] == 2 || maze[pacmanY + 1][pacmanX] == 3 || maze[pacmanY - 1][pacmanX] == 4))
                pacmanY++;
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            if (pacmanX > 0 && (maze[pacmanY][pacmanX - 1] == 1 || maze[pacmanY][pacmanX - 1] == 2 || maze[pacmanY][pacmanX - 1] == 3 || maze[pacmanY - 1][pacmanX] == 4))
                pacmanX--;
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            if (pacmanX < 32 && (maze[pacmanY][pacmanX + 1] == 1 || maze[pacmanY][pacmanX + 1] == 2 || maze[pacmanY][pacmanX + 1] == 3 || maze[pacmanY - 1][pacmanX] == 4))
                pacmanX++;
        }

        pthread_mutex_unlock(&mtx);

        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Pause for smoother movement
    }
}

void* checkCollision(void* arg) {
    collisionThreadArgs* args = static_cast<collisionThreadArgs*>(arg);

    sf::Sprite& pacman = args->pacman;
    sf::Sprite& enemy = args->enemy;
    int& enemyX = args->enemyX;
    int& enemyY = args->enemyY;
    int& pacmanX = args->pacmanX;
    int& pacmanY = args->pacmanY;
    int& lives =args->lives;
    int (*maze)[32] = args->maze;

    while (true) {
        pthread_mutex_lock(&mtx);

        if (pacmanX==enemyX&&pacmanY==enemyY) {
            if(eatghost){
                sem_post(&enemySem);
                playerSpeed+= 5;
                enemyX = 16;
                enemyY = 14;
                enemy.setPosition(enemyX * 20.f + 6.f, enemyY * 20.f + 5.f);
            }
            else{
                pacmanX=1;
                pacmanY=1;
                lives--;
                pacman.setPosition(pacmanX * 20.f + 6.f, pacmanY * 20.f + 5.f);
            }
            
        }

        pthread_mutex_unlock(&mtx);
    }
}

void* updateScore(void* arg){
    PlayerThreadArgs* args=static_cast<PlayerThreadArgs*>(arg);

    int& pacmanX = args->pacmanX;
    int& pacmanY = args->pacmanY;
    int (*maze)[32] = args->maze;
    int& score = args->score;


    while (true) {

        if (maze[pacmanY][pacmanX]==2 ) {
            score++;
            maze[pacmanY][pacmanX]=1;
        }
        else if(maze[pacmanY][pacmanX]==3){
            sem_wait(&cherrySem);
            maze[pacmanY][pacmanX]=1;
            score+=10;
            ifcherry=true;
        }
    }   
}

void* updateEnemyBoost(void* arg){
    EnemyThreadArgs* args=static_cast<EnemyThreadArgs*>(arg);

    int& enemyX = args->enemyX;
    int& enemyY = args->enemyY;
    int (*maze)[32] = args->maze;
    int& speed = args->speed;
    int& i = args->i;

    while (true) {

        if (maze[enemyY][enemyX]==4 ) {
            maze[enemyY][enemyX]=1;
            ifboost[i]=true;
            boostTime[i] = 3;
        }
    }   
}

// Function to draw the Enemies
void drawEnemy(sf::RenderWindow& window, sf::Sprite& ghost, int enemyX, int enemyY) {
    pthread_mutex_lock(&mtx);
    
    ghost.setPosition(enemyX * 20.f + 5.f, enemyY * 20.f + 5.f);
    window.draw(ghost);
    
    pthread_mutex_unlock(&mtx);
}

void* teleportEnemy(void* arg){
    EnemyThreadArgs* args = static_cast<EnemyThreadArgs*>(arg);

    int& enemyX = args->enemyX;
    int& enemyY = args->enemyY;
    int (*maze)[32] = args->maze;
    
    if(maze[enemyX][enemyY]==4)
        enemySpeed += 10;
    
    if (enemyX < 1){
      enemyX = 14;
      enemyY = 12;
    
    }
    else if (enemyX > 30){
      enemyX = 12;
      enemyY = 14;
    
    }
        
    return nullptr;
}

void* moveEnemy(void* arg) {
    EnemyThreadArgs* args = static_cast<EnemyThreadArgs*>(arg);

    int& enemyX = args->enemyX;
    int& enemyY = args->enemyY;
    int (*maze)[32] = args->maze;
    int& speed = args->speed;
    
    int direction=0;
        sem_wait(&enemySem);

    while (true) {
        int move=rand()%4;
        bool canMoveUp = (enemyY > 0 && maze[enemyY - 1][enemyX] != 0);
        bool canMoveDown = (enemyY < 28 && maze[enemyY + 1][enemyX] != 0);
        bool canMoveLeft = (maze[enemyY][enemyX - 1] != 0);
        bool canMoveRight = (maze[enemyY][enemyX + 1] != 0);

        if (direction == 0 && canMoveUp)
            enemyY--;
        else if (direction == 1 && canMoveDown)
            enemyY++;
        else if (direction == 2 && canMoveLeft)
            enemyX--;
        else if (direction == 3 && canMoveRight)
            enemyX++;
        else {
            do {
                direction = rand() % 4; // 0: Up, 1: Down, 2: Left, 3: Right
            } while ((direction == 0 && !canMoveUp) || 
                     (direction == 1 && !canMoveDown) ||
                     (direction == 2 && !canMoveLeft) ||
                     (direction == 3 && !canMoveRight));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(speed - enemySpeed));
    }
    return nullptr;
}

void* GameEngine(void* arg)
{
    pthread_mutex_init(&mtx, NULL);

    sem_init(&cherrySem, 0, 1);
    sem_init(&enemySem, 0, 2);
    
    RenderWindow window(sf::VideoMode(640, 640), "Complex SFML Maze");

    Clock clock;

    sf::Color wallColor = sf::Color::Blue;
    sf::Color bgColor = sf::Color::Black;
    sf::Color pacmanColor = sf::Color::Yellow;
    sf::Color foodColor = sf::Color::Yellow;

    sf::RectangleShape wall(sf::Vector2f(20.f, 20.f));
    wall.setFillColor(wallColor);

    sf::CircleShape food(3.f);
    food.setFillColor(foodColor);

    sf::Texture pacmanTexture;
    if (!pacmanTexture.loadFromFile("pacman.png")) {
        return nullptr;
    }
    sf::Font font;
            if (!font.loadFromFile("times new roman.ttf")) {
                return nullptr;
            }

    sf::Sprite pacmanSprite(pacmanTexture);
    pacmanSprite.setScale(0.02f, 0.02f);

    sf::FloatRect bounds = pacmanSprite.getLocalBounds();
    pacmanSprite.setOrigin(bounds.width / 3.f, bounds.height / 3.f);

    sf::Texture redghost, blueghost, pinkghost, greenghost, eatenGhost, cherry, boost;
    redghost.loadFromFile("ghost1.png");
    blueghost.loadFromFile("ghost3.png");
    pinkghost.loadFromFile("ghost4.png");
    greenghost.loadFromFile("ghost2.png");
    eatenGhost.loadFromFile("eatenghost.png");
    cherry.loadFromFile("cherry.png");
    boost.loadFromFile("fireBoost.png");

    sf::Sprite cherrysprite[4];
    for(int i=0 ; i<4; i++){
        
        cherrysprite[i].setTexture(cherry);
        cherrysprite[i].setScale(0.04f, 0.04f);

        sf::FloatRect bounds = cherrysprite[i].getLocalBounds();
        cherrysprite[i].setOrigin(bounds.width / 3.f, bounds.height / 3.f);

    }

    sf::Sprite enemyboost[2];
    for(int i=0 ; i<4; i++){
        
        enemyboost[i].setTexture(boost);
        enemyboost[i].setScale(0.02f, 0.02f);

        sf::FloatRect bounds = enemyboost[i].getLocalBounds();
        enemyboost[i].setOrigin(bounds.width / 3.f, bounds.height / 3.f);

    }

    sf::Sprite redsprite(redghost), bluesprite(blueghost), pinksprite(pinkghost), greensprite(greenghost);
    redsprite.setScale(0.03f, 0.03f);
    bluesprite.setScale(0.015f, 0.015f);
    pinksprite.setScale(0.05f, 0.05f);
    greensprite.setScale(0.05f, 0.05f);


    sf::FloatRect redbounds = redsprite.getLocalBounds(), bluebounds = bluesprite.getLocalBounds(), 
    pinkbounds = pinksprite.getLocalBounds(), greenbounds = greensprite.getLocalBounds();

    redsprite.setOrigin(redbounds.width / 3.f, redbounds.height / 3.f);
    bluesprite.setOrigin(bluebounds.width / 3.f, bluebounds.height / 3.f);
    pinksprite.setOrigin(pinkbounds.width / 3.f, pinkbounds.height / 3.f);
    greensprite.setOrigin(greenbounds.width / 3.f, greenbounds.height / 3.f);

    int maze[29][32] = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {2,2,2,2,2,2,2,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,2,2,2,2,2,2,2},
        {0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,0},
        {0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,0},
        {0,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,2,2,2,2,2,2,2,2,3,2,2,2,0},
        {0,2,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,0,0,2,0},
        {0,2,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,0,0,2,0},
        {0,2,0,0,2,2,2,2,2,2,0,0,2,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,0,2,0},
        {0,2,0,0,2,0,0,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,2,0,2,0},
        {0,2,0,0,2,0,0,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,2,0,2,0},  
        {0,2,0,0,2,0,0,0,0,2,0,0,2,2,2,2,2,2,2,2,0,0,2,0,0,0,0,0,2,0,2,0},
        {0,2,0,0,2,0,0,0,0,3,0,0,2,0,5,5,5,5,0,2,0,0,2,0,0,0,0,0,2,0,2,0},
        {2,2,2,2,2,2,2,0,0,2,0,0,2,0,1,1,1,1,0,2,0,0,2,0,0,0,0,0,2,2,2,2},
        {0,0,0,0,0,0,2,0,0,3,0,0,2,0,1,1,1,1,0,2,0,0,2,0,0,0,0,0,2,0,0,0},
        {0,0,0,0,0,0,2,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,2,0,0,0},
        {0,2,2,2,2,2,2,0,0,2,0,0,2,2,2,2,2,2,2,2,0,0,2,0,0,0,0,0,2,2,2,2},
        {0,2,0,0,0,0,2,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,2,0,0,2},
        {0,2,2,2,0,0,2,2,2,2,0,0,2,0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,2,0,0,2},
        {0,0,0,2,0,0,2,0,0,2,2,2,2,0,0,0,0,0,0,2,0,0,2,2,2,2,2,2,2,0,0,2},
        {0,0,0,2,0,0,2,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,2,0,0,2},
        {0,0,0,2,0,0,2,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,4,0,0,2},
        {2,2,2,2,2,2,4,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},        
        {0,0,0,3,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0},
        {0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0},
        {2,2,2,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,2,2,2},
        {0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0},        
        {0,2,2,2,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,2,2,2,0},
        {0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0},
        {0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0}
    };

    srand(time(nullptr));

    int pacmanX = 1;
    int pacmanY = 1;
    int score = 0;
    int lives = 3;

    int enemyX1 = 14, enemyY1 = 12, speed1 = 200, boost1 = 0;
    int enemyX2 = 17, enemyY2 = 12, speed2 = 250, boost2 = 0;
    int enemyX3 = 14, enemyY3 = 13, speed3 = 250, boost3 = 0;
    int enemyX4 = 17, enemyY4 = 13, speed4 = 200, boost4 = 0;

    for(int i=0 ; i<4 ; i++){
        ifboost[i]=false;
        boosting[i]=false;
    }

    pthread_t player_id, Score_id;
    pthread_t t_id1, t_id2, t_id3, t_id4;
    pthread_t boost_id[4];
    pthread_t teleport_id1, teleport_id2, teleport_id3, teleport_id4;
    pthread_t collision_id1, collision_id2, collision_id3, collision_id4;
    
    PlayerThreadArgs playerArgs = {pacmanX, pacmanY, maze, score};

    collisionThreadArgs collisionArgs1 = {pacmanSprite, redsprite, enemyX1, enemyY1, pacmanX, pacmanY, maze, lives, speed1};
    collisionThreadArgs collisionArgs2 = {pacmanSprite, bluesprite, enemyX2, enemyY2, pacmanX, pacmanY, maze, lives, speed2};
    collisionThreadArgs collisionArgs3 = {pacmanSprite, pinksprite, enemyX3, enemyY3, pacmanX, pacmanY, maze, lives, speed3};
    collisionThreadArgs collisionArgs4 = {pacmanSprite, greensprite, enemyX4, enemyY4, pacmanX, pacmanY, maze, lives, speed4};

    pthread_create(&player_id, NULL, movePlayer, &playerArgs);

    pthread_create(&Score_id, NULL, updateScore, &playerArgs);
    
    int one=1, two=2, three=3, four=4;
    EnemyThreadArgs enemyArgs[4] = {{enemyX1, enemyY1, maze, speed1, one}, 
                                    {enemyX2, enemyY2, maze, speed2, two}, 
                                    {enemyX3, enemyY3, maze, speed3, three}, 
                                    {enemyX4, enemyY4, maze, speed4, four}};

    for(int i=0 ; i<4 ; i++)
    pthread_create(&t_id1, NULL, moveEnemy, &enemyArgs[i]);

    pthread_create(&collision_id1, NULL, checkCollision, &collisionArgs1);
    pthread_create(&collision_id2, NULL, checkCollision, &collisionArgs2);
    pthread_create(&collision_id3, NULL, checkCollision, &collisionArgs3);
    pthread_create(&collision_id4, NULL, checkCollision, &collisionArgs4);
    
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("TitleScreen.jpg")) {
        return nullptr;
    }
  
    // Create sprite for the background image
    sf::Sprite backgroundSprite(backgroundTexture);
    backgroundSprite.setScale(
        static_cast<float>(window.getSize().x) / backgroundSprite.getLocalBounds().width,
        static_cast<float>(window.getSize().y) / backgroundSprite.getLocalBounds().height
    );
    
    sf::Text menuText;
    menuText.setFont(font);
    menuText.setString("Press Enter to Start");
    menuText.setCharacterSize(24);
    menuText.setFillColor(sf::Color::White);
    menuText.setPosition(200.f, 300.f);
    bool gameStarted = false;
    
    sf::Text menuText1;
    menuText.setFont(font);
    menuText.setString("Press M to view the menu");
    menuText.setCharacterSize(24);
    menuText.setFillColor(sf::Color::White);
    menuText.setPosition(200.f, 400.f);
    bool menu = false;
    
    sf::Text menuText2;
    menuText.setFont(font);
    menuText.setString("use Arrow Keys for Movement");
    menuText.setCharacterSize(24);
    menuText.setFillColor(sf::Color::White);
    menuText.setPosition(200.f, 200.f);
    
    sf::Text menuText3;
    menuText.setFont(font);
    menuText.setString("Press P to pause the game");
    menuText.setCharacterSize(24);
    menuText.setFillColor(sf::Color::White);
    menuText.setPosition(200.f, 300.f);
    
    sf::Text menuText4;
    menuText.setFont(font);
    menuText.setString("Press Esc to exit the game");
    menuText.setCharacterSize(24);
    menuText.setFillColor(sf::Color::White);
    menuText.setPosition(200.f, 500.f);

    
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(24);
    text.setFillColor(sf::Color::White);
    text.setPosition(10.f, 590.f);
            
    sf::Text text1;
    text1.setFont(font);
    text1.setCharacterSize(24);
    text1.setFillColor(sf::Color::White);
    text1.setPosition(10.f, 610.f);

    while (window.isOpen()) {

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    gameStarted = true;
                }
                if (event.key.code == sf::Keyboard::M) {
                    menu= true;
                }
                if (event.key.code == sf::Keyboard::E) {
                    window.close();
                }
            }
        }
            if (gameStarted) {
            
            if(lives==0)
                window.close();

            drawMaze(window, maze, wall, food, cherrysprite, enemyboost);

            if(ifcherry){
                ifcherry=false;
                eatghost=true;
                eatTime=clock.getElapsedTime().asSeconds() + 5;
                redsprite.setScale(0.05f, 0.05f);
                redsprite.setTexture(eatenGhost);
                bluesprite.setScale(0.05f, 0.05f);
                bluesprite.setTexture(eatenGhost);
                pinksprite.setScale(0.05f, 0.05f);
                pinksprite.setTexture(eatenGhost);
                greensprite.setScale(0.05f, 0.05f);
                greensprite.setTexture(eatenGhost);
            }

            if(eatghost && clock.getElapsedTime().asSeconds()>=eatTime){
                eatghost=false;
                
                redsprite.setTexture(redghost);
                bluesprite.setTexture(blueghost);
                pinksprite.setTexture(pinkghost);
                greensprite.setTexture(greenghost);
                
                redsprite.setScale(0.03f, 0.03f);
                bluesprite.setScale(0.015f, 0.015f);
                pinksprite.setScale(0.05f, 0.05f);
                greensprite.setScale(0.05f, 0.05f);
                
                sem_post(&cherrySem);
            }
            
            drawPlayer(window, pacmanSprite, pacmanX, pacmanY);

            drawEnemy(window, redsprite, enemyX1, enemyY1);
            drawEnemy(window, bluesprite, enemyX2, enemyY2);
            drawEnemy(window, pinksprite, enemyX3, enemyY3);
            drawEnemy(window, greensprite, enemyX4, enemyY4);

            for(int i=0 ; i<4 ; i++)
            pthread_create(&boost_id[i], NULL, updateEnemyBoost, &enemyArgs[i]);
            
            for(int i=0 ; i<4 ; i++)
            pthread_create(&teleport_id1, NULL, teleportEnemy, &enemyArgs[i]);

            for(int i=0 ; i<4 ; i++){
                if(ifboost[i]){
                    if(i==0){
                        speed1 -= 30;
                    }
                    else if(i == 1){
                        speed1 -= 30;
                    }
                    else if(i ==2){
                        speed1 -= 30;
                    }
                    else if(i == 3){
                        speed1 -= 30;
                    }

                        ifboost[i] = false;
                        boosting[i]=true;
                        boostTime[i]=clock.getElapsedTime().asSeconds() + 3; 
                }

                if(boosting[i] && clock.getElapsedTime().asSeconds() >= boostTime[i]){
                    boosting[i]=false;

                    if(i==0){
                        speed1 += 30;
                    }
                    else if(i == 1){
                        speed1 += 30;
                    }
                    else if(i ==2){
                        speed1 += 30;
                    }
                    else if(i == 3){
                        speed1 += 30;
                    }
                }
            }

            text.setString("Score: " + std::to_string(score));
            text1.setString("Lives : " + std::to_string(lives));
            window.draw(text);
            window.draw(text1);
            
            window.display();
          }
          else if(menu)
          {
            window.clear(sf::Color::Black);
           
            window.draw(menuText2);
            window.draw(menuText3);
            window.draw(menuText4);

            window.display();
          }
          else {
            window.clear(sf::Color::Black);

            window.draw(backgroundSprite);

            window.draw(menuText);
            window.draw(menuText1);
            window.display();
        }
    }
    
    return nullptr;
}

int main() 
{
        pthread_t GAMEid;
        sem_init(&MainGame, 0, 1);
        pthread_create(&GAMEid, NULL, GameEngine,NULL);
        void* result;
        pthread_join(GAMEid, &result);

        return 0;
}


