#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define MAX_PROCESS 20
#define MAX_IO 3
#define MAX_TIME 1000
#define TIME_QUANTUM 5

//process 구조체 정의
typedef struct {
    int pid;
    int arrival_time;

    int total_cpu_burst_time;
    int remaining_time;
    int next_burst; //다음 실행될 cpu burst의 길이

    int io_count;
    int io_index;
    int io_request_times[MAX_IO];
    int io_burst_times[MAX_IO];
    int total_io_time;

    int priority;
    int readyq_time; // ready queue에 들어온 시점

    int start_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int response_time;
    int is_completed;
} Process;

//queue 구조체 정의
typedef struct {
    Process* data[MAX_PROCESS];
    int front;
    int rear;
    int size;
} Queue;

//Min heap 구조체 정의의
typedef struct {
    Process* data[MAX_PROCESS];
    int size;
} MinHeap;

// queue 함수
void initQueue(Queue *q){
    q->front = 0;
    q->rear = 0;
    q->size = 0;
}

int isEmpty(Queue *q){
    return (q->size == 0);
}

int isFull(Queue *q){
    return (q->size == MAX_PROCESS);
}

// 성공하면 0 반환, 실패하면 -1 반환환
int enqueue(Queue *q, Process *p){
    if (isFull(q)){
        return -1;
    }
    q->data[q->rear] = p;
    q->rear = (q->rear + 1) % MAX_PROCESS;
    q->size++;
    return 0;
}

Process* dequeue(Queue *q){
    if (isEmpty(q)) return NULL;
    Process *p = q->data[q->front];
    q->front = (q->front + 1) % MAX_PROCESS;
    q->size --;
    return p;
}

//MinHeap 함수
void initMinHeap(MinHeap *h){
    h->size = 0;
}

int heapInsert_NSJF(MinHeap *h, Process *p){
    if (h->size >= MAX_PROCESS){
        return -1; // 실패
    }
    int i = h -> size++;
    while (i > 0){
        int parent = (i -1) / 2;
        if (h->data[parent]->next_burst < p->next_burst
            || (h->data[parent]->next_burst == p->next_burst
            && h->data[parent]->readyq_time <= p->readyq_time)){
            break;
        }
        h->data[i] = h->data[parent];
        i = parent;
    }
    h->data[i] = p;
    return 0; //성공
}

int heapInsert_PSJF(MinHeap *h, Process *p){
    if (h->size >= MAX_PROCESS){
        return -1; // 실패
    }
    int i = h -> size++;
    while (i > 0){
        int parent = (i -1) / 2;
        if (h->data[parent]->remaining_time < p->remaining_time
            || (h->data[parent]->remaining_time == p->remaining_time
            && h->data[parent]->readyq_time <= p->readyq_time)){
            break;
        }
        h->data[i] = h->data[parent];
        i = parent;
    }
    h->data[i] = p;
    return 0; //성공
}

int heapInsert_priority(MinHeap *h, Process *p){
    if (h->size >= MAX_PROCESS){
        return -1; // 실패
    }
    int i = h -> size++;
    while (i > 0){
        int parent = (i -1) / 2;
        Process *pp = h->data[parent];
        if (pp->priority < p->priority 
            || (pp->priority == p->priority
            && pp->readyq_time <= p->readyq_time)){
            break;
        }
        h->data[i] = pp;
        i = parent;
    }
    h->data[i] = p;
    return 0; //성공
}

Process* heapPop_NSJF(MinHeap *h){
    if (h->size == 0) return NULL;
    Process* min = h->data[0];
    Process* last = h->data[--h->size];

    int parent = 0;
    while (1){
        int left = 2 * parent + 1;
        int right = 2 * parent + 2;
        int child;

        if (left >= h->size) break;

        child = left;
        if (right < h->size) 
            if(h->data[right]->next_burst < h->data[left]->next_burst
            || (h->data[right]->next_burst == h->data[left]->next_burst
            && h->data[right]->readyq_time <= h->data[left]->readyq_time)){
                child = right;
            }

        if (h->data[child]->next_burst > last->next_burst
            || (last->next_burst == h->data[child]->next_burst
                && last->readyq_time <= h->data[child]->readyq_time)){
            break;
        }

        h->data[parent] = h->data[child];
        parent = child;
    }
    h->data[parent] = last;

    return min;
}

