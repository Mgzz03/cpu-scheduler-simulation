#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

typedef short bool;
#define true 1
#define false 0
#define SHKEY 300

// Process structure
typedef struct {
    int ID;
    int ArrTime;
    int RunTime;
    int Priority;
    int PID;
    int RemaingTime;
    int WatingTime;
    int Stoped;
    int Running;
    int start_time; // For RR
} process;

// Heap structure for HPF and SJF
typedef struct {
    process *proc;
    int *Priority;
    int len;
    int size;
} heap_t;

// Heap functions
void push(heap_t *h, int priority, process *proc) {
    if (h->len >= h->size) {
        h->size = h->size ? h->size * 2 : 100;
        h->proc = realloc(h->proc, h->size * sizeof(process));
        h->Priority = realloc(h->Priority, h->size * sizeof(int));
    }
    h->proc[h->len] = *proc;
    h->Priority[h->len] = priority;
    h->len++;
    // Simple bubble-up for min-heap
    int i = h->len - 1;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->Priority[i] < h->Priority[parent]) {
            // Swap priority
            int temp = h->Priority[i];
            h->Priority[i] = h->Priority[parent];
            h->Priority[parent] = temp;
            // Swap process
            process temp_proc = h->proc[i];
            h->proc[i] = h->proc[parent];
            h->proc[parent] = temp_proc;
            i = parent;
        } else {
            break;
        }
    }
}

process *pop(heap_t *h) {
    if (h->len == 0) return NULL;
    process *result = malloc(sizeof(process));
    *result = h->proc[0];
    h->len--;
    if (h->len > 0) {
        h->proc[0] = h->proc[h->len];
        h->Priority[0] = h->Priority[h->len];
        // Bubble-down for min-heap
        int i = 0;
        while (1) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int smallest = i;
            if (left < h->len && h->Priority[left] < h->Priority[smallest]) smallest = left;
            if (right < h->len && h->Priority[right] < h->Priority[smallest]) smallest = right;
            if (smallest != i) {
                // Swap priority
                int temp = h->Priority[i];
                h->Priority[i] = h->Priority[smallest];
                h->Priority[smallest] = temp;
                // Swap process
                process temp_proc = h->proc[i];
                h->proc[i] = h->proc[smallest];
                h->proc[smallest] = temp_proc;
                i = smallest;
            } else {
                break;
            }
        }
    }
    return result;
}

process *top(heap_t *h) {
    if (h->len == 0) return NULL;
    return &h->proc[0];
}

// Shared memory for clock
int *shmaddr;

int getClk() {
    return *shmaddr;
}

void initClk() {
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1) {
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

void destroyClk(bool terminateAll) {
    shmdt(shmaddr);
    if (terminateAll) {
        killpg(getpgrp(), SIGINT);
    }
}
