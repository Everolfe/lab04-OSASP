#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "ring.h"
#include <time.h>
#include "stack.h"
#define BUFFER_SIZE 15

stack_node_t* producers = NULL;
stack_node_t* consumers = NULL;


sem_t *items;
sem_t *free_space;
sem_t *mutex_ring;
sem_t *mutex_stack;
volatile bool IS_RUNNING = true;

void producer(int32_t);
void consumer(int32_t);
message_t generate_message(void);
void handler_stop_proc();
void display_message(const message_t *message, pid_t pid, bool is_producer, size_t count);
void initialize_semaphores(void);
void menu(ring_t *ring_queue);
void close_semaphores(void);
uint16_t crc16(const uint8_t *data, size_t length);

int main(void) {
    srand(time(NULL));

    signal(SIGTERM, handler_stop_proc);
    signal(SIGINT, handler_stop_proc);

    initialize_semaphores();

    ring_t *ring_queue = NULL;
    for (size_t i = 0; i < BUFFER_SIZE; ++i)
        allocate_node(&ring_queue);

    printf("Shmid segment : %d\n", ring_queue->shmid);

    menu(ring_queue);
    free_stack(&producers);
    free_stack(&consumers);
    close_semaphores();
    return 0;
}

void initialize_semaphores(void) {
    sem_unlink("free_space");
    sem_unlink("items");
    sem_unlink("mutex_ring");
    sem_unlink("mutex_stack");

    free_space = sem_open("free_space", O_CREAT, 0777, 0);
    if (free_space == SEM_FAILED) {
        perror("Failed to open free_space semaphore");
        exit(EXIT_FAILURE);
    }

    items = sem_open("items", O_CREAT, 0777, BUFFER_SIZE);
    if (items == SEM_FAILED) {
        perror("Failed to open items semaphore");
        exit(EXIT_FAILURE);
    }

    mutex_ring = sem_open("mutex_ring", O_CREAT, 0777, 1);
    if (mutex_ring == SEM_FAILED) {
        perror("Failed to open mutex_ring semaphore");
        exit(EXIT_FAILURE);
    }

    mutex_stack = sem_open("mutex_stack", O_CREAT, 0777, 1);
    if (mutex_stack == SEM_FAILED) {
        perror("Failed to open mutex_stack semaphore");
        exit(EXIT_FAILURE);
    }
}

void menu(ring_t *ring_queue) {
    int status;
    char ch;
    do {
        printf("##########################");
        printf("\nSelect an action:\n");
        printf("p - Add a producer\n");
        printf("c - Add a consumer\n");
        printf("d - Delete last producer\n");
        printf("k - Delete last consumer\n");
        printf("s - Show status\n");
        printf("q - Quit\n");
        printf("##########################\n");
        ch = getchar();
        switch (ch) {
            case 'p': {
                pid_t pid = fork();
                if (pid == 0) {
                    producer(ring_queue->shmid);
                } else {
                    push_stack(&producers, pid);
                    printf("Producer added (PID: %d)\n", pid);
                }
                break;
            }
            case 'c': {
                pid_t pid = fork();
                if (pid == 0) {
                    consumer(ring_queue->shmid);
                } else {
                    push_stack(&consumers, pid);
                    printf("Consumer added (PID: %d)\n", pid);
                }
                break;
            }
            case 'd': {
                pid_t pid = get_top_pid(producers);
                if (pid != -1) {
                    kill(pid, SIGTERM);
                    waitpid(pid, NULL, 0);
                    pop_stack(&producers);
                    printf("Producer (PID: %d) terminated\n", pid);
                } else {
                    printf("No producers to delete\n");
                }
                break;
            }
            case 'k': {
                pid_t pid = get_top_pid(consumers);
                if (pid != -1) {
                    kill(pid, SIGTERM);
                    waitpid(pid, NULL, 0);
                    pop_stack(&consumers);
                    printf("Consumer (PID: %d) terminated\n", pid);
                } else {
                    printf("No consumers to delete\n");
                }
                break;
            }
            case 'q': {
                // Завершаем всех производителей
                stack_node_t* current = producers;
                while (current != NULL) {
                    kill(current->pid, SIGTERM);
                    current = current->next;
                }
                
                // Завершаем всех потребителей
                current = consumers;
                while (current != NULL) {
                    kill(current->pid, SIGTERM);
                    current = current->next;
                }
                
                // Даем время на завершение
                sleep(1);
                
                // Убиваем оставшиеся процессы
                current = producers;
                while (current != NULL) {
                    kill(current->pid, SIGKILL);
                    current = current->next;
                }
                
                current = consumers;
                while (current != NULL) {
                    kill(current->pid, SIGKILL);
                    current = current->next;
                }
                
                clear_buff(ring_queue);
                IS_RUNNING = false;
                break;
            }
            case 's': {
                int free_slots, used_slots;
                sem_getvalue(items, &free_slots);
                sem_getvalue(free_space, &used_slots);
                size_t producer_count = stack_size(producers);
                size_t consumer_count = stack_size(consumers);
                printf("\n=== Queue Status ===\n");
                printf("  Buffer size:    %d\n", BUFFER_SIZE);
                printf("  Free slots:     %d\n", free_slots);
                printf("  Used slots:     %d\n", used_slots);
                printf("  Active producers: %zu\n", producer_count);
                printf("  Active consumers: %zu\n", consumer_count);
                printf("  Total produced: %zu\n", ring_queue->produced);
                printf("  Total consumed: %zu\n", ring_queue->consumed);
                printf("===================\n");
                break;
            }
            default: {
                printf("Incorrect input.\n");
                fflush(stdin);
                break;
            }
        }
        waitpid(-1, &status, WNOHANG);
        getchar();
    } while (IS_RUNNING);
}