Process* heapPop_PSJF(MinHeap *h){
    if (h->size == 0) return NULL;
    Process* min = h->data[0];
    Process* last = h->data[--h->size];

    int parent = 0;
    while (1){
        int left = 2 * parent + 1;
        int right = 2 * parent + 2;
        int child;

        if (left >= h->size) break;

        child = left;
        if (right < h->size) 
            if(h->data[right]->remaining_time < h->data[left]->remaining_time
            || (h->data[right]->remaining_time == h->data[left]->remaining_time
            && h->data[right]->readyq_time <= h->data[left]->readyq_time)){
                child = right;
            }

        if (h->data[child]->remaining_time > last->remaining_time
            || (last->remaining_time == h->data[child]->remaining_time
                && last->readyq_time <= h->data[child]->readyq_time)){
            break;
        }

        h->data[parent] = h->data[child];
        parent = child;
    }
    h->data[parent] = last;

    return min;
}

Process* heapPop_priority(MinHeap *h){
    if (h->size == 0) return NULL;
    Process* min = h->data[0];
    Process* last = h->data[--h->size];

    int parent = 0;
    while (1){
        int left = 2 * parent + 1;
        int right = 2 * parent + 2;
        int child;

        if (left >= h->size) break;

        child = left;
        if (right < h->size){
            if(h->data[right]->priority < h->data[left]->priority 
                || (h->data[right]->priority == h->data[left]->priority 
                && h->data[right]->readyq_time <= h->data[left]->readyq_time)){
                child = right;
            }
        }
            

        if (h->data[child]->priority > last->priority 
            || (last->priority == h->data[child]->priority 
                && last->readyq_time <= h->data[child]->readyq_time)){
            break;
        }

        h->data[parent] = h->data[child];
        parent = child;
    }
    h->data[parent] = last;

    return min;
}

//Gantt 차트
int gantt_chart[MAX_TIME];
int gantt_length = 0;

static inline void Record_Gantt(int pid){
    if (gantt_length < MAX_TIME)
        gantt_chart[gantt_length++] = pid;
    else{
        printf("Gantt chart overflow");
        return;
    }
}

void Print_GanttChart(char algorithm[]){
    printf("\n<%s>\n", algorithm);
    printf("\n=== Gantt Chart ===\n");
    printf(" ");
    printf("---------------------------------------------\n");

    printf("|");
    for (int i=0; i<gantt_length; i++){
        if (gantt_chart[i] == 0){
            printf("IDLE|");
        } else{
            printf(" P%-2d|", gantt_chart[i]);
        }
    }
    printf("\n");

    printf(" ");
    printf("---------------------------------------------\n");

    printf("0");
    for (int i = 1; i <= gantt_length; i++) printf("   %2d", i);
    printf("\n\n");
}

void Print_Evaluation(int arr_waiting_time[], int arr_turnaround_time[], int num_process){
    int turnaround = 0;
    int waiting = 0;
    for (int i=0; i < num_process; i++){
        waiting += arr_waiting_time[i];
        turnaround += arr_turnaround_time[i];
    }
    printf("Average waiting time: %.2f\n", (float)waiting / (float)num_process);
    printf("Average turnaround time: %.2f\n", (float)turnaround / (float)num_process);
}


//전역 변수
Process processes[MAX_PROCESS];
Process backup_processes[MAX_PROCESS];
int process_count;
Queue ready_queue; // 이번 틱의 후보
Queue ready_next_queue; // 다음 틱의 후보
Queue waiting_queue;
MinHeap ready_heap;

