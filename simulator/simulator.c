#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "queue.h"

#define JOB_ARRIVAL 0
#define CPU_FINISHED 1
#define DISK1_FINISHED 2
#define DISK2_FINISHED 3

int SEED;
int INIT_TIME;
int FIN_TIME;
int ARRIVE_MIN;
int ARRIVE_MAX;
int QUIT_PROB;
int CPU_MIN;
int CPU_MAX;
int DISK1_MIN;
int DISK1_MAX;
int DISK2_MIN;
int DISK2_MAX;

int get_config(char *config);

//int handler(job *j, job_list *jl, queue* cpu, queue* disk1, queue* disk2);
int arrival_handler(job *j, job_list *jl, queue *cpu);
int cpu_finished_handler(job *j, job_list *jl, queue *disk1, queue *disk2);
int disk1_handler(job *j, job_list *jl, queue *disk1);
int disk2_handler(job *j, job_list *jl, queue *disk2);
int quit_prob();
int cpu_prob();
int disk1_prob();
int disk2_prob();
int arrival_prob();

//read config file into global vars
int get_config(char *config){
  FILE *fp;
  fp = fopen(config, "r");
  if(fp == NULL){
    printf("Falied to read file");
    return -1;
  }
  char parameter[20];
  char temp[20];
  int value;
  while(fgets(parameter, 20, fp) != NULL){
    sscanf(parameter, "%s %d", temp, &value);

    //set global vars based on config
    if(strcmp(temp, "SEED") == 0){
      SEED = value;
    }else if(strcmp(temp, "INIT_TIME") == 0){
      INIT_TIME = value;
    }else if(strcmp(temp, "FIN_TIME") == 0){
      FIN_TIME = value;
    }else if(strcmp(temp, "ARRIVE_MIN") == 0){
      ARRIVE_MIN = value;
    }else if(strcmp(temp, "ARRIVE_MAX") == 0){
      ARRIVE_MAX = value;
    }else if(strcmp(temp, "QUIT_PROB") == 0){
      QUIT_PROB = value;
    }else if(strcmp(temp, "CPU_MIN") == 0){
      CPU_MIN= value;
    }else if(strcmp(temp, "CPU_MAX") == 0){
      CPU_MIN = value;
    }else if(strcmp(temp, "DISK1_MIN") == 0){
      DISK1_MIN = value;
    }else if(strcmp(temp, "DISK1_MAX") == 0){
      DISK1_MAX = value;
    }else if(strcmp(temp, "DISK2_MIN") == 0){
      DISK2_MIN = value;
    }else if(strcmp(temp, "DISK2_MAX") == 0){
      DISK2_MAX = value;
    }
  }
  return 0;
}

int arrival_handler(job *j, job_list *jl, queue *cpu){
  int time = cpu_prob();
  if(cpu->size == 0){
    //randomly generate time
    j->time += time;
    j->type = CPU_FINISHED;
    add_job(jl, j);
  }else{
    //place event in cpu queue
    enqueue(cpu, j);

    //process next cpu event 
    j = dequeue(cpu);
    //randomly generate time
    j->time += time;
    j->type = CPU_FINISHED;
    add_job(jl, j);
  }
  return time;
}

int cpu_finished_handler(job *j, job_list *jl, queue *disk1, queue *disk2){

  if(quit_prob()){
    //("\nProcess Terminated\n");
    free(j);
    return 1;
  }else{
    if(disk1->size < disk2->size){
      j->type = DISK1_FINISHED;
    }else if(disk1->size > disk2->size){
      j->type = DISK2_FINISHED;
    }else if(rand() % 2 == 0){
      j->type = DISK1_FINISHED;
    }else{
      j->type = DISK2_FINISHED;
    }

    add_job(jl, j);
    return 0;
    
    /*
    if(disk1->size < disk2->size){
      return disk1_handler(j, jl, disk1);
    }else if(disk1->size > disk2->size){
      return disk2_handler(j, jl, disk2);
    }else if(rand() % 2 == 0){
      return disk1_handler(j, jl, disk1);
    }else{
      return disk2_handler(j, jl, disk2);
    }
    */
  }
}
      
