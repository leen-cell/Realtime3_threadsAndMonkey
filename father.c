#include "father.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "multithreading.h"  

// External globals from main.c
extern int ENERGY_LOSS_FIGHT;
extern int ENERGY_THRESHOLD;
extern pthread_mutex_t simulation_mutex;
extern int WITHDRAWN_FAMILIES;
extern volatile int simulation_running;
extern int FAMILY_NUM;
extern int withdrawn_families_count;

// Define the global fathers array here
extern Family *families;
//father_t fathers[FAMILY_NUM];
//int NUM_FATHERS = 0;



void *father_thread(void *arg)
{
    Family *fam = (Family *)arg;
    
    safe_print("Father %d started protecting basket\n", fam->id);
    
    while (simulation_running && fam->active) {
        usleep(500000); // 0.5 seconds
        
        // Check if should fight with neighbor
        if (should_fight_father(fam)) {
            int neighbor_id = pick_neighbor_father(fam->id);
            if (neighbor_id >= 0 && families[neighbor_id].active) {
            // FLAG ON (so graphics draws red)
        pthread_mutex_lock(&fam->family_lock);
        fam->father_fighting = 1;
        pthread_mutex_unlock(&fam->family_lock);

        fight_with_neighbor_father(fam, neighbor_id);

        // make red visible
            usleep(200000);

        // FLAG OFF
        pthread_mutex_lock(&fam->family_lock);
            fam->father_fighting = 0;
        pthread_mutex_unlock(&fam->family_lock);

fam->male_energy -= ENERGY_LOSS_FIGHT;

                safe_print("Father %d fought! Energy now: %d, Bananas: %d\n", 
                       fam->id, fam->male_energy, fam->basket_bananas);
            }
        }
        
        // Passive energy loss
        pthread_mutex_lock(&fam->family_lock);
        fam->male_energy -= 1;
        int energy = fam->male_energy;
        pthread_mutex_unlock(&fam->family_lock);
        
        // Check withdrawal condition
        if (energy <= ENERGY_THRESHOLD) {
            pthread_mutex_lock(&fam->family_lock);
            fam->active = 0;
            pthread_mutex_unlock(&fam->family_lock);
            
            withdraw_family_father(fam->id);
            safe_print("Father %d withdrawing with %d bananas (energy depleted)\n", 
                   fam->id, fam->basket_bananas);
            break;
        }
    }
    
    safe_print("[Father %d] exiting\n", fam->id);
    pthread_exit(NULL);
}

int should_fight_father(Family *fam) {
    if (!fam->active) return 0;
    
    pthread_mutex_lock(&fam->basket_lock);
    int my_bananas = fam->basket_bananas;
    pthread_mutex_unlock(&fam->basket_lock);
    
    int total_neighbor_bananas = 0;
    for (int i = 0; i < FAMILY_NUM; i++) {
        if (i != fam->id && families[i].active) {
            pthread_mutex_lock(&families[i].basket_lock);
            total_neighbor_bananas += families[i].basket_bananas;
            pthread_mutex_unlock(&families[i].basket_lock);
        }
    }
    
    int total_bananas = my_bananas + total_neighbor_bananas;
    if (total_bananas == 0) return 0;
    
    int fight_chance = 5 + (my_bananas / 2) + (total_neighbor_bananas / 10);
    if (fight_chance > 40) fight_chance = 40;
    
    int roll = rand() % 100;
    return (roll < fight_chance);
}

int pick_neighbor_father(int father_id) {
    if (FAMILY_NUM <= 1) return -1;
    
    int valid_neighbors[FAMILY_NUM];
    int rich_neighbors[FAMILY_NUM];
    int valid_count = 0;
    int rich_count = 0;
    
    for (int i = 0; i < FAMILY_NUM; i++) {
        if (i != father_id && families[i].active) {
            valid_neighbors[valid_count++] = i;
            
            pthread_mutex_lock(&families[i].basket_lock);
            if (families[i].basket_bananas > 0) {
                rich_neighbors[rich_count++] = i;
            }
            pthread_mutex_unlock(&families[i].basket_lock);
        }
    }
    
    if (rich_count > 0 && (rand() % 100 < 75)) {
        return rich_neighbors[rand() % rich_count];
    }
    
    if (valid_count == 0) return -1;
    return valid_neighbors[rand() % valid_count];
}

