#include "graphics.h"
#include <GL/glut.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

// Window dimensions
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 900
#define INFO_PANEL_WIDTH 300
#define BASKET_PANEL_HEIGHT 150
//to slow down graphics update
#define GRAPHICS_DELAY_MS 300  

// Colors
#define COLOR_OBSTACLE 0.3f, 0.3f, 0.3f
#define COLOR_EMPTY 0.9f, 0.9f, 0.9f
#define COLOR_BANANA 1.0f, 0.84f, 0.0f
#define COLOR_FEMALE 1.0f, 0.4f, 0.7f      // Pink
#define COLOR_MALE 0.2f, 0.4f, 0.8f        // Blue
#define COLOR_BABY 0.5f, 0.9f, 0.5f        // Light green
#define COLOR_BASKET 0.6f, 0.3f, 0.1f      // Brown
#define COLOR_FIGHT 1.0f, 0.0f, 0.0f          // Red (fight)
#define COLOR_BABY_STEAL 1.0f, 0.6f, 0.0f     // Orange (steal)
#define COLOR_BABY_EAT 0.6f, 0.2f, 0.9f       // Purple (eat)


extern volatile int simulation_running;
extern Family *families;
extern int FAMILY_NUM;
extern Cell **maze;
extern int maze_rows;
extern int maze_cols;

// Grid cell size calculation
float cell_width;
float cell_height;
float maze_display_width;
float maze_display_height;

static int simulation_ended = 0;
static int end_countdown = 50;
static int winner_id = -1;
static int winner_bananas = -1;
static int winner_done = 0;





void calculate_cell_size() {
    maze_display_width = WINDOW_WIDTH - INFO_PANEL_WIDTH - 40;
    maze_display_height = WINDOW_HEIGHT - BASKET_PANEL_HEIGHT - 40;
    
    cell_width = maze_display_width / maze_cols;
    cell_height = maze_display_height / maze_rows;
    
    // Keep cells square
    if (cell_width > cell_height) {
        cell_width = cell_height;
    } else {
        cell_height = cell_width;
    }
}

void draw_text(float x, float y, const char* text, void* font) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(font, *text);
        text++;
    }
}

void draw_circle(float cx, float cy, float radius, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cx + radius * cos(angle);
        float y = cy + radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

void draw_triangle(float cx, float cy, float size) {
    glBegin(GL_TRIANGLES);
    glVertex2f(cx, cy + size);           // Top
    glVertex2f(cx - size, cy - size);    // Bottom left
    glVertex2f(cx + size, cy - size);    // Bottom right
    glEnd();
}

void draw_square(float cx, float cy, float size) {
    glBegin(GL_QUADS);
    glVertex2f(cx - size, cy - size);
    glVertex2f(cx + size, cy - size);
    glVertex2f(cx + size, cy + size);
    glVertex2f(cx - size, cy + size);
    glEnd();
}

void draw_basket(float x, float y, float width, float height, int bananas) {
    // Basket body
    glColor3f(COLOR_BASKET);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width * 0.9f, y + height);
    glVertex2f(x + width * 0.1f, y + height);
    glEnd();
    
    // Basket outline
    glColor3f(0.3f, 0.15f, 0.05f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width * 0.9f, y + height);
    glVertex2f(x + width * 0.1f, y + height);
    glEnd();
    
    // Banana count
    glColor3f(1.0f, 1.0f, 1.0f);
    char text[10];
    sprintf(text, "%d", bananas);
    draw_text(x + width/2 - 10, y + height/2 + 5, text, GLUT_BITMAP_HELVETICA_18);
    
    // Draw some bananas if count > 0
    if (bananas > 0) {
        int display_bananas = bananas < 5 ? bananas : 5;
        for (int i = 0; i < display_bananas; i++) {
            float bx = x + width * 0.2f + (i * width * 0.15f);
            float by = y + height * 0.7f;
            glColor3f(COLOR_BANANA);
            draw_circle(bx, by, 3, 8);
        }
    }
}