int disk1_handler(job *j, job_list *jl, queue *disk1){
  int time = disk1_prob();
  if(disk1->size == 0){
    j->time += time;
    j->type = DISK1_FINISHED;
    add_job(jl, j);
  }else{
    enqueue(disk1, j);

    j = dequeue(disk1);
    j->time += time;
    j->type = DISK1_FINISHED;
    add_job(jl, j);
  }
  return time;
}


int disk2_handler(job *j, job_list *jl, queue *disk2){
  int time = disk2_prob();
  if(disk2->size == 0){
    j->time += time;
    j->type = DISK2_FINISHED;
    add_job(jl, j);
  }else{
    enqueue(disk2, j);

    j = dequeue(disk2);
    j->time += time;
    j->type = DISK2_FINISHED;
    add_job(jl, j);
  }
  return time;
}


//QUIT_PROB is percent chance that event quits
int quit_prob(){
  return (rand() % 100 < QUIT_PROB);
}
//returns values for time taken for each process
int cpu_prob(){
  return rand() % (CPU_MAX - CPU_MIN) + CPU_MIN;
}
int disk1_prob(){
  return rand() % (DISK1_MAX - DISK1_MIN) + DISK1_MIN;
}
int disk2_prob(){
  return rand() % (DISK2_MAX - DISK2_MIN) + DISK2_MIN;
}
int arrival_prob(){
  return rand() % (ARRIVE_MAX - ARRIVE_MIN) + ARRIVE_MIN;
}

