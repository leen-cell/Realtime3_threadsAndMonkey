#ifndef MULTITHREADING_H
#define MULTITHREADING_H

#include <pthread.h>
extern int NUM_FATHERS; 

//simulation logic control
extern volatile int simulation_running;

//config values (defined in main.c, read from config file)
extern int FAMILY_NUM;
extern int WITHDRAWN_FAMILIES;      // THRESHOLD
extern int COLLECTED_BANANA;
extern int EATEN_BANANA;
extern int MAX_SIM_TIME;
extern int BANANA_TO_COLLECT;

extern int FEMALE_INITIAL_ENERGY;
extern int MALE_INITIAL_ENERGY;

extern int ENERGY_LOSS_COLLECT;
extern int ENERGY_LOSS_FIGHT;
extern int ENERGY_LOSS_MOVE;
extern int ENERGY_THRESHOLD;

extern int BABY_SLEEP_US;
extern int BABY_STARVATION_THRESHOLD;
extern int BABY_MIN_HUNGER_TO_STEAL;
extern int BABY_HUNGER_RANDOM_MIN;
extern int BABY_HUNGER_RANDOM_MAX;

//global counters (NOT config values)
extern int withdrawn_families_count;  // CHANGED: Counter
extern int max_collected_bananas;
extern int max_eaten_bananas;

extern pthread_mutex_t global_lock;
extern pthread_mutex_t print_lock;


typedef enum {
    SEARCHING,
    RETURNING,
    RESTING,
    DEPOSITING
} FemaleState;

//family structure
typedef struct {
    int id;

    pthread_t male_tid;
    pthread_t female_tid;
    pthread_t baby_tid;

    int active;
    pthread_mutex_t family_lock;

    int basket_bananas;
    pthread_mutex_t basket_lock;
    pthread_mutex_t basket_mutex;  

    int male_energy;
    int female_energy;
    int baby_hunger_level;
    
    int female_row;
    int female_col;
    FemaleState female_state;
    int carried;  
    int entry_row;
    int entry_col;
    
        // ===== Graphics flags =====
    int father_fighting;   // 1 while father is fighting
    int female_fighting;   // 1 while female is fighting
    int baby_action;       // 0=normal, 1=stealing, 2=eating

} Family;

extern Family *families;

/* Controller thread (supervisor) */
void *controller_thread(void *arg);

// Family member threads  
void *baby_thread(void *arg);

// Family Lifecycle Functions  
void create_families();
void start_family_threads();
void join_and_destroy_families();
void print_winning_family();

#endif
