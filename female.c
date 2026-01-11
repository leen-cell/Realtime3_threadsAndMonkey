#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "female.h"
#include "maze.h"
#include "father.h"
#include <limits.h>
#define MAX_INIT_TRIES 50 //Maximum attempts to locate the starting position

// External globals 
extern volatile int simulation_running;
extern Family *families;

// FEMALE THREAD 
void *female_thread(void *arg)
{
    Family *fam = (Family *)arg;
    fam->female_energy = FEMALE_INITIAL_ENERGY;
    fam->carried = 0;
    fam->female_state = SEARCHING;
    init_female_position(fam);
    while (simulation_running && fam->active) {
        pthread_mutex_lock(&fam->family_lock);
         /*
         This mutex protects the family state while the female is updating it.
         Even though only the female thread modifies her energy, state, and position,
         the OpenGL display thread reads these values at the same time to render the maze.
         Without this lock, the display could read partially updated data, causing
         inconsistent or incorrect rendering (race condition).
        */
        //RESTING 
        if (fam->female_energy <= ENERGY_THRESHOLD)
            fam->female_state = RESTING;
        if (fam->female_state == RESTING) {
            fam->female_energy += 2;
            pthread_mutex_unlock(&fam->family_lock);
            sleep(1);
            pthread_mutex_lock(&fam->family_lock); //After sleep, reacquire family lock to safely update state
            if (fam->female_energy >= FEMALE_INITIAL_ENERGY / 2)
                fam->female_state = SEARCHING;
            pthread_mutex_unlock(&fam->family_lock);
            continue;
        }
        //SEARCHING
        if (fam->female_state == SEARCHING) {
            move_female_greedy(fam);
            fam->female_energy -= ENERGY_LOSS_MOVE;
            int taken = collect_banana(fam, &fam->carried);
            if (taken > 0) {
                fam->female_energy -= ENERGY_LOSS_COLLECT;
                safe_print("[Female %d] collected %d (total=%d)\n",
                       fam->id, taken, fam->carried);
            }
            if (fam->carried >= BANANA_TO_COLLECT)
                fam->female_state = RETURNING;
        }
        //RETURNING 
        else if (fam->female_state == RETURNING) {
            move_toward_exit(fam);

            pthread_mutex_unlock(&fam->family_lock);
            fight_if_needed(fam);
            pthread_mutex_lock(&fam->family_lock);

            if (is_exit_cell(fam->female_row, fam->female_col)) {

                                /* CLEAR OCCUPANCY BEFORE TELEPORT */
                pthread_mutex_lock(&maze[fam->female_row][fam->female_col].lock);
                maze[fam->female_row][fam->female_col].occupied = -1;
                pthread_mutex_unlock(&maze[fam->female_row][fam->female_col].lock);

                add_to_basket(fam->id, fam->carried);
                safe_print("[Female %d] deposited %d bananas\n",
                       fam->id, fam->carried);
                fam->carried = 0;
                init_female_position(fam);
                fam->female_state = SEARCHING;
                
            }
        }
        pthread_mutex_unlock(&fam->family_lock);
        usleep(200000);// to update the location & state.

    }
    //EMERGENCY DEPOSIT 
    /*
    EMERGENCY DEPOSIT: occurs after the while loop if the female
    is still carrying bananas when the simulation ends or the family withdraws.
    Safe because add_to_basket locks the father's basket mutex.
    */
    pthread_mutex_lock(&fam->family_lock);
    if (fam->carried > 0) {
        add_to_basket(fam->id, fam->carried);
        safe_print("[Female %d] emergency deposit %d bananas\n",
               fam->id, fam->carried);
        fam->carried = 0;
    }
    pthread_mutex_unlock(&fam->family_lock);

       /* CLEAR OCCUPANCY ON EXIT */
    pthread_mutex_lock(&maze[fam->female_row][fam->female_col].lock);
    if (maze[fam->female_row][fam->female_col].occupied == fam->id)
        maze[fam->female_row][fam->female_col].occupied = -1;
    pthread_mutex_unlock(&maze[fam->female_row][fam->female_col].lock);


    safe_print("[Female %d] EXITING\n", fam->id);
    return NULL;
}
/*
===============================================================================================================
                                                 HELPER FUNCTIONS
===============================================================================================================
*/
int collect_banana(Family *fam, int *carried)
{
    Cell *cell = &maze[fam->female_row][fam->female_col];
    int remaining = BANANA_TO_COLLECT - (*carried);
    int taken = 0; // default if no banana is collected.
    pthread_mutex_lock(&cell->lock); // lock the cell to prevent race condition(to prevnt 2 females from takeing the same banana (false)).
    // Check if there are bananas and the female can still carry more.
    if (cell->value > 0 && remaining > 0) {
        // Take the smaller of available bananas or remaining capacity.
        if(cell->value <= remaining) 
        taken = cell->value;
        else
        taken = remaining;
        // Update cell and carried count.
        cell->value -= taken;
        *carried += taken;
    }
    pthread_mutex_unlock(&cell->lock); // always unlock before returning.
    return taken; // return the number of bananas taken.
}
//===================================================================================
int is_exit_cell(int r, int c)
{
    return (r == 0          ||   //Top.
         r == maze_rows - 1 ||   //Bottom.
         c == 0             ||   //Left.
         c == maze_cols - 1);    //Right.
}
//===================================================================================
void fight_if_needed(Family *fam)
{
    //Fighting only occurs if returning state.
    if (fam->female_state != RETURNING) {
        return;
    }
    //female ape doesnt fight it self.
    for (int i = 0; i < FAMILY_NUM; i++) {
        if (i == fam->id) {
            continue;
        }
        Family *other = &families[i];//other families.
        Family *first;
        Family *second;
        //order the females based on its id, to prevent deadlock.
        if (fam->id < other->id) {
            first = fam;
            second = other;
        } else {
            first = other;
            second = fam;
        }
        pthread_mutex_lock(&first->family_lock);
        pthread_mutex_lock(&second->family_lock);
        //check the other female, is it active ? and returning?
        if (!other->active || other->female_state != RETURNING) {
            pthread_mutex_unlock(&second->family_lock);
            pthread_mutex_unlock(&first->family_lock);
            continue;// if no, go back and check the next one.
        }
        //Calculate the distance between the two females, because fighting will only occur if they are close to each other.
        int dr = abs(fam->female_row - other->female_row);// row distance.
        int dc = abs(fam->female_col - other->female_col);// column distance.
        if (dr + dc != 1) {
            pthread_mutex_unlock(&second->family_lock);
            pthread_mutex_unlock(&first->family_lock);
            continue;// they are not close.
        }
        // if they are close to each other then : 
        int e1 = fam->female_energy;
        int e2 = other->female_energy;
        if (e1 == 0 && e2 == 0) {
            pthread_mutex_unlock(&second->family_lock);
            pthread_mutex_unlock(&first->family_lock);
            return;//there is no enough energy, dont fight.
        }
        //if there is enough energy then Decide rendomlly baced on energy, who is the winner.
        int r = rand() % (e1 + e2);
        Family *winner;
        Family *loser;
        if (r < e1) {
            winner = fam;
            loser = other;
        } else {
            winner = other;
            loser = fam;
        }
        // FLAG ON for both fighters
        winner->female_fighting = 1;
        loser->female_fighting = 1;
        usleep(200000);
        winner->female_fighting = 0;
        loser->female_fighting = 0;
        //the winner will take(stole) the banana.
        int stolen = loser->carried;
        if (stolen > 0) {
            loser->carried = 0;
            winner->carried += stolen;
        }
        //the females will lose the   ' ENERGY_LOSS_FIGHT '
        if (winner->female_energy > ENERGY_LOSS_FIGHT) {
            winner->female_energy -= ENERGY_LOSS_FIGHT;
        } else {
            winner->female_energy = 0;
        }

        if (loser->female_energy > ENERGY_LOSS_FIGHT) {
            loser->female_energy -= ENERGY_LOSS_FIGHT;
        } else {
            loser->female_energy = 0;// there is no minus energy.
        }
        //print the result.
        if (stolen > 0) {
            safe_print("[Fight] Female %d vs Female %d → Winner: %d (stolen=%d)\n",
                   fam->id, other->id, winner->id, stolen);
        }
        // unlock the mutex.
        pthread_mutex_unlock(&second->family_lock);
        pthread_mutex_unlock(&first->family_lock);
        return;
    }
}
//=================================================================================================
void init_female_position(Family *fam)
{
    int r, c, side, tries = 0;
    while (tries < MAX_INIT_TRIES) {
        side = rand() % 4;
        if (side == 0)// Top.
        { 
            r = 0; // first row.
            c = rand() % maze_cols;// random column.
        }
        else if (side == 1) // Bottom.
        {
             r = maze_rows - 1; // the last row.
             c = rand() % maze_cols;// random column.
        }
        else if (side == 2) // Left.
        { 
            r = rand() % maze_rows; // random row.s
            c = 0;// first column.
        }
        else // Right.
        { 
            r = rand() % maze_rows;// random row.
            c = maze_cols - 1;// Last column.
        }
        if (valid_cell(r, c)) break;// the cell should not be obstacle (then it wont be able to move !).
        tries++; // if not available, try another cell.
    }
    fam->female_row = r;
    fam->female_col = c;
    pthread_mutex_lock(&maze[r][c].lock);
    maze[r][c].occupied = fam->id;
    pthread_mutex_unlock(&maze[r][c].lock);

    safe_print("[Init] Female %d enters maze at (%d,%d)\n",
           fam->id, r, c);
}
/////////////////////////////////////////////////////////////////////
void move_toward_exit(Family *fam)
{
     /*
      row delta(change): up (-1), down (+1), no change (0).
      column delta(change) : left (-1), right(+1), no change(0). 
    */
    int dr = 0, dc = 0;

    // If already at exit, no movement is needed
    if (is_exit_cell(fam->female_row, fam->female_col)) {
        return;
    }

    //to locate the female position.
    int top = fam->female_row;// how much it is from the top? = the row number! easy 
    int bottom = maze_rows - 1 - fam->female_row;// how far is it from the last column? = last column - the current row.(Distance)
    int left = fam->female_col;// the number of current column decides its left position.
    int right = maze_cols - 1 - fam->female_col;//last column - current column. this gives the right position.

    // array of possible directions {dr, dc}
    int dirs[4][2] = {
        {-1, 0},  //TOP.
        { 1, 0},  // BOTTOM.
        { 0,-1},  //  LEFT.
        { 0, 1}   // RIGHT.
    };

    int candidates[4];
    int candidate_count = 0;

    int best_dist = INT_MAX;

    // evaluate moves based on future distance to exit
    for (int i = 0; i < 4; i++) {
        int nr = fam->female_row + dirs[i][0];
        int nc = fam->female_col + dirs[i][1];

        int cell_valid;
        if (is_exit_cell(nr, nc)) {
            cell_valid = 1;
        } else {
            cell_valid = valid_cell(nr, nc);
        }

        if (!cell_valid)
            continue;

        // future distance after the move
        int d_top = nr;
        int d_bottom = maze_rows - 1 - nr;
        int d_left = nc;
        int d_right = maze_cols - 1 - nc;

        int future_dist = d_top;
        if (d_bottom < future_dist) future_dist = d_bottom;
        if (d_left < future_dist) future_dist = d_left;
        if (d_right < future_dist) future_dist = d_right;

        if (future_dist < best_dist) {
            best_dist = future_dist;
            candidate_count = 0;
            candidates[candidate_count++] = i;
        } else if (future_dist == best_dist) {
            candidates[candidate_count++] = i;
        }
    }

    // fallback uses same candidates array but keeps direction preference
    if (candidate_count == 0) {
        for (int i = 0; i < 4; i++) {
            int nr = fam->female_row + dirs[i][0];
            int nc = fam->female_col + dirs[i][1];
            if (valid_cell(nr, nc)) {
                candidates[candidate_count++] = i;
            }
        }
    }

    if (candidate_count == 0) {
        safe_print("[WARN] Female %d trapped at (%d,%d)\n",
                   fam->id, fam->female_row, fam->female_col);
        return;
    }

    int choice = candidates[rand() % candidate_count];
    dr = dirs[choice][0];
    dc = dirs[choice][1];

    // new location
    int nr = fam->female_row + dr;
    int nc = fam->female_col + dc;

    int cur_r = fam->female_row;
    int cur_c = fam->female_col;

    // lock smaller coordinate first (row major order) to avoid deadlock
    if (cur_r < nr || (cur_r == nr && cur_c < nc)) {
        pthread_mutex_lock(&maze[cur_r][cur_c].lock);
        pthread_mutex_lock(&maze[nr][nc].lock);
    } else {
        pthread_mutex_lock(&maze[nr][nc].lock);
        pthread_mutex_lock(&maze[cur_r][cur_c].lock);
    }

    // movement allowed into exit cell even if not valid_cell
    if (is_exit_cell(nr, nc) || maze[nr][nc].occupied == -1) {
        maze[cur_r][cur_c].occupied = -1;
        maze[nr][nc].occupied = fam->id;
        fam->female_row = nr;
        fam->female_col = nc;
    }

    pthread_mutex_unlock(&maze[cur_r][cur_c].lock);
    pthread_mutex_unlock(&maze[nr][nc].lock);
}
//////////////////////////////////////////////////////////////////////
void move_female_greedy(Family *fam)
{
    int dirs[4][2] = {{-1,0},{1,0},{0,1},{0,-1}}; // up, down, right, left
    int candidates_r[4];
    int candidates_c[4];
    int candidate_count = 0;
    int best_value = -1;

    // Stage 1: Evaluate neighbors with short locks
    for (int i = 0; i < 4; i++) {
        int nr = fam->female_row + dirs[i][0];
        int nc = fam->female_col + dirs[i][1];
        if (!valid_cell(nr, nc))
            continue;

        pthread_mutex_lock(&maze[nr][nc].lock);
        int cell_occupied = maze[nr][nc].occupied;
        int cell_value = maze[nr][nc].value;
        pthread_mutex_unlock(&maze[nr][nc].lock);

        if (cell_occupied != -1)
            continue;

        if (cell_value > best_value) {
            best_value = cell_value;
            candidate_count = 0;
        }
        if (cell_value == best_value) {
            candidates_r[candidate_count] = nr;
            candidates_c[candidate_count] = nc;
            candidate_count++;
        }
    }

    // If no high-value neighbors, fallback: any valid empty cell
    if (candidate_count == 0) {
        for (int i = 0; i < 4; i++) {
            int nr = fam->female_row + dirs[i][0];
            int nc = fam->female_col + dirs[i][1];
            if (valid_cell(nr, nc)) {
                pthread_mutex_lock(&maze[nr][nc].lock);
                if (maze[nr][nc].occupied == -1) {
                    candidates_r[candidate_count] = nr;
                    candidates_c[candidate_count] = nc;
                    candidate_count++;
                }
                pthread_mutex_unlock(&maze[nr][nc].lock);
            }
        }
    }

    if (candidate_count == 0)
        return; // no valid move at all

    // Randomly pick one among candidates
    int choice = rand() % candidate_count;
    int new_r = candidates_r[choice];
    int new_c = candidates_c[choice];

    // Stage 2: Move with ordered locks to avoid deadlock
    int cur_r = fam->female_row;
    int cur_c = fam->female_col;

    // lock smaller coordinate first (row major order)
    if (cur_r < new_r || (cur_r == new_r && cur_c < new_c)) {
        pthread_mutex_lock(&maze[cur_r][cur_c].lock);
        pthread_mutex_lock(&maze[new_r][new_c].lock);
    } else {
        pthread_mutex_lock(&maze[new_r][new_c].lock);
        pthread_mutex_lock(&maze[cur_r][cur_c].lock);
    }

    if (maze[new_r][new_c].occupied == -1) {
        maze[cur_r][cur_c].occupied = -1;
        maze[new_r][new_c].occupied = fam->id;
        fam->female_row = new_r;
        fam->female_col = new_c;
    }

    pthread_mutex_unlock(&maze[cur_r][cur_c].lock);
    pthread_mutex_unlock(&maze[new_r][new_c].lock);
}