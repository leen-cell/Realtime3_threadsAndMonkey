#ifndef FATHER_H
#define FATHER_H

#include <pthread.h>
#include "multithreading.h"


#define MAX_FATHERS 7


//extern father_t fathers[MAX_FATHERS];
extern int NUM_FATHERS;

// Main thread function
void* father_thread(void* arg);

// Helper functions
int should_fight_father(Family *fam);
int pick_neighbor_father(int father_id);
void fight_with_neighbor_father(Family *self, int neighbor_id);
void withdraw_family_father(int father_id);
int is_family_active_father(int family_id);
int baby_steal_attempt_father(int family_id, int steal_amount);



// Interface functions for other threads (mothers, babies)
void add_to_basket(int father_id, int bananas);
int is_family_active(int father_id);
int baby_steal_attempt_father(int father_id, int steal_amount);

#endif // FATHER_H
