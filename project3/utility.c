#include <stdio.h>
#include <stdlib.h>
#include "utility.h"

// Function Definitions using Linked List //

//This function is used to create the linked list
linked_list * create_linked_list()
{
	linked_list * new_ll =  (linked_list*) malloc(sizeof(linked_list));
	new_ll->head = NULL;
	new_ll->tail = NULL;
	new_ll->size = 0;
	return new_ll;
}



//this function is used to create new node for the linked list
node* create_node(void* data)
{
	node* new_node = (node*) malloc(sizeof(node));
	new_node->data = data;
	new_node->next = NULL;
	new_node->prev = NULL;
	return new_node;
}

//This function add a new node to an existing Linked List
void add_node(linked_list * ll, void * data)
{
	node * new_node = create_node(data);
	if(ll->size == 0)
	{
		ll->head = new_node;
		ll->tail = new_node;
		ll->size = 1 ;
	} else {
		new_node->prev = ll->tail;
		ll->tail->next = new_node;
		ll->tail = new_node;
		ll->size += 1;
	}
}

//// This funciton removes a node to an existing Linked List // //
void remove_data(linked_list* ll, void * data)
{
	node* current_node = ll->head;

	while(current_node != NULL && current_node->data != data) {
		current_node = current_node->next;
	}

	if(current_node != NULL) {
		if(current_node->prev != NULL) {
			current_node->prev->next = current_node->next;
		}
		if(current_node->next != NULL) {
			current_node->next->prev = current_node->prev;
		}
		if(ll->head == current_node) {
			ll->head = current_node->next;
		}
		if(ll->tail == current_node) {
			ll->tail = current_node->prev;
		}
		ll->size --;
		free(current_node);
	}
}

void remove_node(linked_list* ll, node * current_node) {
	if(current_node != NULL) {
		if(current_node->prev != NULL) {
			current_node->prev->next = current_node->next;
		}
		if(current_node->next != NULL) {
			current_node->next->prev = current_node->prev;
		}
		if(ll->head == current_node) {
			ll->head = current_node->next;
		}
		if(ll->tail == current_node) {
			ll->tail = current_node->prev;
		}
		ll->size --;
		free(current_node);
	}
}

// This funciton removes the head from an existing Linked List //
void remove_head(linked_list* ll)
{
	node * current_node = ll->head;
	if(current_node != NULL) {
		ll->head = current_node->next;
		if(ll->tail == current_node) {
			ll->tail = current_node->prev;
		}
		ll->size --;
		free(current_node);
	}
}

// This funciton add a node  at an speficied location //
void add_after(linked_list* ll, node *after_node, void *data)
{
	node* new_node = create_node(data);

	node* next_node = after_node->next;
	new_node->next = next_node;
	if(next_node != NULL) next_node->prev = new_node;

	new_node->prev = after_node;
	after_node->next = new_node;

	if(ll->tail == after_node) {
		ll->tail = new_node;
	}

	ll->size++;
}

// This Function sorts the Linked List
void sort(linked_list *ll, int (*cmp)(void *data1, void *data2)) {
	node *i = ll->head;
	while(i!=NULL) {
		node *j = i->next;
		while(j!=NULL) {
			void * p1 = i->data;
			void * p2 = j->data;
			if((*cmp)(p1,p2) > 0) {
				swap_nodes(i,j);
			}
			j=j->next;
		}
		i = i->next;
	}
}

void swap_nodes(node *a, node *b) {
	void * temp = a->data;
	a->data = b->data;
	b->data = temp;
}

// Queue Implementation //

queue * create_queue() {
	return create_linked_list();
}

// This functons adds a process to the execution queue //
void enqueue(queue* q, void * data)
{
	node* new_node = create_node(data);
	
	new_node->prev = q->tail;
	if(q->tail != NULL) {
		q->tail->next = new_node;
		q->tail = new_node;
	} else {
		q->tail = new_node;
		q->head = new_node;
	}
	q->size += 1;
}

// This functions removes a process from the execution queue//
void* dequeue(queue* q)
{
	if(q->head != NULL) {
		node * current_node = q->head;//Address of q->head
		void * data = current_node->data;

		//Moving Head to next Node
		node * next_node = q->head->next;

		if(next_node != NULL) next_node->prev = NULL;
		q->head = next_node; //current_node = q->head = next_node
		

		//Maintaining boundary tail condition
		if(q->tail == current_node) {
			q->tail = NULL;
		}

		q->size--;
		free(current_node);
		return data;
	}
}