void draw_maze() {
    float maze_offset_y = BASKET_PANEL_HEIGHT + 20;
    
    for (int r = 0; r < maze_rows; r++) {
        for (int c = 0; c < maze_cols; c++) {
            float x = 20 + c * cell_width;
            float y = maze_offset_y + r * cell_height;
            
            pthread_mutex_lock(&maze[r][c].lock);
            int value = maze[r][c].value;
            int occupied = maze[r][c].occupied;
            pthread_mutex_unlock(&maze[r][c].lock);
            
            // Draw cell background
            if (value == -1) {
                glColor3f(COLOR_OBSTACLE);
            } else if (value > 0) {
                float intensity = 0.5f + (value * 0.1f);
                if (intensity > 1.0f) intensity = 1.0f;
                glColor3f(intensity, intensity * 0.84f, 0.0f);
            } else {
                glColor3f(COLOR_EMPTY);
            }
            
            glBegin(GL_QUADS);
            glVertex2f(x, y);
            glVertex2f(x + cell_width, y);
            glVertex2f(x + cell_width, y + cell_height);
            glVertex2f(x, y + cell_height);
            glEnd();
            
            // Draw grid lines
            glColor3f(0.5f, 0.5f, 0.5f);
            glLineWidth(1.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(x, y);
            glVertex2f(x + cell_width, y);
            glVertex2f(x + cell_width, y + cell_height);
            glVertex2f(x, y + cell_height);
            glEnd();
            
            // Draw banana count
            if (value > 0) {
                char text[10];
                sprintf(text, "%d", value);
                glColor3f(0.0f, 0.0f, 0.0f);
                draw_text(x + cell_width/2 - 5, y + cell_height/2 + 5, 
                         text, GLUT_BITMAP_HELVETICA_12);
            }
            
            // Draw female if present
            if (occupied >= 0 && occupied < FAMILY_NUM) {
                pthread_mutex_lock(&families[occupied].family_lock);
                if (families[occupied].female_row == r && 
                    families[occupied].female_col == c) {
                    
// Female (circle)
if (families[occupied].female_fighting) glColor3f(COLOR_FIGHT);
else glColor3f(COLOR_FEMALE);

draw_circle(x + cell_width/2, y + cell_height/2, 
           cell_width * 0.25f, 20);

                    
                    // Female outline
                    glColor3f(0.8f, 0.2f, 0.5f);
                    glLineWidth(2.0f);
                    glBegin(GL_LINE_LOOP);
                    for (int i = 0; i <= 20; i++) {
                        float angle = 2.0f * M_PI * i / 20;
                        float px = x + cell_width/2 + (cell_width * 0.25f) * cos(angle);
                        float py = y + cell_height/2 + (cell_width * 0.25f) * sin(angle);
                        glVertex2f(px, py);
                    }
                    glEnd();
                    
                    // Draw carried bananas indicator
                    int carried = families[occupied].carried;
                    if (carried > 0) {
                        glColor3f(1.0f, 1.0f, 1.0f);
                        char text[5];
                        sprintf(text, "%d", carried);
                        draw_text(x + cell_width/2 - 3, y + cell_height/2 + 3,
                                 text, GLUT_BITMAP_HELVETICA_12);
                    }
                }
                pthread_mutex_unlock(&families[occupied].family_lock);
            }
        }
    }
}

void draw_baskets_panel() {
    // Background
    glColor3f(0.85f, 0.85f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH - INFO_PANEL_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH - INFO_PANEL_WIDTH, BASKET_PANEL_HEIGHT);
    glVertex2f(0, BASKET_PANEL_HEIGHT);
    glEnd();
    
    // Title
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text(20, BASKET_PANEL_HEIGHT - 20, 
             "FAMILY BASKETS & MEMBERS", GLUT_BITMAP_HELVETICA_18);
    
    // Draw each family's basket and members
    float basket_width = (WINDOW_WIDTH - INFO_PANEL_WIDTH - 40) / FAMILY_NUM;
    
    for (int i = 0; i < FAMILY_NUM; i++) {
        float x = 20 + i * basket_width;
        float y = 20;
        
        pthread_mutex_lock(&families[i].family_lock);
        pthread_mutex_lock(&families[i].basket_lock);
        
        int active = families[i].active;
        int bananas = families[i].basket_bananas;
        int male_energy = families[i].male_energy;
        int female_energy = families[i].female_energy;
        int baby_hunger = families[i].baby_hunger_level;
        
        pthread_mutex_unlock(&families[i].basket_lock);
        pthread_mutex_unlock(&families[i].family_lock);
        
        // Family label
        char label[20];
        sprintf(label, "F%d", i);
        glColor3f(0.0f, 0.0f, 0.0f);
        draw_text(x + 5, BASKET_PANEL_HEIGHT - 40, label, GLUT_BITMAP_HELVETICA_12);
        
        // Basket
        draw_basket(x + 5, y, basket_width - 50, 60, bananas);
        
        // Father (square) - next to basket
        float member_x = x + basket_width - 40;
        float member_y = y + 50;
        
if (active) {
    if (families[i].father_fighting) glColor3f(COLOR_FIGHT);
    else glColor3f(COLOR_MALE);
} else {
    glColor3f(0.5f, 0.5f, 0.5f); // Gray if withdrawn
}
draw_square(member_x, member_y, 8);

        
        // Father energy bar
        glColor3f(1.0f, 0.0f, 0.0f);
        float energy_width = (male_energy / 100.0f) * 16;
        glBegin(GL_QUADS);
        glVertex2f(member_x - 8, member_y - 12);
        glVertex2f(member_x - 8 + energy_width, member_y - 12);
        glVertex2f(member_x - 8 + energy_width, member_y - 10);
        glVertex2f(member_x - 8, member_y - 10);
        glEnd();
        
        // Mother (circle) - below father
        member_y -= 20;
if (active) {
    if (families[i].female_fighting) glColor3f(COLOR_FIGHT);
    else glColor3f(COLOR_FEMALE);
} else {
    glColor3f(0.5f, 0.5f, 0.5f);
}
draw_circle(member_x, member_y, 6, 12);

        
        // Mother energy bar
        glColor3f(1.0f, 0.5f, 0.0f);
        energy_width = (female_energy / 100.0f) * 16;
        glBegin(GL_QUADS);
        glVertex2f(member_x - 8, member_y - 10);
        glVertex2f(member_x - 8 + energy_width, member_y - 10);
        glVertex2f(member_x - 8 + energy_width, member_y - 8);
        glVertex2f(member_x - 8, member_y - 8);
        glEnd();
        
        // Baby (triangle) - below mother
        member_y -= 20;
if (active) {
    if (families[i].baby_action == 1) glColor3f(COLOR_BABY_STEAL);
    else if (families[i].baby_action == 2) glColor3f(COLOR_BABY_EAT);
    else glColor3f(COLOR_BABY);
} else {
    glColor3f(0.5f, 0.5f, 0.5f);
}
draw_triangle(member_x, member_y, 6);

        
        // Baby hunger indicator
        if (baby_hunger > 10) {
            glColor3f(1.0f, 0.0f, 0.0f);
            draw_circle(member_x + 10, member_y + 5, 3, 8);
        }
        
        // Divider line
        if (i < FAMILY_NUM - 1) {
            glColor3f(0.6f, 0.6f, 0.6f);
            glLineWidth(1.0f);
            glBegin(GL_LINES);
            glVertex2f(x + basket_width, 0);
            glVertex2f(x + basket_width, BASKET_PANEL_HEIGHT);
            glEnd();
        }
    }
}

void draw_info_panel() {
    float panel_x = WINDOW_WIDTH - INFO_PANEL_WIDTH;
    
    // Background
    glColor3f(0.95f, 0.95f, 0.95f);
    glBegin(GL_QUADS);
    glVertex2f(panel_x, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
    glVertex2f(panel_x, WINDOW_HEIGHT);
    glEnd();
    
    // Title
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text(panel_x + 10, WINDOW_HEIGHT - 30, 
             "SIMULATION INFO", GLUT_BITMAP_HELVETICA_18);
    // WINNER display (after simulation ends)
if (!simulation_running && winner_done && winner_id >= 0) {
    glColor3f(0.0f, 0.0f, 0.8f);
    draw_text(panel_x + 10, WINDOW_HEIGHT - 60, "WINNER FAMILY:", GLUT_BITMAP_HELVETICA_12);

    char wtxt[64];
    glColor3f(0.0f, 0.0f, 0.0f);
    sprintf(wtxt, "ID: %d", winner_id);
    draw_text(panel_x + 10, WINDOW_HEIGHT - 80, wtxt, GLUT_BITMAP_HELVETICA_12);

    sprintf(wtxt, "Bananas: %d", winner_bananas);
    draw_text(panel_x + 10, WINDOW_HEIGHT - 100, wtxt, GLUT_BITMAP_HELVETICA_12);
}

    float y = WINDOW_HEIGHT - 120;
    char text[100];
    
    // Global stats
    glColor3f(0.0f, 0.0f, 0.0f);
    
    int active_families = 0;
    int total_bananas = 0;
    for (int i = 0; i < FAMILY_NUM; i++) {
        pthread_mutex_lock(&families[i].family_lock);
        pthread_mutex_lock(&families[i].basket_lock);
        if (families[i].active) active_families++;
        total_bananas += families[i].basket_bananas;
        pthread_mutex_unlock(&families[i].basket_lock);
        pthread_mutex_unlock(&families[i].family_lock);
    }
    
    sprintf(text, "Active Families: %d/%d", active_families, FAMILY_NUM);
    draw_text(panel_x + 10, y, text, GLUT_BITMAP_HELVETICA_12);
    y -= 20;
    
    sprintf(text, "Total Bananas: %d", total_bananas);
    draw_text(panel_x + 10, y, text, GLUT_BITMAP_HELVETICA_12);
    y -= 30;
    
    // Family details
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text(panel_x + 10, y, "FAMILY DETAILS:", GLUT_BITMAP_HELVETICA_12);
    y -= 25;
    
    for (int i = 0; i < FAMILY_NUM; i++) {
        pthread_mutex_lock(&families[i].family_lock);
        pthread_mutex_lock(&families[i].basket_lock);
        
        int active = families[i].active;
        int bananas = families[i].basket_bananas;
        int male_energy = families[i].male_energy;
        int female_energy = families[i].female_energy;
        int carried = families[i].carried;
        
        pthread_mutex_unlock(&families[i].basket_lock);
        pthread_mutex_unlock(&families[i].family_lock);
        
        // Family header
        if (active) {
            glColor3f(0.0f, 0.5f, 0.0f);
        } else {
            glColor3f(0.7f, 0.0f, 0.0f);
        }
        
        sprintf(text, "F%d: %s", i, active ? "ACTIVE" : "OUT");
        draw_text(panel_x + 10, y, text, GLUT_BITMAP_9_BY_15);
        y -= 18;
        
        if (y < 200) break; // Don't overflow panel
        
        // Details
        glColor3f(0.0f, 0.0f, 0.0f);
        sprintf(text, "  B:%d FE:%d ME:%d C:%d", 
                bananas, female_energy, male_energy, carried);
        draw_text(panel_x + 15, y, text, GLUT_BITMAP_HELVETICA_10);
        y -= 18;
        
        if (y < 200) break;
    }
    
    // Legend
    y = 180;
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text(panel_x + 10, y, "LEGEND:", GLUT_BITMAP_HELVETICA_12);
    y -= 20;
    
    // Father
    glColor3f(COLOR_MALE);
    draw_square(panel_x + 20, y + 5, 5);
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text(panel_x + 30, y, "Father", GLUT_BITMAP_HELVETICA_10);
    y -= 15;
    
    // Mother
    glColor3f(COLOR_FEMALE);
    draw_circle(panel_x + 20, y + 5, 5, 10);
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text(panel_x + 30, y, "Mother", GLUT_BITMAP_HELVETICA_10);
    y -= 15;
    
    // Baby
    glColor3f(COLOR_BABY);
    draw_triangle(panel_x + 20, y + 5, 5);
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text(panel_x + 30, y, "Baby", GLUT_BITMAP_HELVETICA_10);
    y -= 20;
    
    // Obstacle
    glColor3f(COLOR_OBSTACLE);
    glBegin(GL_QUADS);
    glVertex2f(panel_x + 15, y);
    glVertex2f(panel_x + 25, y);
    glVertex2f(panel_x + 25, y + 10);
    glVertex2f(panel_x + 15, y + 10);
    glEnd();
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text(panel_x + 30, y, "Obstacle", GLUT_BITMAP_HELVETICA_10);
    y -= 15;
    
    // Banana
    glColor3f(COLOR_BANANA);
    glBegin(GL_QUADS);
    glVertex2f(panel_x + 15, y);
    glVertex2f(panel_x + 25, y);
    glVertex2f(panel_x + 25, y + 10);
    glVertex2f(panel_x + 15, y + 10);
    glEnd();
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text(panel_x + 30, y, "Bananas", GLUT_BITMAP_HELVETICA_10);
    
    // Controls
    y = 40;
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text(panel_x + 10, y, "Press 'Q' to quit", GLUT_BITMAP_HELVETICA_10);
}

void graphics_display() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    draw_baskets_panel();  // Bottom panel with baskets
    draw_maze();           // Main maze area
    draw_info_panel();     // Right panel with stats
    
    glutSwapBuffers();
}

// void graphics_timer(int value) {
//     if (simulation_running) {
//         glutPostRedisplay();
//         glutTimerFunc(100, graphics_timer, 0);
//     } else {
//         glutPostRedisplay();
//     }
// }

void graphics_timer(int value) {
    glutPostRedisplay();
    
    if (simulation_running) {
        // Simulation still running - keep updating
        glutTimerFunc(GRAPHICS_DELAY_MS, graphics_timer, 0);
    } else {
        // Simulation ended
        if (!simulation_ended) {
            simulation_ended = 1;
            printf("\n[Graphics] Simulation ended \n");
        }
         compute_winner_once();
        // Keep window alive and still redraw sometimes
        glutTimerFunc(GRAPHICS_DELAY_MS, graphics_timer, 0);
    }
}

void graphics_keyboard(unsigned char key, int x, int y) {
    if (key == 'q' || key == 'Q' || key == 27) {
        simulation_running = 0;
    }
}

void init_graphics(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("Ape Banana Collection Simulation");
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    
    calculate_cell_size();
    
    glutDisplayFunc(graphics_display);
    glutKeyboardFunc(graphics_keyboard);
    glutTimerFunc(GRAPHICS_DELAY_MS, graphics_timer, 0);
}

void start_graphics() {
    glutMainLoop();
}

void cleanup_graphics() {
    // Nothing special needed
}

static void compute_winner_once(void)
{
    if (winner_done) return;

    int best_id = -1;
    int best_bananas = -1;

    for (int i = 0; i < FAMILY_NUM; i++) {
        pthread_mutex_lock(&families[i].basket_lock);
        int b = families[i].basket_bananas;
        pthread_mutex_unlock(&families[i].basket_lock);

        if (b > best_bananas) {
            best_bananas = b;
            best_id = i; // index == family id in your drawing labels
        }
    }

    winner_id = best_id;
    winner_bananas = best_bananas;
    winner_done = 1;
}
