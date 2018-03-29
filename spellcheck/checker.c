#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "queue.h"

char *DICT_PATH = "/usr/share/dict/words";
char *DEFAULT_PORT = "24466";

typedef struct prims{
  int *socket_index;
  int *socketids;
  char **word_list;
  char *filename;
  struct queue *word_queue;
  pthread_mutex_t *log_mtx;
  pthread_mutex_t *cond_mtx;
  pthread_cond_t *CV;
  //needs to include monitor;

}prims;

/*
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int avail = 1;
*/

FILE *open_dict(char *path);
int num_words(FILE *fp);
int wl_size(FILE *fp);
void get_words(FILE *word_file, char *words);
void parse(char *words, char **word_list);
void free_wl(char **wl);

int check_word(char **dict, char *word);
void *consume(void *socket_arr);
void *recieve_connection(void *args);
void *write_log(void *args);


FILE *open_dict(char *path){
  FILE *fp = fopen(path, "r");
  if(fp == NULL){
	fprintf(stderr, "%s: no such file\n", path);
	exit(1);
  }
  return fp;
}

//return number of words from the dict file
int num_words(FILE *fp){
  int count = 0;
  char c;
  while((c = fgetc(fp)) > 0){
	if(c == '\n')
		count++;
  }
  rewind(fp);
  return count;
}

//return number of characters in word file
int wl_size(FILE *wf){
  size_t size = 0;
  fseek(wf, 0, SEEK_END);
  size = ftell(wf);
  rewind(wf);
  return size;
}

//read all contents of dict file into char *: words
void get_words(FILE *word_file, char *words){
  int size = wl_size(word_file);

  fread(words, size, 1, word_file);
  words[size] = '\0';
}

//separate words into 2d array of words separated by newline
void parse(char *words, char **word_list){
  while(*words != '\0'){
	if(*words == '\n'){
	  *words++ = '\0';
	}
	*word_list++ = words;
	while(*words != '\0' && *words != '\n'){
	  words++;
	}
  }
  *--word_list = NULL;
}

//set global configs based on args
void import_args(int argc, char **argv){

  const char nums[] = "0123456789";
  int port = 0;
  int dict = 0;

  if(strcspn(argv[1], nums) != 0){
	dict = 1;
	if(argc == 3)
	  port = 2;
  }else{
	port = 1;
	if(argc == 3)
	  dict = 2;
  }

  if(dict){
	DICT_PATH = argv[dict];
  }
  if(port){
	DEFAULT_PORT = argv[port];
  }
}

//doesn't work, don't use
void free_wl(char **wl){
  char **temp = wl;
  wl++;
  while(*wl != NULL){
	free(*wl);
	wl++;
  }
  free(temp);
}

int check_word(char **dict, char *word){
  while(*dict != NULL){
	if(strcmp(*dict, word) == 0){
	  return 1;
	}
	dict++;
  }
  return 0;
}

/*
void *consume(void *socketid){
  //int sockid = (int) socketid;
  printf("%d\n", (int) pthread_self());
  return NULL;
}
*/

void *recieve_connection(void *args){

  struct prims *prims = (struct prims *) args;

  //this should be per thread
  void *buf = malloc(sizeof(char) * 64);
  char word[64];
  //char *logline;
  char *yes = "OK\n";
  char *no = "MISSPELLED\n";

  int *index = prims->socket_index;
  int *sockids = prims->socketids;
  char **wl = prims->word_list;
  struct queue *word_queue = prims->word_queue;
  pthread_mutex_t *log_mtx = prims->log_mtx;
  pthread_mutex_t *cond_mtx = prims->cond_mtx;
  pthread_cond_t *CV = prims->CV;



  //int sock_fd = consume(socketarray);

  while(1){

	//monitor lock
	pthread_cond_wait(CV, cond_mtx);
	//recieve word, scan into "word"
	//recv(new_fd, buf, 64, 0);
	recv(sockids[*index], buf, 64, 0);

	sscanf((char *) buf, "%s\n", word);
	if(strcmp(word, "q") == 0 || strcmp(word, "quit") == 0)
	  break;


	if(check_word(wl, word) == 1){
	  strcat(word, " ");
	  strcat(word, yes);
	  send(sockids[*index], yes, strlen(yes), 0);
	}else{
	  strcat(word, " ");
	  strcat(word, no);
	  send(sockids[*index], no, strlen(no), 0);
	}


	//critical section
	pthread_mutex_lock(log_mtx);
	enqueue(word_queue, word);
	pthread_mutex_unlock(log_mtx);
  }
  //monitor unlock

  free(buf);
  return NULL;
}

