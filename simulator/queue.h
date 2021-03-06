#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct queue{
  int size;
  struct job *head;
}queue;

typedef struct node{
  char *data;
  struct node *next;
}node;

typedef struct job_list{
  int size;
  struct job *head;
  struct job *tail;
}job_list;
  
typedef struct job{
  int time;
  int type;
  struct job *next;
}job;

//queue functions 
job *create_job(int time, int type);
void enqueue(struct queue *q, job *new_job);
job *dequeue(struct queue *q);
void delete_queue(struct queue *q);
void print_queue(queue *q);

//priority queue functions 
void add_job(job_list *jl, job *j);
void sort_job_list(job_list *jl, job *new_job);
job *get_job(job_list *jl);
int peek_job_list(job_list *jl);
void print_job_list(job_list *jl);
void delete_job_list(job_list *jl);

job *create_job(int time, int type){
  job *j = (job *) malloc(sizeof(job));
  j->time = time;
  j->type = type;
  return j;
}

void enqueue(struct queue *q, job *new_job){
  if(q->size == 0){
    q->head = new_job;
  }else{
    new_job->next = q->head;
    q->head = new_job;
  }
  q->size++;
}

job *dequeue(struct queue *q){
  if(q->size == 0){
    return NULL;
  }
  job *j = q->head;
  job *temp = q->head;
  while(j->next != NULL){
    temp = j;
    j = j->next;
  }
  temp->next = NULL;
  q->size--;
  return j;
}

void delete_queue(struct queue *q){
  if(q->size == 0){
    free(q);
    return;
  }
  job *j = q->head;
  job *temp = q->head;
  while(j->next !=NULL){
    temp = j->next;
    //must free(n->data);
    free(j);
    j = temp;
  }
  //must free(temp->data);
  free(temp);
  free(q);
}
void print_queue(queue *q){
  if(q->size == 0){
    return;
  }
  job *head = q->head;
  printf("\n");
  while(head->next != NULL){
    printf("%d\n", head->time);
  }
  printf("%d\n\n", head->time);
}

void add_job(job_list *jl, job *j){
  if(jl->size == 0){
    jl->head = j;
    jl->tail = j;
    jl->size++;
  }else{
    sort_job_list(jl, j);
  }
}

//places new_job in order in jl. o(n) 
void sort_job_list(job_list *jl, job *new_job){
  int time = new_job->time;
  job *head = jl->head;

  if(time >= head->time){
    new_job->next = head;
    jl->head = new_job;
  }else{
    job *temp = head;
    int added = 0;
    //issue here
    while(head->next != NULL){
      if(time < head->time){
	//iterate through list if new job < current head
	temp = head;
	//issue here, circular linked lists happen.
	head = head->next;
      }else{
	//otherwise insert the new job between pointers
	temp->next = new_job;
	new_job->next = head;
	added = 1;
	break;
      }
    }
    if(added == 0){
      if(time > head->time){
	temp->next = new_job;
	new_job->next = head;
      }else{
	head->next = new_job;
	jl->tail = new_job;
      }
    }
  }
  jl->size++;
}

job *get_job(job_list *jl){
  if(jl->size == 0){
    return NULL;
  }
  job *head = jl->head;
  job *temp = jl->head;
  while(head->next != NULL){
    temp = head;
    head = head->next;
  }
  temp->next = NULL;
  jl->size--;
  return head;
}

int peek_job_list(job_list *jl){
  if(jl->size == 0){
    return -1;
  }
  //issue here
  return jl->tail->time;
  /*
  job *head = jl->head;
  while(head->next != NULL){
    head = head->next;
  }
  return head->time;
  */
}

void print_job_list(job_list *jl){
  if(jl->size == 0){
    return;
  }
  job *j = jl->head;
  printf("\n[");
  while(j->next != NULL){
    printf("%d, ", j->time);
    j = j->next;
  }
  printf("%d]\n", j->time);
}

void delete_job_list(job_list *jl){
  if(jl->size == 0){
    free(jl);
    return;
  }
  job *head = jl->head;
  job *temp = head;
  while(head->next !=NULL){
    temp = head->next;
    free(head);
    head = temp;
  }
  free(temp);
  free(jl);
}
