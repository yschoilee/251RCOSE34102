
````markdown
# CPU Scheduler Simulator (`cpu_scheduler.c`)

이 프로그램은 단일 CPU 환경에서 여러 스케줄링 알고리즘을 틱 단위 시뮬레이션하며,  
각 스케줄링 알고리즘의 동작 결과( Gantt Chart, 평균 대기시간, 평균 반환시간 )를 출력하는 도구입니다.

지원 알고리즘은 다음 9가지입니다.

1. FCFS  
2. Non-preemptive SJF  
3. Preemptive SJF  
4. Non-preemptive Priority  
5. Preemptive Priority  
6. Round Robin (Time Quantum = 5)  
7. Lottery Scheduling  
8. Non-preemptive Priority with Aging  
9. Preemptive EDF (Earliest Deadline First)

---

# 1. 컴파일 및 실행

### 컴파일
```bash
gcc cpu_scheduler.c -o cpu_scheduler
````

### 실행

```bash
./cpu_scheduler
```

프로그램은 다음과 같은 순서로 입력을 요구합니다.

1. **프로세스 개수 입력**

   ```
   Enter number of processes (<=20):
   ```

2. **프로세스 생성 모드 선택**

   ```
   Enter mode selection: 1: random 2: user input
   ```

   * `1` : 모든 프로세스를 랜덤 생성
   * `2` : Arrival, CPU burst, Priority만 사용자 입력(I/O 정보는 내부에서 랜덤 생성)

입력 후, 프로세스 테이블이 출력되며 9개의 알고리즘이 순차적으로 자동 실행됩니다.

---

# 2. Process 구조체

코드에서 사용되는 `Process` 구조체의 주요 필드는 다음과 같습니다.

| 필드                     | 설명                                  |
| ---------------------- | ----------------------------------- |
| `pid`                  | 프로세스 ID                             |
| `arrival_time`         | 도착 시간                               |
| `total_cpu_burst_time` | 총 CPU 실행 시간                         |
| `remaining_time`       | 남은 CPU 실행 시간                        |
| `next_burst`           | 다음 CPU burst 길이 (SJF-Nonpreemptive) |
| `io_count`             | I/O 요청 개수 (최대 3개)                   |
| `io_request_times[]`   | 각 I/O 요청이 발생하는 CPU 사용 누적 시점         |
| `io_burst_times[]`     | 각 I/O 요청의 I/O 수행 시간                 |
| `io_index`             | 현재 진행 중인 I/O 인덱스                    |
| `total_io_time`        | 전체 I/O 시간 합                         |
| `priority`             | 우선순위 (값이 작을수록 높은 우선순위)              |
| `readyq_time`          | Ready Queue(혹은 heap)에 들어온 시간        |
| `last_aged_time`       | 마지막으로 aging이 적용된 시간                 |
| `start_time`           | CPU를 처음 할당받은 시간                     |
| `completion_time`      | 종료 시간                               |
| `turnaround_time`      | 반환 시간                               |
| `waiting_time`         | 대기 시간                               |
| `response_time`        | 응답 시간                               |
| `is_completed`         | 완료 여부                               |
| `ticket_count`         | Lottery Scheduling 티켓 수             |
| `deadline`             | EDF deadline                        |
| `missed`               | EDF에서 deadline miss 여부 (1: miss됨)   |

---

# 3. 프로세스 생성 방식

## 3.1 랜덤 생성 모드 (`mode == 1`)

프로세스를 다음 규칙에 따라 랜덤 생성합니다.

* `arrival_time` : 0 ~ 49
* `total_cpu_burst_time` : 1 ~ 25
* I/O 요청 개수 : 0 ~ min(CPU burst, MAX_IO)
* 각 I/O 요청:

  * 이전 I/O 이후의 가능한 구간에서 랜덤 시점 배치
  * I/O burst 시간은 1 ~ 5
* `priority` : 0 ~ 9
* `ticket_count` : 1 ~ MAX_TICKETS (10)
* `deadline` :

  ```
  arrival_time + 1.5 * (total_cpu_burst_time + total_io_time)
  ```

  ※ 정수형 저장으로 소수점은 버려짐
* `next_burst` :

  * I/O가 있으면 첫 I/O 요청까지의 CPU 실행량
  * I/O가 없으면 전체 CPU burst

## 3.2 사용자 입력 모드 (`mode == 2`)

사용자가 입력하는 정보:

```
Arrival, Total CPU Time, Priority
```

I/O 관련 정보(`io_count`, `io_request_times`, `io_burst_times`)는 동일한 방식으로 내부에서 랜덤 생성됩니다.

---

# 4. 자료구조

## 4.1 Queue (Ready Queue, Waiting Queue)

* 원형 큐 기반
* 함수: `enqueue`, `dequeue`, `isEmpty`, `isFull`, `remove_from_queue`
* Lottery Scheduling에서 `remove_from_queue()` 사용

## 4.2 Min-Heap (Ready Heap)

알고리즘별 정렬 기준에 따라 다른 삽입/삭제 함수를 사용합니다.

| 알고리즘                | 기준 값             |
| ------------------- | ---------------- |
| Non-preemptive SJF  | `next_burst`     |
| Preemptive SJF      | `remaining_time` |
| Priority Scheduling | `priority`       |
| EDF                 | `deadline`       |

티브레이커는 모두 `readyq_time`(먼저 대기한 프로세스 우선)을 사용합니다.

### Aging

* Non-preemptive Priority with Aging에서
  ready heap에 있는 모든 프로세스의 우선순위를 일정 주기(`AGING_INTERVAL = 5`)마다 감소(`AGING_AMOUNT = 1`)
* aging 후 `rebuild_priority_heap()` 로 heap 전체 재정렬

---

# 5. 스케줄링 알고리즘

모든 알고리즘은 다음 공통 흐름으로 동작합니다.

1. waiting queue에서 I/O 작업 진행
2. 도착한 프로세스를 ready queue/heap에 삽입
3. CPU에서 실행할 프로세스를 결정
4. 1 tick 실행 및 I/O 요청 체크
5. 종료 시 통계 값 계산
6. Gantt Chart 기록

## 5.1 FCFS

* Queue 기반
* 도착 순서대로 처리
* 선점 없음

## 5.2 Non-preemptive SJF

* 다음 CPU burst 길이(`next_burst`) 기준
* I/O 발생 시 다음 구간 길이를 재계산하여 heap에 삽입

## 5.3 Preemptive SJF

* `remaining_time`이 더 짧은 프로세스가 도착하면 즉시 선점

## 5.4 Non-preemptive Priority

* `priority`가 작은 프로세스 우선
* 선점 없음

## 5.5 Preemptive Priority

* 더 높은 우선순위(더 작은 priority)를 가진 프로세스가 도착하면 선점

## 5.6 Round Robin

* Time Quantum = 5
* quantum을 소진하면 ready queue 뒤로 이동

## 5.7 Lottery Scheduling

* ready queue의 모든 프로세스의 `ticket_count` 합으로 난수 추첨
* 추첨된 프로세스는 queue에서 제거 후 CPU 실행

## 5.8 Non-preemptive Priority + Aging

* 일정 시간마다 priority 감소하여 starvation 방지
* 감소량은 `AGING_AMOUNT = 1`
* 감소 후 heap 재정렬

## 5.9 Preemptive EDF (Earliest Deadline First)

* 가장 이른 deadline을 가진 프로세스 우선
* 다음 두 경우에 deadline miss 처리함:

### 1) **Ready Heap에 있는 프로세스가 deadline을 초과한 경우**

```c
missed->missed = 1;
missed->is_completed = 1;
arr_waiting_time[...] = 0;
arr_turnaround_time[...] = 0;
```

### 2) **CPU에서 실행 중이던 프로세스가 deadline을 초과한 경우**

```c
current->is_completed = 1;
arr_waiting_time[...] = 0;
arr_turnaround_time[...] = 0;
```

‼ **이 경우 코드상 `missed = 1`을 설정하지 않습니다.**

→ 따라서 이 프로세스는 `missed == 0`이지만 waiting/turnaround가 모두 0으로 기록됩니다.

### Evaluation 계산 방식 (코드 그대로)

```c
if (process.is_completed && !process.missed)
    평균 계산에 포함됨