void Create_Process(int n){
    process_count = n;

    for (int i = 0; i < n; i++){
        processes[i].pid = i + 1;
        processes[i].arrival_time = rand() % 50;
        processes[i].total_cpu_burst_time = 1 + rand() % 25;
        processes[i].remaining_time = processes[i].total_cpu_burst_time;

        int max_requests = processes[i].total_cpu_burst_time < MAX_IO
                            ? processes[i].total_cpu_burst_time
                            : MAX_IO;
        processes[i].io_count = rand() % (max_requests + 1);
        processes[i].io_index = 0;
        processes[i].total_io_time = 0;
        int last = 1;
        for (int j=0; j < processes[i].io_count; j++){
            int max_slot = processes[i].total_cpu_burst_time - last - (processes[i].io_count - j);
            if (max_slot <= 0){
                processes[i].io_count = j;
                break;
            }
            processes[i].io_request_times[j] = last + rand() % (max_slot) + 1;
            last = processes[i].io_request_times[j];
            processes[i].io_burst_times[j] = 1 + rand() % 5;
            processes[i].total_io_time += processes[i].io_burst_times[j];
        }
        processes[i].next_burst = (processes[i].io_count > 0)
            ? processes[i].io_request_times[0]
            : processes[i].total_cpu_burst_time;

        processes[i].priority = rand() % 10;
        processes[i].readyq_time = processes[i].arrival_time;

        processes[i].is_completed = 0;
        processes[i].start_time = -1;
    }
}

void Input_Process(int n){
    process_count = n;
    for (int i = 0; i < n; i++){
        processes[i].pid = i + 1;
        printf("P%d의 Arrival CPU Priority를 입력하시오.(예시:11 14 2)", processes[i].pid);
        scanf("%d %d %d",
              &processes[i].arrival_time,
              &processes[i].total_cpu_burst_time,
              &processes[i].priority);
        processes[i].remaining_time = processes[i].total_cpu_burst_time;

        int max_requests = processes[i].total_cpu_burst_time < MAX_IO
                            ? processes[i].total_cpu_burst_time
                            : MAX_IO;
        processes[i].io_count = rand() % (max_requests + 1);
        processes[i].io_index = 0;
        processes[i].total_io_time = 0;
        int last = 1;
        for (int j=0; j < processes[i].io_count; j++){
            int max_slot = processes[i].total_cpu_burst_time - last - (processes[i].io_count - j);
            if (max_slot <= 0){
                processes[i].io_count = j;
                break;
            }
            processes[i].io_request_times[j] = last + rand() % (max_slot) + 1;
            last = processes[i].io_request_times[j];
            processes[i].io_burst_times[j] = 1 + rand() % 5;
            processes[i].total_io_time += processes[i].io_burst_times[j];
        }

        // next_burst 초기화
        processes[i].next_burst = (processes[i].io_count > 0)
            ? processes[i].io_request_times[0]
            : processes[i].total_cpu_burst_time;
        processes[i].readyq_time = processes[i].arrival_time;
        processes[i].is_completed = 0;
        processes[i].start_time = -1;
    }
}

// 초기 process 정보를 백업해놓음
void SaveProcess(){
    for (int i=0; i < process_count; i++){
        backup_processes[i] = processes[i];
    }
}

void ResetProcess(){
    for (int i=0; i < process_count; i++){
        processes[i] = backup_processes[i];
    }
}

void ConfigQueue(){
    initQueue(&ready_queue);
    initQueue(&ready_next_queue); // io 종료 시 최소한 다음 틱에 실행되도록 설계함.
    initQueue(&waiting_queue);
    gantt_length = 0;
}

void ConfigMinHeap(){
    initMinHeap(&ready_heap);
    initQueue(&ready_next_queue); // io 종료 시 최소한 다음 틱에 실행되도록 설계함.
    initQueue(&waiting_queue);
    gantt_length = 0;
}


// 스케줄링 알고리즘들

