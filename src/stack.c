#include "stack.h"
#include <sys/types.h>

void push_stack(stack_node_t** head, pid_t pid) {
    stack_node_t *new = (stack_node_t*) malloc(sizeof(stack_node_t));
    if (new == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    new->next = *head;
    new->pid = pid;
    *head = new;
}
void pop_stack(stack_node_t** head) {
    if(*head!=NULL) {
        stack_node_t *temp = *head;
        *head = (*head)->next;
        free(temp);
    }
}

pid_t get_top_pid(stack_node_t* head) {
    return (head != NULL) ? head->pid : -1;
}

void free_stack(stack_node_t** head) {
    while (*head != NULL) {
        pop_stack(head);
    }
}

size_t stack_size(stack_node_t* head) {
    size_t count = 0;
    stack_node_t* current = head;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}