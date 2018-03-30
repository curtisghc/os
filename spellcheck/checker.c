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

int NUM_THREADS = 5;

//synchronization primates
pthread_mutex_t LOG_MTX = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MAIN_MTX = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t testlock = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t LOG_CV = PTHREAD_COND_INITIALIZER;
pthread_cond_t MAIN_CV = PTHREAD_COND_INITIALIZER;


typedef struct prims{
  int *socket_index;
  int *socketids;
  char **word_list;
  char *filename;
  struct queue *word_queue;
}prims;

//dumb stuff
FILE *open_dict(char *path);
int num_words(FILE *fp);
int wl_size(FILE *fp);
void get_words(FILE *word_file, char *words);
void parse(char *words, char **word_list);
void free_wl(char **wl);
int check_word(char **dict, char *word);

//real stuff
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

void *recieve_connection(void *args){
  struct prims *prims = (struct prims *) args;

  //this should be per thread
  void *buf = malloc(sizeof(char) * 64);
  char word[64];
  char *yes = "OK\n";
  char *no = "MISSPELLED\n";

  int *index = prims->socket_index;
  int *sockids = prims->socketids;
  char **wl = prims->word_list;
  struct queue *word_queue = prims->word_queue;


  int myid;

  //wait for broadcast from main to get sockids
  while(1){
	//instead of all this, just wait for broadcast from main

	pthread_cond_wait(&MAIN_CV, &MAIN_MTX);
	myid = sockids[*index];


	while(1){

	  recv(myid, buf, 64, 0);
	  sscanf((char *) buf, "%s\n", word);

	  //quit condition
	  if(strcmp(word, "q") == 0 || strcmp(word, "quit") == 0)
		break;

	  if(check_word(wl, word) == 1){
		strcat(word, " ");
		strcat(word, yes);
		send(myid, yes, strlen(yes), 0);
	  }else{
		strcat(word, " ");
		strcat(word, no);
		send(myid, no, strlen(no), 0);
	  }
	  //broadcast main to wake up -- release lock


	  //critical section
	  //wait for log mutex -- maybe CV
	  enqueue(word_queue, word);
	  //wakeup log writer
	  pthread_cond_broadcast(&LOG_CV);
	}
	close(myid);
  }
  free(buf);
  pthread_exit(NULL);
}

void *write_log(void *args){
  struct prims *prims = (struct prims *) args;


  struct queue *word_queue = prims->word_queue;
  char *filename = prims->filename;

  FILE *fp;

  //initialize log file to be overwritten per session
  fp = fopen(filename, "w");
  fclose(fp);

  while(1){
	//critical section
	//wait to acquire log mutex -- maybe CV so it can block
	pthread_cond_wait(&LOG_CV, &LOG_MTX);

	//append word to and then close log file every time
	fp = fopen(filename, "a");
	if(fp == NULL){
	  fprintf(stderr, "Error writing to log file");
	}else {
	  fprintf(fp, "%s", dequeue(word_queue));
	}
	fclose(fp);
	//release log mutex / CV
  }

  pthread_exit(NULL);
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
  listen(sockfd, NUM_THREADS);
  //this doesn't print the correct address or port
  printf("Listening on at: %s\n", DEFAULT_PORT);
  // now accept an incoming connection:
  addr_size = sizeof their_addr;


  //distinct thread should use this, but main initializes
  struct queue *word_queue = (queue *) malloc(sizeof(queue));
  word_queue->size = 0;

  //circular array for socket ids
  int *sockids = (int *) malloc(sizeof(int) * NUM_THREADS);


  //create primitives struct for easy passing
  struct prims *prims = (struct prims *) malloc(sizeof(prims));
  prims->socket_index = (int *) malloc(sizeof(int));
  prims->socketids = sockids;
  prims->word_list = wl;
  prims->filename = "logfile";
  prims->word_queue = word_queue;


  //create one thread, send to work on log writing and then
  //create num_threads threads, send them to wait for sockids to open
  pthread_t tid;
  for(int i = 0; i < NUM_THREADS; i++){
	pthread_create(&tid, NULL, recieve_connection, prims);
	pthread_detach(tid);
  }

  pthread_create(&tid, NULL, write_log, prims);


  int newid;
  for(int index = 0; ; index = (index + 1) % NUM_THREADS){
	//add new socket to circular queue

	//pthread_cond_wait(&CV, &cond_mtx);

	newid = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
	printf("New connection\n");

	//lock out sockids array

	// should fix this to be locked
	sockids[index] = newid;
	*prims->socket_index = index;
	//instead of all this, broadcast to CV
	pthread_cond_broadcast(&MAIN_CV);

  }



  print_queue(word_queue);
  //tidying up

  freeaddrinfo(res);
  free(wl);

  free(prims->socket_index);
  free(prims->socketids);
  delete_queue(prims->word_queue);

  pthread_mutex_destroy(&LOG_MTX);
  pthread_mutex_destroy(&MAIN_MTX);
  pthread_cond_destroy(&MAIN_CV);
  free(prims);

  return 0;
}
/*
  sigpipe/segfault terminates when using telnet quit
  not able to service 2 clients at the same time
 */


/*
  can a semaphore be used rather than a condition variable -- must use cv
  instructions say not to create a client -- don't need to make a client
 */
