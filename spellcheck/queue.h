#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct queue{
  int size;
  struct node *head;
}queue;

typedef struct node{
  char *data;
  struct node *next;
}node;


//queue functions
void enqueue(struct queue *q, char *word);
char *dequeue(struct queue *q);
char *peek(struct queue *q);
void delete_queue(struct queue *q);
void print_queue(struct queue *q);

void enqueue(struct queue *q, char *word){
  struct node *n = (node *) malloc(sizeof(node));

  n->data = (char *) malloc(strlen(word) + 1);
  strcpy(n->data, word);

  n->next = q->head;
  q->head = n;


  /*
  if(q->size == 0){
    q->head = n;
  }else{
	struct node *current = q->head;
	for(int i = q->size; i > 1; i--){
	  current = current->next;
	}
	current->next = n;
  }
  */
  q->size++;
}

char *dequeue(struct queue *q){
  if(q->size == 0){
    return NULL;
  }
  node *temp = q->head;
  char *word = temp->data;
  q->head = temp->next;
  q->size--;

  free(temp);
  return word;
}

char *peek(struct queue *q){
  if(q->size == 0){
	return NULL;
  }else{
	return q->head->data;
  }
}

void delete_queue(struct queue *q){
  if(q->size == 0){
    free(q);
    return;
  }else{
	node *current = q->head;
	node *temp = current;
	for(int i = q->size; i > 1; i--){
	  temp = current->next;
	  free(current->data);
	  free(current);
	  current = temp;
	}
  free(q);
  }
}

void print_queue(queue *q){
  if(q->size == 0){
	printf("Empty Queue!\n");
  }else{
	node *temp = q->head;
	printf("\n[");
	for(int i = q->size; i > 0; i--){//while(temp->next != NULL){
	  printf("%s->", temp->data);
	  temp = temp->next;
	}
	printf("]\n");
  }
}
