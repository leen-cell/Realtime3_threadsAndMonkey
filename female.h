#ifndef FEMALE_H
#define FEMALE_H

#include <pthread.h>
#include "multithreading.h"

/* Female thread */
void *female_thread(void *arg);

/* Female helper functions */
int collect_banana(Family *fam, int *carried);
void init_female_position(Family *fam);
void fight_if_needed(Family *fam);
void move_toward_exit(Family *fam);
void move_female_greedy(Family *fam);
int is_exit_cell(int r, int c);

#endif