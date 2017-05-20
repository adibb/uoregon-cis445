#include <stdlib.h>
#include "pq.h"

// Definition of an event node.
struct e_node {
    float time;
    int type;
    e_node* next;
};

// Definition of an event list.
struct e_list {
    e_node* head;
    e_node* tail;
};

// Allocate a new event list.
e_list* new_list(){
    e_list *el;
    if ((el = (e_list *) malloc(sizeof(e_list))) != NULL) {
        el->head = NULL;
        el->tail = NULL;
    }
    return el;
}

// Free the passed event list.
void free_list(e_list *el){
    while (el->head != NULL){
        e_node *en = pop(el);
        free(en);
    }
    free(el);
    el = NULL;
}

// Push a new event node onto the list.
void push(e_list *el, float time, int type){
    
    // Allocate a new event node
    e_node *en;
    if ((en = (e_node *) malloc(sizeof(e_node))) != NULL) {
        en->time = time;
        en->type = type;
        en->next = NULL;
    }
    
    // Add it to the head of the list and bubble down
    // DEVNOTE: Replace with faster implementation if there's time.
    if (el->head == NULL){
        el->head = en;
        el->tail = en;
    } else {
        e_node *insertion = el->head;
        while (insertion->next != NULL && insertion->next->time < en->time){
            insertion = insertion->next;
        }
        en->next = insertion->next;
        insertion->next = en;
    }
}

// Peek at the head of the list.
e_node* peek(e_list *el){
    return el->head;
}

// Pop the head of the list.
e_node* pop(e_list *el){
    e_node *result = el->head;
    el->head = result->next;
    result->next = NULL;
    return result;
}

// Get the event time from a node.
float get_event_time(e_node *en){
    return en->time;
}

// Get the event type from a node.
int get_event_type(e_node *en){
    return en->type;
}

// Check if the event list is empty.
int is_empty(e_list *el){
    return (el->head == NULL);
}