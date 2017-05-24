#ifndef _PQ_H
#define _PQ_H

/*
 * The following declarations are used for a simple priority-queue 
 * data structure based on a linked list. 
 */

typedef struct e_node e_node;
typedef struct e_list e_list;


e_list* new_list();
void    free_list(e_list*);

void    push(e_list*, float, int);
e_node* peek(e_list*);
e_node* pop(e_list*);

void    print_list(e_list*);

float   get_event_time(e_node*);
int     get_event_type(e_node*);
int     is_empty(e_list*);

#endif // _PQ_H