void fight_with_neighbor_father(Family *self, int neighbor_id) {
    Family *neighbor = &families[neighbor_id];
    
    // Lock baskets in fixed order to avoid deadlock
    Family *first = (self->id < neighbor_id) ? self : neighbor;
    Family *second = (self->id < neighbor_id) ? neighbor : self;
    
    pthread_mutex_lock(&first->basket_lock);
    pthread_mutex_lock(&second->basket_lock);
    
    safe_print("FIGHT! Father %d (bananas: %d) vs Father %d (bananas: %d)\n",
           self->id, self->basket_bananas, 
           neighbor_id, neighbor->basket_bananas);
    
    int self_power = self->male_energy + (self->basket_bananas * 2);
    int neighbor_power = neighbor->male_energy + (neighbor->basket_bananas * 2);
    int total_power = self_power + neighbor_power;
    
    if (total_power == 0) total_power = 1;
    
    int winner_roll = rand() % total_power;
    
    if (winner_roll < self_power) {
        safe_print("Father %d WINS! Takes %d bananas from Father %d\n",
               self->id, neighbor->basket_bananas, neighbor_id);
        self->basket_bananas += neighbor->basket_bananas;
        neighbor->basket_bananas = 0;
    } else {
        safe_print("Father %d WINS! Takes %d bananas from Father %d\n",
               neighbor_id, self->basket_bananas, self->id);
        neighbor->basket_bananas += self->basket_bananas;
        self->basket_bananas = 0;
    }
    
    pthread_mutex_unlock(&second->basket_lock);
    pthread_mutex_unlock(&first->basket_lock);
}

void withdraw_family_father(int father_id) {
    pthread_mutex_lock(&global_lock);
    withdrawn_families_count++;
    safe_print("*** Family %d WITHDRAWN (Total withdrawn: %d/%d) ***\n", 
           father_id, withdrawn_families_count, WITHDRAWN_FAMILIES);
    pthread_mutex_unlock(&global_lock);
}

int is_family_active_father(int family_id) {
    if (family_id >= FAMILY_NUM || family_id < 0) return 0;
    pthread_mutex_lock(&families[family_id].family_lock);
    int a = families[family_id].active;
    pthread_mutex_unlock(&families[family_id].family_lock);
    return a;
}

int baby_steal_attempt_father(int family_id, int steal_amount) {
    if (family_id >= FAMILY_NUM || family_id < 0) return 0;
    
    if (pthread_mutex_trylock(&families[family_id].basket_lock) == 0) {
        int stolen = 0;
        if (families[family_id].basket_bananas >= steal_amount) {
            families[family_id].basket_bananas -= steal_amount;
            stolen = steal_amount;
        } else {
            stolen = families[family_id].basket_bananas;
            families[family_id].basket_bananas = 0;
        }
        
        if (stolen > 0) {
            safe_print("Baby from family %d STOLE %d bananas!\n", family_id, stolen);
        }
        
        pthread_mutex_unlock(&families[family_id].basket_lock);
        return stolen;
    }
    return 0;
}
void add_to_basket(int family_id, int bananas) {
    if (family_id >= FAMILY_NUM || family_id < 0) return;
    
    pthread_mutex_lock(&families[family_id].basket_lock);
    families[family_id].basket_bananas += bananas;
    safe_print("Mother added %d bananas to Family %d's basket (Total: %d)\n",
           bananas, family_id, families[family_id].basket_bananas);
    pthread_mutex_unlock(&families[family_id].basket_lock);
}
