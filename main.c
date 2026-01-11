//girliiies to compile use : gcc -Wall -Wextra main.c maze.c multithreading.c female.c father.c graphics.c -o program -pthread -lGL -lGLU -lglut -lm

//to run just use normal ./program
// :))))))))))

// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "maze.h"
#include <pthread.h>
#include "multithreading.h"
#include "graphics.h"
#include <unistd.h>  

int FAMILY_NUM = 0;
int BANANA_TO_COLLECT = 0;
int WITHDRAWN_FAMILIES = 0;
int COLLECTED_BANANA = 0;
int EATEN_BANANA = 0;
int MAX_SIM_TIME = 0;

int FEMALE_INITIAL_ENERGY = 0;
int MALE_INITIAL_ENERGY = 0;
int ENERGY_LOSS_COLLECT = 0;
int ENERGY_LOSS_FIGHT = 0;
int ENERGY_LOSS_MOVE = 0;
int ENERGY_THRESHOLD = 0;
int BABY_SLEEP_US = 200000;
int BABY_STARVATION_THRESHOLD = 15;
int BABY_MIN_HUNGER_TO_STEAL = 5;
int BABY_HUNGER_RANDOM_MIN = 10;
int BABY_HUNGER_RANDOM_MAX = 25;
int MAZE_ROWS = 0;
int MAZE_COLS = 0;
int MAZE_BANANA_CELLS = 0;
int MAZE_OBSTACLE_CELLS = 0;

char MAZE_FILE[256] = "maze2d.txt";
pthread_mutex_t simulation_mutex = PTHREAD_MUTEX_INITIALIZER; 

void read_config(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("ERROR: Could not open config file '%s'\n", filename);
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n')
            continue;

        char key[64];
        char value[128];

        if (sscanf(line, "%63[^=]=%127s", key, value) != 2)
            continue;

        if (strcmp(key, "FAMILY_NUM") == 0)
            FAMILY_NUM = atoi(value);
        else if (strcmp(key, "BANANA_TO_COLLECT") == 0)
            BANANA_TO_COLLECT = atoi(value);
        else if (strcmp(key, "WITHDRAWN_FAMILIES") == 0)
            WITHDRAWN_FAMILIES = atoi(value);
        else if (strcmp(key, "COLLECTED_BANANA") == 0)
            COLLECTED_BANANA = atoi(value);
        else if (strcmp(key, "EATEN_BANANA") == 0)
            EATEN_BANANA = atoi(value);
        else if (strcmp(key, "MAX_SIM_TIME") == 0)
            MAX_SIM_TIME = atoi(value);
        else if (strcmp(key, "FEMALE_INITIAL_ENERGY") == 0)
            FEMALE_INITIAL_ENERGY = atoi(value);
        else if (strcmp(key, "MALE_INITIAL_ENERGY") == 0)
            MALE_INITIAL_ENERGY = atoi(value);
        else if (strcmp(key, "ENERGY_LOSS_COLLECT") == 0)
            ENERGY_LOSS_COLLECT = atoi(value);
        else if (strcmp(key, "ENERGY_LOSS_FIGHT") == 0)
            ENERGY_LOSS_FIGHT = atoi(value);
        else if (strcmp(key, "ENERGY_THRESHOLD") == 0)
            ENERGY_THRESHOLD = atoi(value);
        else if (strcmp(key, "ENERGY_LOSS_MOVE") == 0)
            ENERGY_LOSS_MOVE = atoi(value);
        else if (strcmp(key, "MAZE_FILE") == 0)
            strncpy(MAZE_FILE, value, sizeof(MAZE_FILE) - 1);
        else if (strcmp(key, "BABY_SLEEP_US") == 0)
            BABY_SLEEP_US = atoi(value);
        else if (strcmp(key, "BABY_STARVATION_THRESHOLD") == 0)
            BABY_STARVATION_THRESHOLD = atoi(value);
        else if (strcmp(key, "BABY_MIN_HUNGER_TO_STEAL") == 0)
            BABY_MIN_HUNGER_TO_STEAL = atoi(value);
        else if (strcmp(key, "BABY_HUNGER_RANDOM_MIN") == 0)
            BABY_HUNGER_RANDOM_MIN = atoi(value);
        else if (strcmp(key, "BABY_HUNGER_RANDOM_MAX") == 0)
            BABY_HUNGER_RANDOM_MAX = atoi(value);
        else if (strcmp(key, "MAZE_ROWS") == 0)
             MAZE_ROWS = atoi(value);
        else if (strcmp(key, "MAZE_COLS") == 0)
            MAZE_COLS = atoi(value);
        else if (strcmp(key, "MAZE_BANANA_CELLS") == 0)
             MAZE_BANANA_CELLS = atoi(value);
        else if (strcmp(key, "MAZE_OBSTACLE_CELLS") == 0)
              MAZE_OBSTACLE_CELLS = atoi(value);
        
    }

    fclose(file);
}