void FCFS(){
    ConfigQueue();
    int current_time = 0;
    int num_completed_process = 0;
    int arr_waiting_time[MAX_PROCESS] = {0};
    int arr_turnaround_time[MAX_PROCESS] = {0};

    Process *current = NULL; // cpu에서 현재 실행 중인 process

    while (num_completed_process < process_count){

        //io 실행
        int wq_size = waiting_queue.size;
        for (int i=0; i<wq_size; i++){
            Process *p = dequeue(&waiting_queue);
            p->io_burst_times[p->io_index] --;
            if (p->io_burst_times[p->io_index] == 0){
                p->io_index ++;
                if (enqueue(&ready_next_queue, p) < 0){
                    printf("ready next queue overflow");
                    return;
                }
            } else{
                if (enqueue(&waiting_queue, p) < 0){
                    printf("waiting queue overflow");
                    return;
                }
            }
        }

        // 도착하면 ready queue에 추가
        for (int i=0; i<process_count; i++){
            if (!processes[i].is_completed && processes[i].arrival_time == current_time){
                if (enqueue(&ready_queue, &processes[i]) < 0){
                    printf("ready queue overflow");
                    return;
                }
            }
        }

        // process가 비었다면 process를 할당
        if (current == NULL){
            current = dequeue(&ready_queue);
            if (current && current->start_time == -1){
                current->start_time = current_time;
            }
        }

        // process 실행
        if (current){
            current->remaining_time--;
            Record_Gantt(current->pid);

            // io 작업 발생
            int executed = current->total_cpu_burst_time - current->remaining_time;
            if (current->remaining_time > 0 
                && current->io_index < current->io_count 
                && executed == current->io_request_times[current->io_index]){
                if (enqueue(&waiting_queue, current) < 0){
                    printf("waiting queue overflow");
                    return;
                }
                current = NULL;
            }

            // process 종료
            if (current != NULL && current->remaining_time == 0){
                current->completion_time = current_time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->total_cpu_burst_time - current->total_io_time;
                current->response_time = current->start_time - current->arrival_time;
                current->is_completed = 1;
                num_completed_process ++;

                arr_waiting_time[current->pid - 1] = current->waiting_time;
                arr_turnaround_time[current->pid - 1] = current->turnaround_time;
                current = NULL;
            }
        }
        else{ //idle
            Record_Gantt(0);
        }
        int rn_size = ready_next_queue.size;
        for (int i = 0; i < rn_size; i++) {
            Process *p = dequeue(&ready_next_queue);
            if (enqueue(&ready_queue, p) < 0) {
                printf("ready_queue overflow\n");
                return;
            }
        }    
        
        current_time++;

    }

    Print_GanttChart("FCFS");
    Print_Evaluation(arr_waiting_time, arr_turnaround_time, num_completed_process);

}

void non_preemptive_SJF(){
    ConfigMinHeap();
    int current_time = 0;
    int num_completed_process = 0;
    int arr_waiting_time[MAX_PROCESS] = {0};
    int arr_turnaround_time[MAX_PROCESS] = {0};

    Process *current = NULL; // cpu에서 현재 실행 중인 process

    while (num_completed_process < process_count){

        //io 실행
        int wq_size = waiting_queue.size;
        for (int i=0; i<wq_size; i++){
            Process *p = dequeue(&waiting_queue);
            p->io_burst_times[p->io_index] --;
            if (p->io_burst_times[p->io_index] == 0){ // io 종료
                p->io_index ++;
              

                //next_burst 계산
                if (p->io_index < p->io_count){
                    int prev_t = p->io_request_times[p->io_index-1];
                    int curr_t = p->io_request_times[p->io_index];
                    p->next_burst = curr_t - prev_t;
                } else{ // 마지막 io였음
                    int last_t = p->io_request_times[p->io_index-1];
                    p->next_burst = p->total_cpu_burst_time - last_t;
                }

                p->readyq_time = current_time;
                if (enqueue(&ready_next_queue, p) < 0){
                    printf("ready next queue overflow");
                    return;
                }
            } else{
                if (enqueue(&waiting_queue, p) < 0){
                    printf("waiting queue overflow");
                    return;
                }
            }
        }

        // 도착하면 ready heap에 추가
        for (int i=0; i<process_count; i++){
            if (!processes[i].is_completed && processes[i].arrival_time == current_time){
                processes[i].readyq_time = current_time;
                if (heapInsert_NSJF(&ready_heap, &processes[i]) < 0){
                    printf("ready heap overflow");
                    return;
                }
            }
        }

        // process가 비었다면 process를 할당
        if (current == NULL){
            current = heapPop_NSJF(&ready_heap);
            if (current && current->start_time == -1){
                current->start_time = current_time;
            }
        }

        // process 실행
        if (current){
            current->remaining_time--;
            Record_Gantt(current->pid);

            // io 작업 발생
            int executed = current->total_cpu_burst_time - current->remaining_time;
            if (current->remaining_time > 0 
                && current->io_index < current->io_count 
                && executed == current->io_request_times[current->io_index]){
                if (enqueue(&waiting_queue, current) < 0){
                    printf("waiting queue overflow");
                    return;
                }
                current = NULL;
            }

            // process 종료
            if (current != NULL && current->remaining_time == 0){
                current->completion_time = current_time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->total_cpu_burst_time - current->total_io_time;
                current->response_time = current->start_time - current->arrival_time;
                current->is_completed = 1;
                num_completed_process ++;

                arr_waiting_time[current->pid - 1] = current->waiting_time;
                arr_turnaround_time[current->pid - 1] = current->turnaround_time;
                current = NULL;
            }
        }
        else{ //idle
            Record_Gantt(0);
        }
        int rn_size = ready_next_queue.size;
        for (int i = 0; i < rn_size; i++) {
            Process *p = dequeue(&ready_next_queue);
            if (heapInsert_NSJF(&ready_heap, p) < 0) {
                printf("ready_heap overflow\n");
                return;
            }
        }   
        current_time++;

    }
    Print_GanttChart("non preemptive SJF");
    Print_Evaluation(arr_waiting_time, arr_turnaround_time, num_completed_process);

}