```

즉:

* Ready Heap에서 miss된 프로세스 → 제외
* CPU에서 miss된 프로세스 → 포함되지만 waiting/turnaround=0

이는 코드의 실제 동작이며, README도 이대로 기술합니다.

---

# 6. 출력 형식

## 6.1 프로세스 테이블

예시:

```
PID Arrival CPU Priority io_request_1 io_burst_1 ...
 1      10  14        2           5          3         *          *
```

I/O가 없는 항목은 `*`로 출력됩니다.

## 6.2 Gantt Chart

형식:

```
=== Gantt Chart ===
 ---------------------------------------------
| P 1| P 1|IDLE| P 2| ... |
 ---------------------------------------------
0   1   2   3   ...
```

## 6.3 통계 출력

```
Average waiting time: xx.xx
Average turnaround time: yy.yy
```

EDF의 경우:

* Ready Heap에서 miss된 프로세스는 제외
* CPU 실행 중이던 상태에서 deadline miss된 프로세스는 waiting=0, turnaround=0으로 포함됨

---

# 7. 주요 상수

| 상수               | 의미                          |
| ---------------- | --------------------------- |
| `MAX_PROCESS`    | 최대 프로세스 수(20)               |
| `MAX_IO`         | 프로세스당 최대 I/O 요청 수(3)        |
| `MAX_TIME`       | Gantt Chart 최대 길이(1000)     |
| `TIME_QUANTUM`   | Round Robin time slice (=5) |
| `MAX_TICKETS`    | Lottery 티켓 최대개수 (=10)       |
| `AGING_INTERVAL` | Priority aging 주기 (=5)      |
| `AGING_AMOUNT`   | aging 시 priority 감소량 (=1)   |

---
