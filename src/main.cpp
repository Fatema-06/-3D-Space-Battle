#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/glut.h>
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>

using namespace std;

// Game Constants
const int WIDTH = 1200;
const int HEIGHT = 700;
const float PI = 3.14159265358979323846f;
const float LASER_SPEED = 15.0f;
const int SPACESHIP_SIZE = 40;
const float TITLE_ROTATION_SPEED = 1.0f;
const int DAMAGE_COOLDOWN = 1000;
const float GAME_OVER_PULSE_SPEED = 0.005f;

// Game States
enum GameState { INTRO, MENU, INSTRUCTIONS, GAME, GAMEOVER };
GameState currentState = INTRO;
bool mousePressed = false;
float mouseX = 0, mouseY = 0;
float titleRotation = 0.0f;
bool explosionActive = false;
float explosionRadius = 0.0f;

// Game Structures
struct Spaceship {
    float x, y, z;
    float angle;
    int health;
    int lastHitTime;
    bool thrusting;
};

struct Laser {
    float x, y, z;
    float dx, dy, dz;
    bool isPlayer1;
};

// Game State
vector<Laser> lasers;
Spaceship player1 = {0, 0, -500, 0, 100, 0};
Spaceship player2 = {0, 0, 500, 180, 100, 0};
float camX = 0, camY = 0, camZ = 1500;

// Prototypes
void initGL();
void updateGame(int);
void drawSpaceship(const Spaceship&, bool);
void drawLaser(const Laser&);
void drawStarfield();
void drawText(float x, float y, const string& text);
void drawStrokeText(float x, float y, float z, const string& text, float scale = 0.2f);
void drawButton(float x1, float y1, float x2, float y2, bool hovered);
void drawExplosion(float x, float y, float z);
void drawIntroScreen();
void drawMenuScreen();
void drawInstructionsScreen();
void drawGameOverScreen();

void initGL(){
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat lightPos[] = {0.0f, 500.0f, 1000.0f, 1.0f};
    GLfloat ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glEnable(GL_COLOR_MATERIAL);
}

// Fallback cylinder implementation if glutSolidCylinder isn't available
void drawCylinder(GLfloat radius, GLfloat height, GLint slices) {
    GLUquadricObj *quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH);
    gluQuadricTexture(quadric, GL_TRUE);
    gluCylinder(quadric, radius, radius, height, slices, 1);
    gluDeleteQuadric(quadric);
}

void drawSpaceship(const Spaceship& ship, bool isPlayer1) {
    glPushMatrix();
    
    // Extreme left/right positioning (-25 and +25)
    float xOffset = isPlayer1 ? -25.0f : 25.0f;
    glTranslatef(xOffset, ship.y, ship.z);  // Removed ship.x to prevent interference
    
    glRotatef(ship.angle, 0, 1, 0);
    
    // Scale down slightly (3.5x)
    glScalef(3.5f, 3.5f, 3.5f);

    // Main hull - more detailed and elongated
    glPushMatrix();
    glColor3f(isPlayer1 ? 0.0f : 1.0f, isPlayer1 ? 0.5f : 0.0f, 0.0f);
    glScalef(1.0f, 0.6f, 2.0f);
    glutSolidSphere(SPACESHIP_SIZE, 50, 50);
    glPopMatrix();

    // Detailed nose cone
    glPushMatrix();
    glTranslatef(0, 0, SPACESHIP_SIZE * 1.5f);
    glColor3f(0.7f, 0.7f, 0.9f);
    glutSolidCone(SPACESHIP_SIZE * 0.6f, SPACESHIP_SIZE * 1.2f, 50, 50);
    glPopMatrix();

    // Wings with more realistic shape
    glPushMatrix();
    glColor3f(0.3f, 0.3f, 0.3f);
    
    // Left wing
    glPushMatrix();
    glTranslatef(-SPACESHIP_SIZE * 1.2f, 0, 0);
    glRotatef(45, 0, 1, 0);
    glScalef(0.5f, 0.1f, 2.0f);
    glutSolidCube(SPACESHIP_SIZE * 2.0f);
    glPopMatrix();
    
    // Right wing
    glPushMatrix();
    glTranslatef(SPACESHIP_SIZE * 1.2f, 0, 0);
    glRotatef(-45, 0, 1, 0);
    glScalef(0.5f, 0.1f, 2.0f);
    glutSolidCube(SPACESHIP_SIZE * 2.0f);
    glPopMatrix();
    
    glPopMatrix();

    // Engine section with multiple thrusters
    glPushMatrix();
    glTranslatef(0, 0, -SPACESHIP_SIZE * 1.5f);
    
    // Main engine housing
    glColor3f(0.2f, 0.2f, 0.2f);
    glScalef(1.0f, 0.8f, 1.2f);
    glutSolidSphere(SPACESHIP_SIZE * 0.8f, 50, 50);
    
    // Thrusters
    glColor3f(0.8f, 0.6f, 0.1f);
    for (int i = 0; i < 3; i++) {
        glPushMatrix();
        glTranslatef((i-1) * SPACESHIP_SIZE * 0.4f, 0, -SPACESHIP_SIZE * 0.6f);
        glutSolidCone(SPACESHIP_SIZE * 0.3f, SPACESHIP_SIZE * 0.6f, 20, 20);
        glPopMatrix();
    }
    glPopMatrix();

    glPopMatrix();
}