void preemptive_SJF(){
    ConfigMinHeap();
    int current_time = 0;
    int num_completed_process = 0;
    int arr_waiting_time[MAX_PROCESS] = {0};
    int arr_turnaround_time[MAX_PROCESS] = {0};

    Process *current = NULL; // cpu에서 현재 실행 중인 process

    while (num_completed_process < process_count){

        //io 실행
        int wq_size = waiting_queue.size;
        for (int i=0; i<wq_size; i++){
            Process *p = dequeue(&waiting_queue);
            p->io_burst_times[p->io_index] --;
            if (p->io_burst_times[p->io_index] == 0){ // io 종료
                p->io_index ++;
                
                p->readyq_time = current_time;
                if (enqueue(&ready_next_queue, p) < 0){
                    printf("ready next queue overflow");
                    return;
                }
            } else{
                if (enqueue(&waiting_queue, p) < 0){
                    printf("waiting queue overflow");
                    return;
                }
            }
        }

        // 도착하면 ready heap에 추가
        for (int i=0; i<process_count; i++){
            if (!processes[i].is_completed && processes[i].arrival_time == current_time){
                processes[i].readyq_time = current_time;
                if (heapInsert_PSJF(&ready_heap, &processes[i]) < 0){
                    printf("ready heap overflow");
                    return;
                }
            }
        }
        Process *top = (ready_heap.size > 0 ? ready_heap.data[0] : NULL);
        
        // process가 비었다면 process를 할당
        if (current == NULL){
            current = heapPop_PSJF(&ready_heap);
            if (current && current->start_time == -1){
                current->start_time = current_time;
            }
        } else if (top && (top->remaining_time < current->remaining_time)){
            current->readyq_time = current_time;
            if (heapInsert_PSJF(&ready_heap, current) < 0){
                    printf("ready heap overflow");
                    return;
                }
            current = heapPop_PSJF(&ready_heap);
            if (current->start_time == -1) current->start_time = current_time;
        }

        // process 실행
        if (current){
            current->remaining_time--;
            Record_Gantt(current->pid);

            // io 작업 발생
            int executed = current->total_cpu_burst_time - current->remaining_time;
            if (current->remaining_time > 0 
                && current->io_index < current->io_count 
                && executed == current->io_request_times[current->io_index]){
                if (enqueue(&waiting_queue, current) < 0){
                    printf("waiting queue overflow");
                    return;
                }
                current = NULL;
            }

            // process 종료
            if (current != NULL && current->remaining_time == 0){
                current->completion_time = current_time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->total_cpu_burst_time - current->total_io_time;
                current->response_time = current->start_time - current->arrival_time;
                current->is_completed = 1;
                num_completed_process ++;

                arr_waiting_time[current->pid - 1] = current->waiting_time;
                arr_turnaround_time[current->pid - 1] = current->turnaround_time;
                current = NULL;
            }
        }
        else{ //idle
            Record_Gantt(0);
        }
        int rn_size = ready_next_queue.size;
        for (int i = 0; i < rn_size; i++) {
            Process *p = dequeue(&ready_next_queue);
            if (heapInsert_PSJF(&ready_heap, p) < 0) {
                printf("ready_heap overflow\n");
                return;
            }
        }   
        current_time++;

    }
    Print_GanttChart("preemptive SJF");
    Print_Evaluation(arr_waiting_time, arr_turnaround_time, num_completed_process);
}