int main(int argc, char **argv){
  job_list *jl = (struct job_list *) malloc(sizeof(job_list));
  queue *cpu = (struct queue *) malloc(sizeof(queue));
  queue *disk1 = (struct queue *) malloc(sizeof(queue));
  queue *disk2 = (struct queue *) malloc(sizeof(queue));
  jl->size = 0;
  cpu->size = 0;
  disk1->size = 0;
  disk2->size = 0;

  if(get_config("config") == -1){
    printf("Failed to retrieve config");
    return -1;
  }

  srand(SEED);
  //srand(time(NULL));

  //initial jobs
  add_job(jl, create_job(INIT_TIME,0));
  add_job(jl, create_job(FIN_TIME,0));
  
  //initialize log file
  FILE *fp = fopen("log", "w");
  if(fp == NULL){
    printf("Failed to write log");
    return -1;
  }
  fprintf(fp, "SEED %d\n", SEED);
  fprintf(fp, "INIT_TIME %d\n", INIT_TIME);
  fprintf(fp, "FIN_TIME %d\n", FIN_TIME);
  fprintf(fp, "ARRIVE_MIN %d\n", ARRIVE_MIN);
  fprintf(fp, "ARRIVE_MAX %d\n", ARRIVE_MAX);
  fprintf(fp, "QUIT_PROB %d\n", QUIT_PROB);
  fprintf(fp, "CPU_MIN %d\n", CPU_MIN);
  fprintf(fp, "CPU_MAX %d\n", CPU_MAX);
  fprintf(fp, "DISK1_MIN %d\n", DISK1_MIN);
  fprintf(fp, "DISK1_MAX %d\n", DISK1_MAX);
  fprintf(fp, "DISK2_MIN %d\n", DISK2_MIN);
  fprintf(fp, "DISK2_MAX %d\n\n", DISK2_MAX);

  //statistics counters
  job *event;
  int time;
  int counter = 0;
  int event_queue_size = 0;
  int busy_time;

  int cpu_queue_size = 0;
  int cpu_num_events = 0;
  int cpu_total_time = 0;
  int cpu_max_time = 0;

  int disk1_queue_size = 0;
  int disk1_num_events = 0;
  int disk1_total_time = 0;
  int disk1_max_time = 0;

  int disk2_queue_size = 0;
  int disk2_num_events = 0;
  int disk2_total_time = 0;
  int disk2_max_time = 0;


  //main loop
  while(jl->head != NULL && peek_job_list(jl) < FIN_TIME){
    //pop job from event queue
    event = get_job(jl);
    time = event->time;

    //add new event to event queue with random time + current event time
    add_job(jl, create_job(time + arrival_prob(), 0));


    //info about job list
    //print_job_list(jl);
    
    //printf("T:%d\n", time);

    //log info about popped job
    fprintf(fp, "\nEvent number: %d\n", counter);
    fprintf(fp, "Event time: %d\n", event->time);
    fprintf(fp, "Event type: ");

    if(event->type == 0){
      fprintf(fp, "Event Arrival\n");
      cpu_num_events++;
      busy_time = arrival_handler(event, jl, cpu/*, disk1, disk2*/);
      if(busy_time > cpu_max_time){
	cpu_max_time = busy_time;
      }
      cpu_total_time += busy_time;

    }else if(event->type == 1){
      fprintf(fp, "CPU Finished\n");
      //add total time to cpu, and dispatch event to handler

      //cpu_finished_handler(event, jl, disk1, disk2);
      if(cpu_finished_handler(event, jl, disk1, disk2)){
	fprintf(fp, "Event Terminated\n");
      }

    }else if(event->type == 2){
      fprintf(fp, "Disk1 Finished\n");
      disk1_num_events++;
      //add total time to cpu, and dispatch event to handler
      busy_time = disk1_handler(event, jl, disk1);
      if(busy_time > disk1_max_time){
	disk1_max_time = busy_time;
      }
      disk1_total_time += busy_time;

    }else if(event->type == 3){
      fprintf(fp, "Disk2 Finished\n");
      disk2_num_events++;
      //add total time to cpu, and dispatch event to handler
      busy_time = disk2_handler(event, jl, disk2);
      if(busy_time > disk2_max_time){
	disk2_max_time = busy_time;
      }
      disk2_total_time += busy_time;

    }

    //printf("%d\n", counter);
    //statistics
    counter++;
    event_queue_size += jl->size;
    cpu_queue_size += cpu_num_events;
    disk1_queue_size += disk1_num_events;
    disk2_queue_size += disk2_num_events;
  }


  //big block of statistics
  fprintf(fp, "\n%d events processed\n", counter);

  float average_event_queue_size = (float)event_queue_size / counter;
  float average_cpu_queue_size = (float)cpu_queue_size / counter;
  float average_disk1_queue_size = (float)disk1_queue_size / counter;
  float average_disk2_queue_size = (float)disk2_queue_size / counter;
  fprintf(fp, "\nAverage event queue size: %f\n", average_event_queue_size);
  fprintf(fp, "Average cpu queue size: %f\n", average_cpu_queue_size);
  fprintf(fp, "Average disk1 queue size: %f\n", average_disk1_queue_size);
  fprintf(fp, "Average disk2 queue size: %f\n", average_disk2_queue_size);

  int total_time = FIN_TIME - INIT_TIME;
  float cpu_utilization = cpu_total_time / total_time;
  float disk1_utilization = disk1_total_time / total_time;
  float disk2_utilization = disk2_total_time / total_time;
  fprintf(fp, "\nCPU utilization: %f\n", cpu_utilization);
  fprintf(fp, "Disk1 utilization: %f\n", disk1_utilization);
  fprintf(fp, "Disk2 utilization: %f\n", disk2_utilization);

  fprintf(fp, "\nCPU max time: %d\n", cpu_max_time);
  fprintf(fp, "Disk1 max time: %d\n", disk1_max_time);
  fprintf(fp, "Disk2 max time: %d\n", disk2_max_time);

  float average_cpu_time = (float) cpu_total_time / cpu_num_events;
  float average_disk1_time = (float) disk1_total_time / disk1_num_events;
  float average_disk2_time = (float) disk2_total_time / disk2_num_events;
  fprintf(fp, "\nCPU Average time: %f\n", average_cpu_time);
  fprintf(fp, "Disk1 Average time: %f\n", average_disk1_time);
  fprintf(fp, "Disk2 Average time: %f\n", average_disk2_time);

  float cpu_throughput = (float) cpu_num_events / total_time;
  float disk1_throughput = (float) disk1_num_events / total_time;
  float disk2_throughput = (float) disk2_num_events / total_time;
  fprintf(fp, "\nCPU throughput: %f\n", cpu_throughput);
  fprintf(fp, "Disk1 throughput: %f\n", disk1_throughput);
  fprintf(fp, "Disk2 throughput: %f\n", disk2_throughput);


  printf("%d events processed\n", counter);
  //printf("Total cpu time:%d\n", cpu_total_time);
  //printf("Total disk1 time:%d\n", disk1_total_time);
  //printf("Total disk2 time:%d\n", disk2_total_time);

  fclose(fp);

  delete_queue(cpu);
  delete_queue(disk1);
  delete_queue(disk2);
  delete_job_list(jl);

  return 0;
}
