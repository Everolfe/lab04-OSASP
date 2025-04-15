#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct stack_node {
    pid_t pid;
    struct stack_node* next;
} stack_node_t;

void push_stack(stack_node_t**, pid_t);
void pop_stack(stack_node_t**);
pid_t get_top_pid(stack_node_t* head);
void free_stack(stack_node_t** head);
size_t stack_size(stack_node_t* head);