void non_preemptive_priority(){
    ConfigMinHeap();
    int current_time = 0;
    int num_completed_process = 0;
    int arr_waiting_time[MAX_PROCESS] = {0};
    int arr_turnaround_time[MAX_PROCESS] = {0};

    Process *current = NULL; // cpu에서 현재 실행 중인 process

    while (num_completed_process < process_count){

        //io 실행
        int wq_size = waiting_queue.size;
        for (int i=0; i<wq_size; i++){
            Process *p = dequeue(&waiting_queue);
            p->io_burst_times[p->io_index] --;
            if (p->io_burst_times[p->io_index] == 0){ // io 종료
                p->io_index ++;
                p->readyq_time = current_time;

                if (enqueue(&ready_next_queue, p) < 0){
                    printf("ready next queue overflow");
                    return;
                }
            } else{
                if (enqueue(&waiting_queue, p) < 0){
                    printf("waiting queue overflow");
                    return;
                }
            }
        }

        // 도착하면 ready heap에 추가
        for (int i=0; i<process_count; i++){
            if (!processes[i].is_completed && processes[i].arrival_time == current_time){
                processes[i].readyq_time = current_time;
                if (heapInsert_priority(&ready_heap, &processes[i]) < 0){
                    printf("ready heap overflow");
                    return;
                }
            }
        }

        // process가 비었다면 process를 할당
        if (current == NULL){
            current = heapPop_priority(&ready_heap);
            if (current && current->start_time == -1){
                current->start_time = current_time;
            }
        }

        // process 실행
        if (current){
            current->remaining_time--;
            Record_Gantt(current->pid);

            // io 작업 발생
            int executed = current->total_cpu_burst_time - current->remaining_time;
            if (current->remaining_time > 0 
                && current->io_index < current->io_count 
                && executed == current->io_request_times[current->io_index]){
                if (enqueue(&waiting_queue, current) < 0){
                    printf("waiting queue overflow");
                    return;
                }
                current = NULL;
            }

            // process 종료
            if (current != NULL && current->remaining_time == 0){
                current->completion_time = current_time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->total_cpu_burst_time - current->total_io_time;
                current->response_time = current->start_time - current->arrival_time;
                current->is_completed = 1;
                num_completed_process ++;

                arr_waiting_time[current->pid - 1] = current->waiting_time;
                arr_turnaround_time[current->pid - 1] = current->turnaround_time;
                current = NULL;
            }
        }
        else{ //idle
            Record_Gantt(0);
        }
        int rn_size = ready_next_queue.size;
        for (int i = 0; i < rn_size; i++) {
            Process *p = dequeue(&ready_next_queue);
            if (heapInsert_priority(&ready_heap, p) < 0) {
                printf("ready_heap overflow\n");
                return;
            }
        }   
        current_time++;

    }
    Print_GanttChart("non preemptive priority");
    Print_Evaluation(arr_waiting_time, arr_turnaround_time, num_completed_process);
}

