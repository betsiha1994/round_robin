#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_CALLS 100
#define MAX_AGENTS 10

typedef struct {
    int id;
    int remainingTime;
} Call;

/* Shared ready queue */
Call queue[MAX_CALLS];
int front = 0, rear = 0;

/* Synchronization */
pthread_mutex_t queueLock;
pthread_mutex_t timeLock;

int timeQuantum;
int currentTime = 0;
int nextCallId = 1;
int totalCalls;
int activeCalls = 0;
int producerDone = 0;

/* Enqueue */
void enqueue(Call c) {
    queue[rear++] = c;
}

/* Dequeue */
Call dequeue() {
    return queue[front++];
}

/* Check empty */
int isEmpty() {
    return front == rear;
}

/* ---------------- PRODUCER (Random Inter-Arrival) ---------------- */
void* callProducer(void* arg) {
    while (nextCallId <= totalCalls) {

        int interArrival = rand() % 4 + 1;  // 1–4 seconds
        sleep(interArrival);

        Call c;
        c.id = nextCallId++;
        c.remainingTime = rand() % 6 + 2;   // 2–7 minutes service time

        pthread_mutex_lock(&queueLock);
        enqueue(c);
        activeCalls++;
        pthread_mutex_unlock(&queueLock);

        pthread_mutex_lock(&timeLock);
        printf("[Time %d] New Call %d arrived (service %d)\n",
               currentTime, c.id, c.remainingTime);
        pthread_mutex_unlock(&timeLock);
    }

    producerDone = 1;
    return NULL;
}

/* ---------------- AGENT THREAD ---------------- */
void* agentWork(void* arg) {
    int agentId = *(int*)arg;

    while (1) {
        pthread_mutex_lock(&queueLock);

        if (isEmpty()) {
            if (producerDone && activeCalls == 0) {
                pthread_mutex_unlock(&queueLock);
                break;
            }
            pthread_mutex_unlock(&queueLock);
            usleep(100000); // short wait
            continue;
        }

        Call c = dequeue();
        pthread_mutex_unlock(&queueLock);

        pthread_mutex_lock(&timeLock);
        printf("Agent %d serving Call %d at time %d\n",
               agentId, c.id, currentTime);

        int served = (c.remainingTime > timeQuantum)
                     ? timeQuantum
                     : c.remainingTime;

        currentTime += served;
        c.remainingTime -= served;
        pthread_mutex_unlock(&timeLock);

        sleep(1);  // simulate work

        if (c.remainingTime > 0) {
            pthread_mutex_lock(&queueLock);
            enqueue(c);
            pthread_mutex_unlock(&queueLock);

            printf("Call %d paused, remaining %d\n",
                   c.id, c.remainingTime);
        } else {
            pthread_mutex_lock(&queueLock);
            activeCalls--;
            pthread_mutex_unlock(&queueLock);

            printf("Call %d completed\n", c.id);
        }
    }

    return NULL;
}

void welcomePage() {
    printf("=========================================\n");
    printf("   CALL CENTER ROUND ROBIN SIMULATION\n");
    printf("=========================================\n");
    printf(" Features:\n");
    printf(" - Random inter-arrival calls\n");
    printf(" - Multiple parallel agents (threads)\n");
    printf(" - Round Robin scheduling\n");
    printf(" - Real-life call center behavior\n");
    printf("=========================================\n");
    printf(" Press ENTER to start simulation...\n");
    getchar(); // wait for user
}


/* ---------------- MAIN ---------------- */
int main() {
    int numAgents;

    srand(time(NULL));
     welcomePage(); 

    printf("Enter total number of calls to simulate: ");
    scanf("%d", &totalCalls);

    printf("Enter number of agents: ");
    scanf("%d", &numAgents);

    printf("Enter time quantum: ");
    scanf("%d", &timeQuantum);

    pthread_mutex_init(&queueLock, NULL);
    pthread_mutex_init(&timeLock, NULL);

    pthread_t producer;
    pthread_t agents[MAX_AGENTS];
    int agentIds[MAX_AGENTS];

    pthread_create(&producer, NULL, callProducer, NULL);

    for (int i = 0; i < numAgents; i++) {
        agentIds[i] = i + 1;
        pthread_create(&agents[i], NULL, agentWork, &agentIds[i]);
    }

    pthread_join(producer, NULL);

    for (int i = 0; i < numAgents; i++) {
        pthread_join(agents[i], NULL);
    }

    pthread_mutex_destroy(&queueLock);
    pthread_mutex_destroy(&timeLock);

    printf("\nAll calls processed with RANDOM inter-arrival and TRUE parallel agents.\n");
    return 0;
}