void drawLaser(const Laser& laser) {
    glPushMatrix();
    glTranslatef(laser.x, laser.y, laser.z);
    glRotatef(-90, 1, 0, 0);
    glColor3f(1.0f, 0.0f, 0.0f);
    glutSolidCone(3.0f, 50.0f, 10, 10);
    glPopMatrix();
}

void drawStarfield() {
    glDisable(GL_LIGHTING);
    glBegin(GL_POINTS);
    glColor3f(1,1,1);
    for(int i=0; i<1000; i++) {
        glVertex3f(rand()%2000-1000, rand()%2000-1000, rand()%2000-1000);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawText(float x, float y, const string& text) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    
    glRasterPos2f(x, y);
    for(char c : text) 
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawStrokeText(float x, float y, float z, const string& text, float scale) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);
    glLineWidth(3.0f);
    for(char c : text) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
    }
    glPopMatrix();
}


void drawButton(float x1, float y1, float x2, float y2, bool hovered) {
    glDisable(GL_LIGHTING);
    glColor3f(hovered ? 0.2f : 0.1f, 0.1f, hovered ? 0.9f : 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
    
    glColor3f(1,1,1);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawExplosion(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glDisable(GL_LIGHTING);
    glBegin(GL_POINTS);
    for(int i=0; i<500; i++) {
        float angle = 2*PI*rand()/RAND_MAX;
        float dist = explosionRadius * rand()/RAND_MAX;
        glColor3f(1, 0.5, 0);
        glVertex3f(dist*cos(angle), dist*sin(angle), 0);
    }
    glEnd();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawIntroScreen() {
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawStarfield();
    
    glPushMatrix();
    glTranslatef(WIDTH/2, HEIGHT/2 + 100, 0);
    glRotatef(titleRotation, 0, 1, 0);
    glColor3f(0.8f, 0.2f, 0.2f);
    drawStrokeText(-200, 0, 0, "SPACE SHOOTER 3D", 0.3f);
    glPopMatrix();

    glColor3f(0.5, 0.5, 1.0);
    drawText(WIDTH/2-300, HEIGHT-300, "Computer Graphics Project");
    drawText(100, HEIGHT-400, "Developed By:");
    glColor3f(0.8, 0.8, 1.0);
    drawText(100, HEIGHT-450, "Fatema Mohammad, Fatma Alaa, Aya Shaban");
    drawText(100, HEIGHT-500, "Raoda Hafez, Hadeel Mokhtar, Jana Mohamed");
    drawText(100, HEIGHT-550, "Supervised by: Dr. Mohamed El Mowafy");
    
    glColor3f(sin(glutGet(GLUT_ELAPSED_TIME)*0.005f) > 0 ? 1 : 0.5, 0.5, 0.5);
    drawText(WIDTH/2-200, 100, "Press ENTER to continue");
}

void drawMenuScreen() {
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    drawStarfield();
    
    float startY = HEIGHT/2 + 50;
    float instructionsY = HEIGHT/2 - 50;
    float exitY = HEIGHT/2 - 150;

    bool startHovered = (mouseX > WIDTH/2-100 && mouseX < WIDTH/2+100 &&
                       mouseY > HEIGHT - (startY + 100) && mouseY < HEIGHT - startY);
    drawButton(WIDTH/2-100, startY, WIDTH/2+100, startY+100, startHovered);
    glColor3f(1,1,1);
    drawText(WIDTH/2-50, startY+50, "Start Game");

    bool instrHovered = (mouseX > WIDTH/2-100 && mouseX < WIDTH/2+100 &&
                       mouseY > HEIGHT - (instructionsY + 100) && mouseY < HEIGHT - instructionsY);
    drawButton(WIDTH/2-100, instructionsY, WIDTH/2+100, instructionsY+100, instrHovered);
    glColor3f(1,1,1);
    drawText(WIDTH/2-80, instructionsY+50, "Instructions");

    bool exitHovered = (mouseX > WIDTH/2-100 && mouseX < WIDTH/2+100 &&
                      mouseY > HEIGHT - (exitY + 100) && mouseY < HEIGHT - exitY);
    drawButton(WIDTH/2-100, exitY, WIDTH/2+100, exitY+100, exitHovered);
    glColor3f(1,1,1);
    drawText(WIDTH/2-50, exitY+50, "Exit");
}

void drawInstructionsScreen() {
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    drawText(100, HEIGHT-100, "Controls:");
    drawText(100, HEIGHT-150, "Player 1 (Arrow Keys):");
    drawText(100, HEIGHT-200, "W/S - Move");
    drawText(100, HEIGHT-250, "A/D - Rotate");
    drawText(100, HEIGHT-300, "C - Fire Laser");
    
    drawText(WIDTH/2+100, HEIGHT-150, "Player 2:");
    drawText(WIDTH/2+100, HEIGHT-200, "I/K - Move");
    drawText(WIDTH/2+100, HEIGHT-250, "J/L - Rotate");
    drawText(WIDTH/2+100, HEIGHT-300, "M - Fire Laser");
    
    drawText(100, 100, "Press BACKSPACE to return to menu");
}

void drawGameOverScreen() {
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw 3D explosion
    gluLookAt(0, 0, 1000, 0, 0, 0, 0, 1, 0); // Fixed camera position
    if(explosionRadius < 300) explosionRadius += 5.0f;
    drawExplosion(0, 0, (player1.health <= 0) ? -500 : 500);

    // Switch to 2D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_LIGHTING);

    // Game Over text
    float pulse = 0.5f * (1.0f + sin(glutGet(GLUT_ELAPSED_TIME) * GAME_OVER_PULSE_SPEED));
    glColor3f(1.0f, pulse, 0.0f);
    glPushMatrix();
    glTranslatef(WIDTH/2 - 400, HEIGHT - 200, 0);
    drawStrokeText(0, 0, 0, "GAME OVER!", 0.6f + pulse * 0.2f);
    glPopMatrix();

    // Winner text
    string winnerText = (player1.health <= 0) ? "PLAYER 2 VICTORY!" : "PLAYER 1 VICTORY!";
    float winnerPulse = 0.5f * (1.0f + sin(glutGet(GLUT_ELAPSED_TIME) * GAME_OVER_PULSE_SPEED * 1.5f));
    glColor3f(0.2f, 0.8f, 1.0f);
    glPushMatrix();
    glTranslatef(WIDTH/2 - 300, HEIGHT/2 + 100, 0);
    drawStrokeText(0, 0, 0, winnerText, 0.4f + winnerPulse * 0.1f);
    glPopMatrix();


    // Buttons
    bool playAgainHovered = (mouseX > WIDTH/2-150 && mouseX < WIDTH/2+150 &&
        mouseY > HEIGHT/2-50 && mouseY < HEIGHT/2+50);
        bool quitHovered = (mouseX > WIDTH/2-150 && mouseX < WIDTH/2+150 &&
        mouseY > HEIGHT/2-150 && mouseY < HEIGHT/2-50);

        glColor3f(1,1,1);
        drawText(WIDTH/2-60, HEIGHT/2+15, "PLAY AGAIN");

        glColor3f(1,1,1);
        drawText(WIDTH/2-40, HEIGHT/2-100, "QUIT");

    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void displayHUD() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_LIGHTING);

    glColor3f(1,1,1);
    glRasterPos2i(50, HEIGHT-50);
    string text = "Player 1: " + to_string(player1.health);
    for(char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);

    glRasterPos2i(WIDTH-200, HEIGHT-50);
    text = "Player 2: " + to_string(player2.health);
    for(char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);

    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    switch(currentState) {
        case INTRO: drawIntroScreen(); break;
        case MENU: drawMenuScreen(); break;
        case INSTRUCTIONS: drawInstructionsScreen(); break;
        case GAME: {
            gluLookAt(camX, camY, camZ, 0, 0, 0, 0, 1, 0);
            drawStarfield();
            drawSpaceship(player1, true);
            drawSpaceship(player2, false);
            for(const auto& laser : lasers) drawLaser(laser);
            displayHUD();
            break;
        }
        case GAMEOVER: drawGameOverScreen(); break;
    }
    glutSwapBuffers();
}

void updateGame(int value) {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    
    if(currentState == GAME) {
        // Update lasers
        for(auto& laser : lasers) {
            laser.x += laser.dx;
            laser.y += laser.dy;
            laser.z += laser.dz;
        }

        // Remove off-screen lasers
        lasers.erase(remove_if(lasers.begin(), lasers.end(), [](const Laser& l) {
            return abs(l.x) > 2000 || abs(l.y) > 2000 || abs(l.z) > 2000;
        }), lasers.end());

        // Collision detection
        auto checkCollision = [](const Laser& laser, const Spaceship& ship) {
            float dx = laser.x - ship.x;
            float dy = laser.y - ship.y;
            float dz = laser.z - ship.z;
            return sqrt(dx*dx + dy*dy + dz*dz) < SPACESHIP_SIZE*2;
        };

        for(const auto& laser : lasers) {
            if(laser.isPlayer1) {
                if(checkCollision(laser, player2) && (currentTime - player2.lastHitTime) > DAMAGE_COOLDOWN) {
                    player2.health -= 10;
                    player2.lastHitTime = currentTime;
                }
            } else {
                if(checkCollision(laser, player1) && (currentTime - player1.lastHitTime) > DAMAGE_COOLDOWN) {
                    player1.health -= 10;
                    player1.lastHitTime = currentTime;
                }
            }
        }

        if(player1.health <= 0 || player2.health <= 0) {
            currentState = GAMEOVER;
        }
    }

    // Update animations
    if(currentState == INTRO || currentState == MENU) {
        titleRotation += TITLE_ROTATION_SPEED;
        if(titleRotation > 360) titleRotation -= 360;
    }
    
    if(currentState == GAMEOVER && !explosionActive) {
        explosionActive = true;
        explosionRadius = 0.0f;
    } else if(currentState != GAMEOVER) {
        explosionActive = false;
    }
    
    glutPostRedisplay();
    glutTimerFunc(16, updateGame, 0);
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, double(w)/h, 1.0, 5000.0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int, int) {
    const float moveSpeed = 5.0f;
    const float rotateSpeed = 3.0f;

    switch(key) {
        case 13: // Enter
            if(currentState == INTRO) currentState = MENU;
            break;
        case 8: // Backspace
            if(currentState == INSTRUCTIONS) currentState = MENU;
            break;
        case 'w': player1.y += moveSpeed; break;
        case 's': player1.y -= moveSpeed; break;
        case 'a': player1.angle += rotateSpeed; break;
        case 'd': player1.angle -= rotateSpeed; break;
        case 'c': {
            float rad = player1.angle * PI/180;
            lasers.push_back({
                player1.x, player1.y, player1.z,
                LASER_SPEED * sin(rad),
                0,
                LASER_SPEED * cos(rad),
                true
            });
            break;
        }
        case 'i': player2.y += moveSpeed; break;
        case 'k': player2.y -= moveSpeed; break;
        case 'j': player2.angle += rotateSpeed; break;
        case 'l': player2.angle -= rotateSpeed; break;
        case 'm': {
            float rad = player2.angle * PI/180;
            lasers.push_back({
                player2.x, player2.y, player2.z,
                LASER_SPEED * sin(rad),
                0,
                LASER_SPEED * cos(rad),
                false
            });
            break;
        }
        case 27: exit(0);
    }
    glutPostRedisplay();
}

void specialKeys(int key, int, int) {
    const float camSpeed = 20.0f;
    switch(key) {
        case GLUT_KEY_UP: camY += camSpeed; break;
        case GLUT_KEY_DOWN: camY -= camSpeed; break;
        case GLUT_KEY_LEFT: camX -= camSpeed; break;
        case GLUT_KEY_RIGHT: camX += camSpeed; break;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    mouseX = x;
    mouseY = HEIGHT - y;

    if(button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        if(currentState == MENU) {
            if(mouseX > WIDTH/2-100 && mouseX < WIDTH/2+100 &&
               mouseY > HEIGHT/2+50 && mouseY < HEIGHT/2+150) {
                currentState = GAME;
            }
            else if(mouseX > WIDTH/2-100 && mouseX < WIDTH/2+100 &&
                    mouseY > HEIGHT/2-50 && mouseY < HEIGHT/2+50) {
                currentState = INSTRUCTIONS;
            }
            else if(mouseX > WIDTH/2-100 && mouseX < WIDTH/2+100 &&
                    mouseY > HEIGHT/2-150 && mouseY < HEIGHT/2-50) {
                exit(0);
            }
        }
        else if(currentState == GAMEOVER) {
            if(mouseX > WIDTH/2-150 && mouseX < WIDTH/2+150 &&
               mouseY > HEIGHT/2-50 && mouseY < HEIGHT/2+50) {
                // Reset game state
                player1 = {0, 0, -500, 0, 100, 0};
                player2 = {0, 0, 500, 180, 100, 0};
                lasers.clear();
                explosionRadius = 0.0f;
                currentState = GAME;
            }
            else if(mouseX > WIDTH/2-150 && mouseX < WIDTH/2+150 &&
                    mouseY > HEIGHT/2-150 && mouseY < HEIGHT/2-50) {
                exit(0);
            }
        }
    }
    glutPostRedisplay();
}

void motion(int x, int y) {
    mouseX = x;
    mouseY = HEIGHT - y;
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("3D Space Shooter");
    
    initGL();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(motion);
    glutTimerFunc(0, updateGame, 0);
    
    glutMainLoop();
    return 0;
}