void close_semaphores(void) {
    sem_unlink("free_space");
    sem_unlink("items");
    sem_unlink("mutex_ring");
    sem_unlink("mutex_stack");

    sem_close(mutex_ring);
    sem_close(mutex_stack);
    sem_close(items);
    sem_close(free_space);

    printf("Semaphores closed and unlinked.\n");
}

message_t generate_message(void) {
    message_t message = {
            .data = {0},
            .hash = 0,
            .size = 0,
            .type = 0
    };

    do {
        message.size = rand() % 257;
    } while (message.size == 0);

    size_t realSize = message.size;
    if (realSize == 256) {
        message.size = 0;
        realSize = (message.size == 0) ? 256 : message.size;
    }

    message.hash = crc16(message.data,realSize); 

    return message;
}

void display_message(const message_t *message, pid_t pid, bool is_producer, size_t count) {
    if (is_producer) {
        printf("pid: %d produce msg: hash=%04X (total: %zu)\n", pid, message->hash, count);
    } else {
        printf("pid: %d consume msg: hash=%04X (total: %zu)\n", pid, message->hash, count);
    }
}
void handler_stop_proc() {
    IS_RUNNING = false;
}

void consumer(int32_t shmid) {
    ring_t *queue = shmat(shmid, NULL, 0);
    pid_t my_pid = getpid();
    bool was_skipped = false;  // Флаг для отслеживания пропусков
    
    do {
        int available_messages;
        sem_getvalue(free_space, &available_messages);
        
        if (available_messages <= 0) {
            if (!was_skipped) {
                printf("pid: %d skipping - no messages available\n", my_pid);
                was_skipped = true;
            }
            sleep(1);
            continue;
        } else {
            was_skipped = false;  // Сбрасываем флаг, когда сообщения появляются
        }
        
        sem_wait(free_space);
        sem_wait(mutex_ring);
        
        if (!IS_RUNNING) {
            sem_post(mutex_ring);
            sem_post(free_space);
            break;
        }
        
        message_t *message = pop_message(queue);
        if (message != NULL) {
            display_message(message, my_pid, false, queue->consumed);
            free(message);
        }
        
        sem_post(mutex_ring);
        sem_post(items);
        
        sleep(1);
        
    } while (IS_RUNNING);
    shmdt(queue);
}

void producer(int32_t shmid) {
    ring_t *queue = shmat(shmid, NULL, 0);
    pid_t my_pid = getpid();
    bool was_skipped = false;  // Флаг для отслеживания пропусков
    
    do {
        int free_slots;
        sem_getvalue(items, &free_slots);
        
        if (free_slots <= 0) {
            if (!was_skipped) {
                printf("pid: %d skipping - no free slots\n", my_pid);
                was_skipped = true;
            }
            sleep(1);
            continue;
        } else {
            was_skipped = false;  // Сбрасываем флаг, когда слоты освобождаются
        }
        
        sem_wait(items);
        sem_wait(mutex_ring);
        
        if (!IS_RUNNING) {
            sem_post(mutex_ring);
            sem_post(items);
            break;
        }
        
        message_t new_message = generate_message();
        push_message(queue, &new_message);
        
        display_message(&new_message, my_pid, true, queue->produced);
        
        sem_post(mutex_ring);
        sem_post(free_space);
        
        sleep(1);
        
    } while (IS_RUNNING);
    shmdt(queue);
}

uint16_t crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;  // Initial value
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;  
            else
                crc <<= 1;
        }
    }
    return crc;
}
