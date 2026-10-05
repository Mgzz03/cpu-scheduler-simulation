#include "headers.h"
#include <math.h>
#include <sys/wait.h>

#define MAX_PROCS 1000
int ProStart = 0;
int finished_flag = 0;
int lastProcess_flag = 0;
char WriteBuff[256]; // Increased buffer size

float averageWTA = 0;
float averageWt = 0;
float CPU_UTILIZATION = 0;

// Circular queue for Round Robin
typedef struct {
    process *procs[MAX_PROCS];
    int front, rear, size;
} CircularQueue;

void initQueue(CircularQueue *q) {
    q->front = q->rear = -1;
    q->size = 0;
}

int isQueueEmpty(CircularQueue *q) {
    return q->size == 0;
}

void enqueue(CircularQueue *q, process *proc) {
    if (q->size == MAX_PROCS) return;
    if (isQueueEmpty(q)) q->front = 0;
    q->rear = (q->rear + 1) % MAX_PROCS;
    q->procs[q->rear] = proc;
    q->size++;
}

process *dequeue(CircularQueue *q) {
    if (isQueueEmpty(q)) return NULL;
    process *proc = q->procs[q->front];
    q->front = (q->front + 1) % MAX_PROCS;
    q->size--;
    if (q->size == 0) q->front = q->rear = -1;
    return proc;
}

typedef struct {
    long mtype;
    int ID;
    int ArrTime;
    int RunTime;
    int Priority;
    int memsize;
} msgbuff;

void handle(int signum) {
    int x;
    wait(&x);
    finished_flag = 1;
}

void write_perf_file(float cpu_ut, float ta, float avg_wait, float std_wta) {
    FILE *sch_perf_file = fopen("scheduler.perf", "w");
    fprintf(sch_perf_file, "CPU utilization = %.2f%%\nAvg WTA = %.2f\nAvg Waiting = %.2f\nStd WTA = %.2f\n", cpu_ut, ta, avg_wait, std_wta);
    fclose(sch_perf_file);
}