void preemptive_priority(){
    ConfigMinHeap();
    int current_time = 0;
    int num_completed_process = 0;
    int arr_waiting_time[MAX_PROCESS] = {0};
    int arr_turnaround_time[MAX_PROCESS] = {0};

    Process *current = NULL; // cpu에서 현재 실행 중인 process

    while (num_completed_process < process_count){

        //io 실행
        int wq_size = waiting_queue.size;
        for (int i=0; i<wq_size; i++){
            Process *p = dequeue(&waiting_queue);
            p->io_burst_times[p->io_index] --;
            if (p->io_burst_times[p->io_index] == 0){ // io 종료, ready heap으로 복귀귀
                p->io_index ++;
                p->readyq_time = current_time;

                if (enqueue(&ready_next_queue, p) < 0){
                    printf("ready next queue overflow");
                    return;
                }
            } else{
                if (enqueue(&waiting_queue, p) < 0){
                    printf("waiting queue overflow");
                    return;
                }
            }
        }

        // 도착하면 ready heap에 추가
        for (int i=0; i<process_count; i++){
            if (!processes[i].is_completed && processes[i].arrival_time == current_time){
                processes[i].readyq_time = current_time;
                if (heapInsert_priority(&ready_heap, &processes[i]) < 0){
                    printf("ready heap overflow");
                    return;
                }
            }
        }

        Process *top = (ready_heap.size > 0 ? ready_heap.data[0] : NULL);

        // process가 비었다면 process를 할당
        if (current == NULL){
            current = heapPop_priority(&ready_heap);
            if (current && current->start_time == -1){
                current->start_time = current_time;
            }
        }
        else if (top && (top->priority < current->priority)){
            current->readyq_time = current_time;
            if (heapInsert_priority(&ready_heap, current) < 0){
                    printf("ready heap overflow");
                    return;
                }
            current = heapPop_priority(&ready_heap);
            if (current->start_time == -1)
                current->start_time = current_time;
        }

        // process 실행
        if (current){
            current->remaining_time--;
            Record_Gantt(current->pid);

            // io 작업 발생
            int executed = current->total_cpu_burst_time - current->remaining_time;
            if (current->remaining_time > 0 
                && current->io_index < current->io_count 
                && executed == current->io_request_times[current->io_index]){
                if (enqueue(&waiting_queue, current) < 0){
                    printf("waiting queue overflow");
                    return;
                }
                current = NULL;
            }

            // process 종료
            if (current != NULL && current->remaining_time == 0){
                current->completion_time = current_time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->total_cpu_burst_time - current->total_io_time;
                current->response_time = current->start_time - current->arrival_time;
                current->is_completed = 1;
                num_completed_process ++;

                arr_waiting_time[current->pid - 1] = current->waiting_time;
                arr_turnaround_time[current->pid - 1] = current->turnaround_time;
                current = NULL;
            }
        }
        else{ //idle
            Record_Gantt(0);
        }
        int rn_size = ready_next_queue.size;
        for (int i = 0; i < rn_size; i++) {
            Process *p = dequeue(&ready_next_queue);
            if (heapInsert_priority(&ready_heap, p) < 0) {
                printf("ready_heap overflow\n");
                return;
            }
        }   
        current_time++;

    }
    Print_GanttChart("preemptive priority");
    Print_Evaluation(arr_waiting_time, arr_turnaround_time, num_completed_process);
}

