#include <stdio.h>
#include <stdlib.h>
#include "maze.h"
#include "multithreading.h"
#include <time.h>

int maze_rows = 0;
int maze_cols = 0;
Cell **maze = NULL;

/*
 * Maze file format:
 * First line: rows cols
 * Then rows x cols integers:
 *   -1 = obstacle
 *    0 = empty
 *   >0 = banana count
 *///later we will create random maze using user's inputs.
void load_maze(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open maze file");
        exit(1);
    }

    if (fscanf(fp, "%d %d", &maze_rows, &maze_cols) != 2) {
        fprintf(stderr, "Invalid maze file format\n");
        exit(1);
    }

    /* Allocate maze */
    maze = malloc(maze_rows * sizeof(Cell *));
    for (int i = 0; i < maze_rows; i++) {
        maze[i] = malloc(maze_cols * sizeof(Cell));
    }

    /* Read cell values */
    for (int i = 0; i < maze_rows; i++) {
        for (int j = 0; j < maze_cols; j++) {
            if (fscanf(fp, "%d", &maze[i][j].value) != 1) {
                fprintf(stderr, "Invalid maze cell at (%d,%d)\n", i, j);
                exit(1);
            }
            maze[i][j].occupied = -1;   
            pthread_mutex_init(&maze[i][j].lock, NULL);
        }
    }

    fclose(fp);
}

/* Print maze for debugging */
void print_maze() {
    printf("Maze (%d x %d):\n", maze_rows, maze_cols);
    for (int i = 0; i < maze_rows; i++) {
        for (int j = 0; j < maze_cols; j++) {
            if (maze[i][j].value == -1)
                printf(" X ");
            else
                printf(" %d ", maze[i][j].value);
        }
        printf("\n");
    }
}

/* Check if cell is obstacle */
int is_obstacle(int r, int c) {
    return maze[r][c].value == -1;
}

/* Take ONE banana if available */
int take_banana(int r, int c) {
    if (r < 0 || r >= maze_rows || c < 0 || c >= maze_cols)
        return 0;

    int taken = 0;
    pthread_mutex_lock(&maze[r][c].lock);
    if (maze[r][c].value > 0) {
        maze[r][c].value--;
        taken = 1;
    }
    pthread_mutex_unlock(&maze[r][c].lock);

    return taken;
}


/* Free maze memory */
void free_maze() {
    for (int i = 0; i < maze_rows; i++) {
        for (int j = 0; j < maze_cols; j++) {
            pthread_mutex_destroy(&maze[i][j].lock);
        }
        free(maze[i]);
    }
    free(maze);
}

void create_maze_random() {
    int rows, cols, banana_cells, obstacle_cells;
    printf("Enter maze size (rows cols): ");
    scanf("%d %d", &rows, &cols);

    printf("Enter number of cells with bananas: ");
    scanf("%d", &banana_cells);
    if (banana_cells > rows * cols) banana_cells = rows * cols;

    printf("Enter number of cells with obstacles: ");
    scanf("%d", &obstacle_cells);
    if (obstacle_cells + banana_cells > rows * cols) 
        obstacle_cells = rows * cols - banana_cells;

    srand(time(NULL));
    maze_allocate(rows, cols);

    // fill everything with empty
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            maze_set_cell(i, j, 0);

    // place obstacles (-1)
    int placed = 0;
    while (placed < obstacle_cells) {
        int r = rand() % rows;
        int c = rand() % cols;
        if (maze_get_cell(r, c) == 0) {
            maze_set_cell(r, c, -1);  // obstacle
            placed++;
        }
    }

    // place bananas (>0)
    placed = 0;
    while (placed < banana_cells) {
        int r = rand() % rows;
        int c = rand() % cols;
        if (maze_get_cell(r, c) == 0) {
            int banana_count = 1 + rand() % 5; // 1 to 5 bananas
            maze_set_cell(r, c, banana_count);
            placed++;
        }
    }

    printf("Random maze created!\n");
}

void maze_allocate(int rows, int cols) {
    maze_rows = rows;
    maze_cols = cols;

    maze = (Cell **)malloc(rows * sizeof(Cell *));
    if (!maze) { printf("Error allocating maze rows\n"); exit(1); }

    for (int i = 0; i < rows; i++) {
        maze[i] = (Cell *)malloc(cols * sizeof(Cell));
        if (!maze[i]) { printf("Error allocating maze columns\n"); exit(1); }
        for (int j = 0; j < cols; j++) {
            maze[i][j].value = 0;
            maze[i][j].occupied = -1;  
            pthread_mutex_init(&maze[i][j].lock, NULL);
        }
    }
}
void maze_set_cell(int row, int col, int value) {
    if (row >= 0 && row < maze_rows && col >= 0 && col < maze_cols) {
        maze[row][col].value = value;  // can be -1, 0, >0
    }
}

int maze_get_cell(int row, int col) {
    if (row >= 0 && row < maze_rows && col >= 0 && col < maze_cols) {
        return maze[row][col].value;
    }
    return -2; // invalid
}



int valid_cell(int r, int c)
{
    if (r < 0 || r >= maze_rows || c < 0 || c >= maze_cols)
        return 0;
    if (maze[r][c].value == -1)
        return 0;
    return 1;
}

void create_maze_random_from_config(void)
{
    extern int MAZE_ROWS;
    extern int MAZE_COLS;
    extern int MAZE_BANANA_CELLS;
    extern int MAZE_OBSTACLE_CELLS;

    if (MAZE_ROWS <= 0 || MAZE_COLS <= 0) {
        printf("ERROR: Invalid maze size in config\n");
        exit(1);
    }

    int total_cells = MAZE_ROWS * MAZE_COLS;

    int banana_cells = MAZE_BANANA_CELLS;
    int obstacle_cells = MAZE_OBSTACLE_CELLS;

    if (banana_cells > total_cells)
        banana_cells = total_cells;

    if (banana_cells + obstacle_cells > total_cells)
        obstacle_cells = total_cells - banana_cells;

    maze_allocate(MAZE_ROWS, MAZE_COLS);

    /* Initialize all cells as empty */
    for (int i = 0; i < MAZE_ROWS; i++) {
        for (int j = 0; j < MAZE_COLS; j++) {
            maze[i][j].value = 0;
            maze[i][j].occupied = -1;
        }
    }

    /* Place obstacles */
    int placed = 0;
    while (placed < obstacle_cells) {
        int r = rand() % MAZE_ROWS;
        int c = rand() % MAZE_COLS;
        if (maze[r][c].value == 0) {
            maze[r][c].value = -1;
            placed++;
        }
    }

    /* Place bananas */
    placed = 0;
    while (placed < banana_cells) {
        int r = rand() % MAZE_ROWS;
        int c = rand() % MAZE_COLS;
        if (maze[r][c].value == 0) {
            maze[r][c].value = 1 + rand() % 5;
            placed++;
        }
    }

    printf("Random maze generated from config (%dx%d)\n",
           MAZE_ROWS, MAZE_COLS);
}




