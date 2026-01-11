#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>  
#include "multithreading.h"
#include "maze.h"
#include "father.h"
#include "female.h"

#define ENTRY_COL 0

volatile int simulation_running = 1;

/* Families */
Family *families = NULL;

//global counters
int withdrawn_families_count = 0;  // CHANGED: Renamed
int max_collected_bananas = 0;
int max_eaten_bananas = 0;

//this lock is used to protect global counters
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;


void *controller_thread(void *arg) {
    time_t start_time = time(NULL);
    while (simulation_running) {
        usleep(500000);
        
        pthread_mutex_lock(&global_lock);
        
        //1. check withdrawn families
        if (withdrawn_families_count >= WITHDRAWN_FAMILIES) {
            safe_print("[Controller] Terminating: withdrawn families limit reached (%d/%d)\n",
                   withdrawn_families_count, WITHDRAWN_FAMILIES);
            simulation_running = 0;
        }
        //2. check max collected bananas
        if (max_collected_bananas >= COLLECTED_BANANA) {
            safe_print("[Controller] Terminating: max collected bananas limit reached\n");
            simulation_running = 0;
        }
        //3. check max eaten bananas
        if (max_eaten_bananas >= EATEN_BANANA) {
            safe_print("[Controller] Terminating: max eaten bananas limit reached\n");
            simulation_running = 0;
        }
        
        pthread_mutex_unlock(&global_lock);
        
        //4. check time limit
        if (difftime(time(NULL), start_time) >= MAX_SIM_TIME) {
            safe_print("[Controller] Terminating: time limit reached\n");
            simulation_running = 0;
        }
    }
    safe_print("[Controller] Simulation ended.\n");
    return NULL;
}


void create_families()
{
    families = malloc(FAMILY_NUM * sizeof(Family));
    if (!families) {
        perror("Failed to allocate families");
        exit(1);
    }
     
    for (int i = 0; i < FAMILY_NUM; i++) {
        families[i].id = i;
        families[i].active = 1;
        families[i].father_fighting = 0;
        families[i].female_fighting = 0;
        families[i].baby_action = 0;

        pthread_mutex_init(&families[i].family_lock, NULL);

        families[i].basket_bananas = 0;
        pthread_mutex_init(&families[i].basket_lock, NULL);
        pthread_mutex_init(&families[i].basket_mutex, NULL);  

        families[i].male_energy = MALE_INITIAL_ENERGY;
        families[i].female_energy = FEMALE_INITIAL_ENERGY;
        families[i].baby_hunger_level = 0;
        families[i].female_row = families[i].id % maze_rows;
        families[i].female_col = 0;
        
        safe_print("[Init] Family %d: Father initialized with %d energy, starts at (%d,%d)\n",
               i, MALE_INITIAL_ENERGY, families[i].female_row, families[i].female_col);
    }
    
    safe_print("[Main] %d families created\n", FAMILY_NUM);
}

void start_family_threads()
{
    for (int i = 0; i < FAMILY_NUM; i++) {
        // Pass &families[i] now
        if (pthread_create(&families[i].male_tid, NULL, father_thread, &families[i]) != 0) {
            perror("Failed to create father thread");
            exit(1);
        }

        pthread_create(&families[i].female_tid, NULL, female_thread, &families[i]);
        pthread_create(&families[i].baby_tid, NULL, baby_thread, &families[i]);
    }

    safe_print("[Main] All family threads started\n");
}
void join_and_destroy_families()
{
    for (int i = 0; i < FAMILY_NUM; i++) {
        pthread_join(families[i].male_tid, NULL);
        pthread_join(families[i].female_tid, NULL);
        pthread_join(families[i].baby_tid, NULL);

        pthread_mutex_destroy(&families[i].family_lock);
        pthread_mutex_destroy(&families[i].basket_lock);
        
        // ADDED: Destroy father mutexes
        //pthread_mutex_destroy(&fathers[i].basket_mutex);
    }

    free(families);
    families = NULL;

    safe_print("[Main] All families cleaned up\n");
}