void *write_log(void *args){// struct queue *word_queue, pthread_mutex_t *M){
  struct prims *prims = (struct prims *) args;


  struct queue *word_queue = prims->word_queue;
  pthread_mutex_t *log_mtx = prims->log_mtx;
  char *filename = prims->filename;


  FILE *fp = fopen(filename, "w");
  if(fp == NULL){
	fprintf(stderr, "%s: unable to write to log file", filename);
  }else{
	while(1){
	  //critical section
	  pthread_mutex_lock(log_mtx);
	  fprintf(fp, "%s", dequeue(word_queue));
	  pthread_mutex_unlock(log_mtx);
	}
  }
  fclose(fp);
  return NULL;
}


int main(int argc, char **argv){
  //manually set dict file and port if necessary
  if(argc > 1){
	import_args(argc, argv);
  }

  //allocate and open files, read into words data structure.
  FILE *wf = open_dict(DICT_PATH);
  char **wl = (char **) malloc(sizeof(char *) * num_words(wf));
  char *words = (char *) malloc(sizeof(char) * wl_size(wf));
  get_words(wf, words);
  fclose(wf);
  parse(words, wl);
  //dict datastructure setup, no more


  //now do socket stuff
  //connect to socket singularly just to test
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  struct addrinfo hints, *res;
  int sockfd;//, new_fd;

  //need to do error checking
  // first, load up address structs with getaddrinfo():
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &res);

  // make a socket, bind it, and listen on it:
  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  bind(sockfd, res->ai_addr, res->ai_addrlen);
  listen(sockfd, 5);
  //this doesn't print the correct address or port
  printf("Listening on at: %s\n", DEFAULT_PORT);
  // now accept an incoming connection:
  addr_size = sizeof their_addr;


  //distinct thread should use this, but main initializes
  struct queue *word_queue = (queue *) malloc(sizeof(queue));
  word_queue->size = 0;

  //circular array for socket ids
  int *sockids = (int *) malloc(sizeof(int) * 5);

  //set up synchronization primates
  pthread_mutex_t log_mtx;
  pthread_mutex_init(&log_mtx, NULL);
  pthread_mutex_t cond_mtx;
  pthread_mutex_init(&cond_mtx, NULL);
  pthread_cond_t CV;
  pthread_cond_init(&CV, NULL);

  //create primitives struct for easy passing
  struct prims *prims = (struct prims *) malloc(sizeof(prims));
  prims->socket_index = (int *) malloc(sizeof(int));
  prims->socketids = sockids;
  prims->word_list = wl;
  prims->filename = "logfile";
  prims->word_queue = word_queue;
  prims->log_mtx = &log_mtx;
  prims->cond_mtx = &cond_mtx;
  prims->CV = &CV;

  //create one thread, send to work on log writing and then
  //create 5 threads, send them to wait for sockids to open
  pthread_t tid;
  for(int i = 0; i < 5; i++){
	pthread_create(&tid, NULL, recieve_connection, prims);
	pthread_detach(tid);
  }
  pthread_exit(NULL);

  pthread_create(&tid, NULL, write_log, prims);
  pthread_exit(NULL);


  //make main loop toward end; main should mostly be waiting for accept() to return
  for(int index = 0; ; index = (index + 1) % 5){
	//add new socket to circular queue

	//lock index of socket array? should be locked while threads are connected
	sockids[index] = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
	*prims->socket_index = index;
	//unlock
	pthread_cond_signal(&CV);
	//wakeup monitor, one of the threads will unlock hopefully
  }

  /*
  //working thing
  while(1){
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
	recieve_connection(wl, new_fd, word_queue);
  }
  */


  print_queue(word_queue);
  //tidying up
  freeaddrinfo(res);
  free(wl);

  free(prims->socket_index);
  free(prims->socketids);
  delete_queue(prims->word_queue);
  pthread_mutex_destroy(&log_mtx);
  pthread_mutex_destroy(&cond_mtx);
  pthread_cond_destroy(&CV);
  free(prims);

  return 0;
}

/*
  can a semaphore be used rather than a condition variable -- must use cv
  instructions say not to create a client -- don't need to make a client
 */