// main function
int main(int argc, char **argv)
{
    srand(time(NULL));
    read_config("config.txt");
    
    printf("========== Configuration ==========\n");
    printf("FAMILY_NUM              = %d\n", FAMILY_NUM);
    printf("BANANA_TO_COLLECT       = %d\n", BANANA_TO_COLLECT);
    printf("WITHDRAWN_FAMILIES      = %d\n", WITHDRAWN_FAMILIES);
    printf("COLLECTED_BANANA        = %d\n", COLLECTED_BANANA);
    printf("EATEN_BANANA            = %d\n", EATEN_BANANA);
    printf("MAX_SIM_TIME            = %d\n", MAX_SIM_TIME);
    printf("\n");
    printf("FEMALE_INITIAL_ENERGY   = %d\n", FEMALE_INITIAL_ENERGY);
    printf("MALE_INITIAL_ENERGY     = %d\n", MALE_INITIAL_ENERGY);
    printf("ENERGY_LOSS_COLLECT     = %d\n", ENERGY_LOSS_COLLECT);
    printf("ENERGY_LOSS_FIGHT       = %d\n", ENERGY_LOSS_FIGHT);
    printf("ENERGY_THRESHOLD        = %d\n", ENERGY_THRESHOLD);
    printf("\n");
    printf("MAZE_FILE               = %s\n", MAZE_FILE);
    printf("==================================\n\n");

    printf("Do you want to create random maze from config or generate random maze from user input?\n");
    printf("1 = config file, 2 = user input: ");
    
    int choice;
    scanf("%d", &choice);

    if (choice == 1) 
         create_maze_random_from_config();
    else if (choice == 2) 
        create_maze_random();
    else {
        printf("Invalid choice, loading default file.\n");
        create_maze_random_from_config();
    }
    
    print_maze();
    
    printf("\n ===== INITIALIZING FAMILIES =====\n");
    create_families();
    start_family_threads();
    
    // Controller
    printf("\n===== STARTING CONTROLLER =====\n");
    pthread_t controller;
    if (pthread_create(&controller, NULL, controller_thread, NULL) != 0) {
        perror("Failed to create controller thread");
        exit(1);
    }

    // FIXED: Graphics MUST run in main thread
    printf("\n===== STARTING GRAPHICS (Main Thread) =====\n");
    init_graphics(argc, argv);
    
    // This blocks until window closes OR simulation ends
    start_graphics();
    
    // After graphics loop exits (window closed or simulation ended)
    printf("\n===== Graphics Loop Exited =====\n");
    
    // Signal all threads to stop if not already stopped
    simulation_running = 0;
    
    // Wait for controller
    pthread_join(controller, NULL);
    
    // Print results
    print_winning_family();
    
    // Cleanup
    join_and_destroy_families();
    free_maze();
    pthread_mutex_destroy(&simulation_mutex);
    
    printf("\n===== SIMULATION COMPLETED SUCCESSFULLY =====\n");
    
    return 0;
}
