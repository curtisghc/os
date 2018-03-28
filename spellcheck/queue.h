#include <stdio.h>
#include <stdlib.h>

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
void delete_queue(struct queue *q);
void print_queue(struct queue *q);

void enqueue(struct queue *q, char *word){
  struct node *n = (node *) malloc(sizeof(node));
  n->data = word;

  if(q->size == 0){
    q->head = n;
  }else{
	struct node *current = q->head;
	for(int i = q->size; i > 1; i--){
	  current = current->next;
	}
	current->next = n;
  }
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

void delete_queue(struct queue *q){
  if(q->size == 0){
    free(q);
    return;
  }else{
	node *current = q->head;
	node *temp = current;
	for(int i = q->size; i > 1; i--){
	  temp = current->next;
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
	node *head = q->head;
	printf("\n[");
	while(head->next != NULL){
	  printf("%s->", head->data);
	}
	printf("%s]\n", head->data);
  }
}