void round_robin(){
    ConfigQueue();
    int current_time = 0;
    int num_completed_process = 0;
    int arr_waiting_time[MAX_PROCESS] = {0};
    int arr_turnaround_time[MAX_PROCESS] = {0};

    Process *current = NULL; // cpu에서 현재 실행 중인 process
    int slice_used = 0;

    while (num_completed_process < process_count){

        //io 실행
        int wq_size = waiting_queue.size;
        for (int i=0; i<wq_size; i++){
            Process *p = dequeue(&waiting_queue);
            p->io_burst_times[p->io_index] --;
            if (p->io_burst_times[p->io_index] == 0){
                p->io_index ++;
                if (enqueue(&ready_next_queue, p) < 0){
                    printf("ready next queue overflow");
                    return;
                }
            } else{
                if (enqueue(&waiting_queue, p) < 0){
                    printf("waiting queue overflow");
                    return;
                }
            }
        }

        // 도착하면 ready queue에 추가
        for (int i=0; i<process_count; i++){
            if (!processes[i].is_completed && processes[i].arrival_time == current_time){
                if (enqueue(&ready_queue, &processes[i]) < 0){
                    printf("ready queue overflow");
                    return;
                }
            }
        }

        // process가 비었다면 process를 할당당
        if (current == NULL){
            current = dequeue(&ready_queue);
            slice_used = 0;
            if (current && current->start_time == -1){
                current->start_time = current_time;
            }
        }

        // process 실행
        if (current){
            current->remaining_time--;
            slice_used ++;
            Record_Gantt(current->pid);

            // io 작업 발생
            int executed = current->total_cpu_burst_time - current->remaining_time;
            if (current->remaining_time > 0 
                && current->io_index < current->io_count 
                && executed == current->io_request_times[current->io_index]){
                if (enqueue(&waiting_queue, current) < 0){
                    printf("waiting queue overflow");
                    return;
                }
                current = NULL;
                slice_used = 0;
            }

            // process 종료
            if (current != NULL && current->remaining_time == 0){
                current->completion_time = current_time + 1;
                current->turnaround_time = current->completion_time - current->arrival_time;
                current->waiting_time = current->turnaround_time - current->total_cpu_burst_time - current->total_io_time;
                current->response_time = current->start_time - current->arrival_time;
                current->is_completed = 1;
                num_completed_process ++;

                arr_waiting_time[current->pid - 1] = current->waiting_time;
                arr_turnaround_time[current->pid - 1] = current->turnaround_time;
                current = NULL;
                slice_used = 0;
            } else if(slice_used == TIME_QUANTUM){ // time quantum 넘어감감
                enqueue(&ready_queue, current);
                current = NULL;
                slice_used = 0;
            }
        }
        else{ //idle
            Record_Gantt(0);
            slice_used = 0;
        }
        int rn_size = ready_next_queue.size;
        for (int i = 0; i < rn_size; i++) {
            Process *p = dequeue(&ready_next_queue);
            if (enqueue(&ready_queue, p) < 0) {
                printf("ready_queue overflow\n");
                return;
            }
        }   
        current_time++;

    }
    Print_GanttChart("Round Robin");
    Print_Evaluation(arr_waiting_time, arr_turnaround_time, num_completed_process);
}


void Print_Processes() {
    printf("PID\tArrival\tCPU\tPriority\tio_request_1\tio_burst_1\tio_request_2\tio_burst_2\tio_request_3\tio_burst_3\n");
    for (int i = 0; i < process_count; i++) {
        printf("%3d\t%7d\t%3d\t%8d\t",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].total_cpu_burst_time,
               processes[i].priority);
        for (int j=0; j<3; j++){
            if (j<processes[i].io_count){
                printf("%12d\t%10d\t",processes[i].io_request_times[j], processes[i].io_burst_times[j]);
            }
            else{
                printf("%12s\t%10s\t", "*", "*");
            }
        }
        printf("\n");
    }

}


int main() {
    int n;
    printf("20 이하 프로세스 수 입력: ");
    scanf("%d", &n);
    if (n<1 || n > MAX_PROCESS){
        printf(" 20 이하의 프로세스 수를 입력하시오.");
        return -1;
    }

    int mode;
    printf("모드 선택: 1: 랜덤 생성 2: 사용자 입력\n");
    scanf("%d", &mode);

    if (mode == 1) {
        srand(43);
        //srand(time(NULL));
        Create_Process(n);

    } else if (mode ==2){
        Input_Process(n);
    } else{
        printf("1 또는 2를 입력하시오.");
        return -1;
    }


    SaveProcess();
    Print_Processes();

    FCFS();

    ResetProcess();
    non_preemptive_SJF(); //next cpu burst time을 기준으로 함.

    ResetProcess();
    preemptive_SJF(); // remaining time을 기준으로 함.

    ResetProcess();
    non_preemptive_priority(); // priority가 작은 순서대로 실행하는 알고리즘

    ResetProcess();
    preemptive_priority(); // priority가 작은 순서대로 실행하는 알고리즘

    ResetProcess();
    round_robin(); // time quantum=5
    return 0;
}