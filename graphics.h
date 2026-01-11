
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "multithreading.h"
#include "maze.h"

// Initialize OpenGL window
void init_graphics(int argc, char** argv);

// Start the graphics loop (blocks until window closes)
void start_graphics();

// Cleanup graphics resources
void cleanup_graphics();

// Callback to update simulation state
void graphics_display();
void graphics_timer(int value);
void graphics_keyboard(unsigned char key, int x, int y);
static void compute_winner_once(void);

#endif // GRAPHICS_H