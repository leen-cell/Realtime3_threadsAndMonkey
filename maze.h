#ifndef MAZE_H
#define MAZE_H

#include <pthread.h>

/*
 * Cell value meaning:
 *  -1 : obstacle
 *   0 : empty
 *  >0 : number of bananas
 */
typedef struct {
    int value;
    int occupied;
    pthread_mutex_t lock; //to put a lock on the cell
} Cell;

/* Maze dimensions */
extern int maze_rows;
extern int maze_cols;

// Maze grid 
extern Cell **maze;
int is_obstacle(int r, int c);
int take_banana(int r, int c);

void load_maze(const char *filename);
void print_maze();
void add_banana(int r, int c);
void free_maze();// clean up
void maze_allocate(int rows, int cols);// allocate the maze memory
void maze_set_cell(int row, int col, int value);// set value of a cell
void load_maze(const char *filename);// load maze from file
void shuffle_dirs(int dirs[4][2]);
int valid_cell(int r, int c);

void create_maze_random();
int maze_get_cell(int row, int col);

void create_maze_random_from_config(void);


#endif

