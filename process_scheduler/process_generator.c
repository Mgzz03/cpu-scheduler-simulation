#include "headers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/msg.h>

//---------------- some initializations --------------
typedef enum scheduling_algorithms_ {HPF, RR, SJF} scheduling_algorithms;
scheduling_algorithms chosen_alg;
void clearResources(int);
int upQ;
int ended = 0;
int quantum = 0; // Added for RR

void handle(int signum) // Handler that changes ended from 0 to 1 when the scheduler finishes
{
    ended = 1;
}
//----------------- read file --------------------------------

/// @brief function to read inputs 
/// @param file_name 
/// @return a process list containing processes
heap_t* read_ints(const char* file_name)
{
    FILE* file = fopen(file_name, "r");
    process p[100]; // Increased size for safety
    heap_t *process_list = (heap_t *)calloc(1, sizeof(heap_t));
    
    for (int i = 0; i < 100 && !feof(file); i++) {
        if (fscanf(file, "%d %d %d %d", &p[i].ID, &p[i].ArrTime, &p[i].RunTime, &p[i].Priority) == 4) {
            push(process_list, p[i].ArrTime, &p[i]);
            printf("list size:%d, ID:%d, ArrTime:%d\n", process_list->len, p[i].ID, p[i].ArrTime);
        }
    }  

    fclose(file);   
    return process_list;     
}
//------------------- get algorithm num ----------------------------------------

/// @brief function to return the algorithm number to be sent to scheduler
/// @return char of the algorithm num
char* get_algo_num()
{
    if (chosen_alg == HPF)
        return "1";
    if (chosen_alg == RR)
        return "2";
    if (chosen_alg == SJF)
        return "3";
    return "0"; // Default (should not reach here)
}
//---------------- ask about which algorithm ------------------------------------------------

/// @brief function to ask the user which algorithm to use 
/// @return the algorithm to be used in the enum of algorithms
scheduling_algorithms ask_for_alg() {
    int answer = -1;
    while (!(answer == 1 || answer == 2 || answer == 3)) {
        printf("Please specify an algorithm for scheduling.\n1: HPF\n2: RR\n3: SJF\n");
        scanf("%d", &answer);

        switch (answer) {
        case 1:
            printf("Working with HPF\n");
            return HPF;
        case 2:
            printf("Working with RR\n");
            printf("Enter time quantum for Round Robin: ");
            scanf("%d", &quantum);
            return RR;
        case 3:
            printf("Working with SJF\n");
            return SJF;
        default:
            printf("Invalid choice. Please try again.\n");
            continue;
        }  
    }
}
//---------------- struct message --------------------------------------------------

/// @brief to store the message
typedef struct {
    long mtype;
    int ID;
    int ArrTime;
    int RunTime;
    int Priority;
    int memsize;
} msgbuff;
//------------------ fork clk --------------------------------------------------

/// @brief function to fork the clk
void fork_clk() {
    int pid = fork();
    if (pid == 0) {
        printf("clk is running\n");
        char *args[] = {"./clk.out", NULL};
        execv(args[0], args);
        perror("Error in execv'ing to clk");
        exit(EXIT_FAILURE);
    } else {
        printf("out of clk\n");
    }
}
//--------------- fork scheduler ------------------------------------------------------

/// @brief function to fork the scheduler 
/// @return pid
int fork_schedular() {
    int pid = fork();
    printf("check: %d\n", pid);
    if (pid == 0) {
        printf("check: %d\n", pid);
        char q_str[10];
        sprintf(q_str, "%d", quantum);
        char *args[] = {"./scheduler.out", get_algo_num(), q_str, NULL};
        execl("./scheduler.out", "scheduler.out", get_algo_num(), q_str, NULL);
        perror("Error in execv'ing to scheduler");
        exit(EXIT_FAILURE);
    } else {
        return pid;
    }
}

//--------------- main -----------------------------------------------------------
int main(int argc, char *argv[]) {
    //----------- some initializations ----------------------
    key_t key_up = 123;
    upQ = msgget(key_up, IPC_CREAT | 0644);
    signal(SIGINT, clearResources);
    //---------------------------------------------------------
    // 1. Read the input files.
    heap_t *list = read_ints("test.txt");
    //---------------------------------------------------------
    // 2. Ask the user for the chosen scheduling algorithm and its parameters.
    chosen_alg = ask_for_alg();
    //---------------------------------------------------------
    // 3. Initiate and create the scheduler and clock processes.
    fork_clk();
    initClk();
    fork_schedular();
    //---------------------------------------------------------
    // 6. Send the information to the scheduler at the appropriate time.
    msgbuff mss;
    while (list->len > 0) {
        if (top(list)->ArrTime <= getClk()) {
            process *proc = pop(list);
            mss.mtype = 1;
            mss.ArrTime = proc->ArrTime;
            mss.ID = proc->ID;
            mss.Priority = proc->Priority;
            mss.RunTime = proc->RunTime;
            printf("check if mss is sent at time %d\n", getClk());
            int send_val = msgsnd(upQ, &mss, sizeof(mss) - sizeof(mss.mtype), !IPC_NOWAIT);
            if (send_val == -1) {
                perror("Error in sending");
            }
        }
    }
    printf("Sending Last message\n");
    mss.ID = 0;
    int send_val = msgsnd(upQ, &mss, sizeof(mss) - sizeof(mss.mtype), !IPC_NOWAIT);
    while (!ended) {
        signal(SIGINT, handle);
    }
    //---------------------------------------------------------
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum) {
    printf("clearing resources\n");
    msgctl(upQ, IPC_RMID, (struct msqid_ds *)0);
    exit(0);
}