void print_winning_family()
{
    int max_bananas = -1;
    for (int i = 0; i < FAMILY_NUM; i++) {
        pthread_mutex_lock(&families[i].basket_lock);
        int bananas = families[i].basket_bananas;
        pthread_mutex_unlock(&families[i].basket_lock);

        if (bananas > max_bananas) {
            max_bananas = bananas;
        }
    }
    
    if (max_bananas <= 0) {
        safe_print("\n===== SIMULATION RESULT =====\n");
        safe_print("No winning family.\n");
        safe_print("=============================\n");
        return;
    }

    safe_print("\n===== SIMULATION RESULT =====\n");
    safe_print("Winning banana count: %d\n", max_bananas);
    safe_print("Winning family ID(s): ");

    int first = 1;
    for (int i = 0; i < FAMILY_NUM; i++) {
        pthread_mutex_lock(&families[i].basket_lock);
        int bananas = families[i].basket_bananas;
        pthread_mutex_unlock(&families[i].basket_lock);

        if (bananas == max_bananas) {
            if (!first) safe_print(", ");
            safe_print("%d", i);
            first = 0;
        }
    }
    safe_print("\n=============================\n");
}
void *baby_thread(void *arg)
{
    Family *fam = (Family *)arg;

    safe_print("[Baby %d] started - waiting for fight opportunities to steal bananas!\n", fam->id);

    while (simulation_running && fam->active) {
        usleep(BABY_SLEEP_US);

        // // ADDED: Check if family is still active
        // if (!is_family_active_father(fam->id)) {
        //     pthread_mutex_lock(&fam->family_lock);
        //     fam->active = 0;
        //     pthread_mutex_unlock(&fam->family_lock);
        //     break;
        // }

        pthread_mutex_lock(&fam->family_lock);
        fam->baby_hunger_level++;
        int hunger = fam->baby_hunger_level;
        pthread_mutex_unlock(&fam->family_lock);

        if (hunger < BABY_MIN_HUNGER_TO_STEAL) {
            continue;
        }

        int target_family = rand() % FAMILY_NUM;
        int steal_amount = 1 + (rand() % 3);
        int stolen = baby_steal_attempt_father(target_family, steal_amount);

        if (stolen > 0) {
            pthread_mutex_lock(&fam->family_lock);
            fam->baby_action = 1; // stealing
            pthread_mutex_unlock(&fam->family_lock);

            int threshold = BABY_HUNGER_RANDOM_MIN +
                            (rand() % (BABY_HUNGER_RANDOM_MAX - BABY_HUNGER_RANDOM_MIN + 1));

            if (hunger > threshold) {
                pthread_mutex_lock(&fam->family_lock);
                fam->baby_action = 2; // eating
                pthread_mutex_unlock(&fam->family_lock);

                pthread_mutex_lock(&global_lock);
                max_eaten_bananas += stolen;
                pthread_mutex_unlock(&global_lock);

                safe_print("[Baby %d] TOO HUNGRY (%d > %d) → ATE %d bananas (stolen from family %d)\n",
                       fam->id, hunger, threshold, stolen, target_family);
            } else {
                add_to_basket(fam->id, stolen);

                safe_print("[Baby %d] hunger %d ≤ %d → added %d stolen bananas to dad's basket (from family %d)\n",
                       fam->id, hunger, threshold, stolen, target_family);
            }

            pthread_mutex_lock(&fam->family_lock);
            fam->baby_hunger_level = (fam->baby_hunger_level >= 10) ?
                                     fam->baby_hunger_level - 10 : 0;
            pthread_mutex_unlock(&fam->family_lock);
            usleep(200000); // OPTIONAL: visible

            pthread_mutex_lock(&fam->family_lock);
            fam->baby_action = 0; // back to normal
            pthread_mutex_unlock(&fam->family_lock);

        }
    }

    safe_print("[Baby %d] exiting\n", fam->id);
    return NULL;
}
void safe_print(const char *format, ...) {
    pthread_mutex_lock(&print_lock);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    pthread_mutex_unlock(&print_lock);
}
