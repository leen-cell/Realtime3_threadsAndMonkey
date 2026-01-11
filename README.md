
#  Apes Collecting Bananas – POSIX Threads Simulation

**ENCS4330 – Real-Time Applications & Embedded Systems**
**Project #3 – POSIX Threads under Unix/Linux**

**Instructor:** Dr. Hanna Bullata
**Semester:** 1st Semester 2025/2026
---

##  Project Overview

This project simulates the behavior of **ape families collecting bananas inside a 2D maze** using **POSIX threads (pthreads)** in C under Linux.
The simulation models **concurrent behaviors**, **synchronization**, **shared resources**, and **real-time events**, while providing **OpenGL visualization**.

Each ape family consists of:

* **Father (Male ape)** – protects the basket
* **Mother (Female ape)** – collects bananas from the maze
* **Baby apes** – attempt to steal/eat bananas during fights

The simulation ends based on configurable termination conditions defined in a **configuration file**.

---

## 🧠 Main Concepts Used

* POSIX Threads (`pthread`)
* Mutexes & synchronization
* Shared data structures
* Randomized maze generation
* Thread coordination and termination
* OpenGL graphics (simple visualization)
* Configuration-file driven parameters

---

##  Project Structure

```
.
├── main.c              # Program entry point, initialization, controller logic
├── maze.c / maze.h     # Maze creation, obstacles & banana distribution
├── multithreading.c/.h# Thread utilities, controller thread, safe_print
├── father.c / father.h # Father (male ape) thread behavior
├── female.c / female.h # Female (mother ape) thread behavior
├── graphics.c / graphics.h # OpenGL visualization
├── config.txt          # User-defined simulation parameters
├── Makefile            # Build instructions
└── README.md           # This file
```

---

##  Simulation Logic

###  Female Apes (Mothers)

* Enter the maze and collect bananas.
* Leave the maze after collecting a fixed number of bananas.
* If two females encounter each other while exiting, a **fight may occur**.
* Fighting causes **energy loss**.
* If energy drops below a threshold → the female rests temporarily.

---

###  Male Apes (Fathers)

* Guard the family basket.
* Random fights may occur with **neighboring male apes**.
* Fight probability increases as basket banana count increases.
* Winner steals all bananas from the loser.
* Energy decreases during fights.
* If energy drops below threshold → **entire family withdraws** from simulation.

---

###  Baby Apes

* Observe male fights.
* Attempt to **steal bananas** during fights.
* May:

  * Eat bananas
  * Or add them to their father’s basket
* If eaten bananas exceed a threshold → simulation ends.

---

##  Maze Description

* 2D grid with:

  * Empty cells
  * Obstacles
  * Banana cells (each cell may contain multiple bananas)
* Maze size, banana distribution, and obstacle density are randomized.
* Visualization updates dynamically using OpenGL.

---

##  Simulation Termination Conditions

The simulation stops when **any** of the following is true:

1. Withdrawn families exceed a given limit
2. A family’s basket exceeds a banana threshold
3. A baby eats too many bananas
4. Maximum simulation time is reached

(All values are configurable)

---

##  Configuration File (`config.txt`)

All user-defined parameters are loaded from a configuration file to avoid recompilation.


---

## Threading Model

| Component  | Implementation           |
| ---------- | ------------------------ |
| Father     | One pthread per family   |
| Female     | One pthread per family   |
| Baby       | One pthread per family   |
| Controller | Single monitoring thread |

Mutexes are used to protect:

* Family state
* Basket content
* Simulation state
* Console printing (`safe_print`)

---

##  Compilation & Execution

###  Compile

```bash
make
```

###  Run

```bash
./program
```

###  Clean

```bash
make clean
```

---

##  Debugging

For debugging:

```bash
gcc -g -Wall -Wextra -pthread *.c -o program
gdb ./program
```

---


##  Author

**Leen Alqazaqi**
**Leen Aldeek**
**Rand Awadallah**
**Areej Younis**
Electrical & Computer Engineering
Birzeit University