int main(int argc, char **argv) {
    FILE *fPointer;
    fPointer = fopen("Scheduler.log", "w");
    fprintf(fPointer, "#At\ttime\tx\tprocess\ty\tstate\t\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");

    int algorithm = atoi(argv[1]);
    int quantum = (algorithm == 2) ? atoi(argv[2]) : 0; // Quantum for RR
    process p, running_process;
    running_process.ID = 0;
    running_process.Priority = -1;
    process *puff;
    heap_t *process_list = (heap_t *)calloc(1, sizeof(heap_t));
    heap_t *ProFinished = (heap_t *)calloc(1, sizeof(heap_t));
    CircularQueue rr_queue;
    initQueue(&rr_queue);
    msgbuff data;
    int msgqid1 = msgget((key_t)123, 0644 | IPC_CREAT);
    initClk();
    int rec_val = msgrcv(msgqid1, &data, sizeof(data) - sizeof(data.mtype), 0, !IPC_NOWAIT);
    int last_time = getClk();

    while (true) {
        signal(SIGINT, handle);
        // Read received data
        while (rec_val != -1) {
            if (data.ID == 0) {
                lastProcess_flag = 1;
                break;
            }
            p.ID = data.ID;
            p.Priority = data.Priority;
            p.RunTime = data.RunTime;
            p.ArrTime = data.ArrTime;
            p.PID = 0;
            p.RemaingTime = data.RunTime;
            p.WatingTime = 0;
            p.start_time = 0; // For RR
            if (algorithm == 1) { // HPF
                push(process_list, p.Priority, &p);
            } else if (algorithm == 2) { // RR
                enqueue(&rr_queue, &p);
            } else if (algorithm == 3) { // SJF
                push(process_list, p.RunTime, &p);
            }
            rec_val = msgrcv(msgqid1, &data, sizeof(data) - sizeof(data.mtype), 0, IPC_NOWAIT);
        }

        int current_time = getClk();
        if (current_time > last_time) {
            // Update waiting times for ready processes
            for (int i = 0; i < process_list->len; i++) {
                if (!process_list->proc[i].Running && !process_list->proc[i].Stoped) {
                    process_list->proc[i].WatingTime += current_time - last_time;
                }
            }
            if (algorithm == 2) {
                for (int i = 0; i < rr_queue.size; i++) {
                    int idx = (rr_queue.front + i) % MAX_PROCS;
                    if (!rr_queue.procs[idx]->Running && !rr_queue.procs[idx]->Stoped) {
                        rr_queue.procs[idx]->WatingTime += current_time - last_time;
                    }
                }
            }
            last_time = current_time;
        }

        if (algorithm == 1) { // Preemptive HPF
            if (top(process_list) != NULL && (running_process.Priority == -1 || top(process_list)->Priority < running_process.Priority || finished_flag == 1)) {
                if (finished_flag == 0 && running_process.ID != 0) {
                    printf("Process ID: %d Stoped\n", running_process.ID);
                    kill(running_process.PID, SIGSTOP);
                    running_process.RemaingTime -= (getClk() - ProStart);
                    running_process.Stoped = getClk();
                    running_process.Running = 0;
                    sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tstopped\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime);
                    fprintf(fPointer, WriteBuff);
                    push(process_list, running_process.Priority, &running_process);
                } else if (finished_flag == 1) {
                    running_process.RemaingTime = 0;
                    running_process.Stoped = getClk();
                    sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime, (getClk() - running_process.ArrTime), (float)(getClk() - running_process.ArrTime) / running_process.RunTime);
                    fprintf(fPointer, WriteBuff);
                    averageWTA += (float)((getClk() - running_process.ArrTime) / running_process.RunTime);
                    averageWt += (float)running_process.WatingTime;
                    CPU_UTILIZATION += running_process.RunTime;
                    push(ProFinished, running_process.ID, &running_process);
                    printf("Process ID: %d end\n", running_process.ID);
                }
                puff = pop(process_list);
                running_process = *puff;
                if (running_process.PID == 0) {
                    int pid = fork();
                    if (pid == -1) perror("error in fork");
                    else if (pid == 0) {
                        char BurstTime[5];
                        char arrt[5];
                        sprintf(BurstTime, "%d", running_process.RunTime);
                        sprintf(arrt, "%d", running_process.ArrTime);
                        execl("./process.out", "process.out", BurstTime, arrt, NULL);
                    } else {
                        running_process.PID = pid;
                        running_process.WatingTime += (getClk() - running_process.ArrTime);
                        running_process.Running = 1;
                        printf("Process ID: %d Starts \n", running_process.ID);
                        sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tstarted\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime);
                        fprintf(fPointer, WriteBuff);
                    }
                } else {
                    running_process.WatingTime += (getClk() - running_process.Stoped);
                    kill(running_process.PID, SIGCONT);
                    running_process.Running = 1;
                    printf("Process ID: %d Resume\n", running_process.ID);
                    sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tresumed\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime);
                    fprintf(fPointer, WriteBuff);
                }
                finished_flag = 0;
                ProStart = getClk();
            } else if (top(process_list) == NULL && lastProcess_flag == 1 && finished_flag) {
                running_process.RemaingTime = 0;
                running_process.Running = 0;
                running_process.Stoped = getClk();
                push(ProFinished, running_process.ID, &running_process);
                averageWt += (float)running_process.WatingTime;
                averageWTA += (float)((getClk() - running_process.ArrTime) / running_process.RunTime);
                CPU_UTILIZATION = 100 * (CPU_UTILIZATION / (float)getClk());
                averageWt = averageWt / (float)ProFinished->len;
                averageWTA = averageWTA / (float)ProFinished->len;
                sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime, (getClk() - running_process.ArrTime), (float)(getClk() - running_process.ArrTime) / running_process.RunTime);
                fprintf(fPointer, WriteBuff);
                break;
            }
        } else if (algorithm == 2) { // Round Robin
            if (running_process.ID != 0 && (finished_flag == 1 || (getClk() - ProStart >= quantum && running_process.RemaingTime > 0))) {
                if (finished_flag == 0) {
                    printf("Process ID: %d Stoped\n", running_process.ID);
                    kill(running_process.PID, SIGSTOP);
                    running_process.RemaingTime -= (getClk() - ProStart);
                    running_process.Stoped = getClk();
                    running_process.Running = 0;
                    sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tstopped\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime);
                    fprintf(fPointer, WriteBuff);
                    enqueue(&rr_queue, &running_process);
                } else {
                    running_process.RemaingTime = 0;
                    running_process.Stoped = getClk();
                    sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime, (getClk() - running_process.ArrTime), (float)(getClk() - running_process.ArrTime) / running_process.RunTime);
                    fprintf(fPointer, WriteBuff);
                    averageWTA += (float)((getClk() - running_process.ArrTime) / running_process.RunTime);
                    averageWt += (float)running_process.WatingTime;
                    CPU_UTILIZATION += running_process.RunTime;
                    push(ProFinished, running_process.ID, &running_process);
                    printf("Process ID: %d end\n", running_process.ID);
                }
                running_process.ID = 0;
                finished_flag = 0;
            }
            if (running_process.ID == 0 && !isQueueEmpty(&rr_queue)) {
                puff = dequeue(&rr_queue);
                running_process = *puff;
                if (running_process.PID == 0) {
                    int pid = fork();
                    if (pid == -1) perror("error in fork");
                    else if (pid == 0) {
                        char BurstTime[5];
                        char arrt[5];
                        sprintf(BurstTime, "%d", running_process.RunTime);
                        sprintf(arrt, "%d", running_process.ArrTime);
                        execl("./process.out", "process.out", BurstTime, arrt, NULL);
                    } else {
                        running_process.PID = pid;
                        running_process.WatingTime += (getClk() - running_process.ArrTime);
                        running_process.Running = 1;
                        printf("Process ID: %d Starts \n", running_process.ID);
                        sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tstarted\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime);
                        fprintf(fPointer, WriteBuff);
                    }
                } else {
                    running_process.WatingTime += (getClk() - running_process.Stoped);
                    kill(running_process.PID, SIGCONT);
                    running_process.Running = 1;
                    printf("Process ID: %d Resume\n", running_process.ID);
                    sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tresumed\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime);
                    fprintf(fPointer, WriteBuff);
                }
                ProStart = getClk();
            } else if (isQueueEmpty(&rr_queue) && lastProcess_flag == 1 && finished_flag && running_process.ID == 0) {
                CPU_UTILIZATION = 100 * (CPU_UTILIZATION / (float)getClk());
                averageWt = averageWt / (float)ProFinished->len;
                averageWTA = averageWTA / (float)ProFinished->len;
                break;
            }
        } else if (algorithm == 3) { // SJF
            if (top(process_list) != NULL && (finished_flag == 1 || running_process.ID == 0)) {
                if (running_process.ID != 0) {
                    running_process.RemaingTime = 0;
                    running_process.Running = 0;
                    running_process.Stoped = getClk();
                    printf("Process ID: %d Stoped\n", running_process.ID);
                    averageWTA += (float)((getClk() - running_process.ArrTime) / running_process.RunTime);
                    averageWt += (float)running_process.WatingTime;
                    CPU_UTILIZATION += running_process.RunTime;
                    sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime, (getClk() - running_process.ArrTime), (float)(getClk() - running_process.ArrTime) / running_process.RunTime);
                    fprintf(fPointer, WriteBuff);
                    push(ProFinished, running_process.ID, &running_process);
                }
                puff = pop(process_list);
                running_process = *puff;
                if (running_process.PID == 0) {
                    int pid = fork();
                    if (pid == -1) perror("error in fork");
                    else if (pid == 0) {
                        char BurstTime[5];
                        char arrt[5];
                        sprintf(BurstTime, "%d", running_process.RunTime);
                        sprintf(arrt, "%d", running_process.ArrTime);
                        execl("./process.out", "process.out", BurstTime, arrt, NULL);
                    } else {
                        running_process.PID = pid;
                        running_process.WatingTime += (getClk() - running_process.ArrTime);
                        running_process.Running = 1;
                        printf("Process ID: %d Starts \n", running_process.ID);
                        sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tstarted\t\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime);
                        fprintf(fPointer, WriteBuff);
                    }
                }
                finished_flag = 0;
                ProStart = getClk();
            } else if (top(process_list) == NULL && lastProcess_flag == 1 && finished_flag) {
                running_process.RemaingTime = 0;
                running_process.Running = 0;
                running_process.Stoped = getClk();
                push(ProFinished, running_process.ID, &running_process);
                averageWt += (float)running_process.WatingTime;
                averageWTA += (float)((getClk() - running_process.ArrTime) / running_process.RunTime);
                CPU_UTILIZATION = 100 * (CPU_UTILIZATION / (float)getClk());
                averageWt = averageWt / (float)ProFinished->len;
                averageWTA = averageWTA / (float)ProFinished->len;
                sprintf(WriteBuff, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\n", getClk(), running_process.ID, running_process.ArrTime, running_process.RunTime, running_process.RemaingTime, running_process.WatingTime, (getClk() - running_process.ArrTime), (float)(getClk() - running_process.ArrTime) / running_process.RunTime);
                fprintf(fPointer, WriteBuff);
                break;
            }
        }

        rec_val = msgrcv(msgqid1, &data, sizeof(data) - sizeof(data.mtype), 0, IPC_NOWAIT);
    }

    write_perf_file(CPU_UTILIZATION, averageWTA, averageWt, 0);
    fclose(fPointer);
    printf("Sch. Finished\n");
    msgctl(msgqid1, IPC_RMID, NULL);
    free(process_list);
    free(ProFinished);
    destroyClk(1);
    return 0;
}
