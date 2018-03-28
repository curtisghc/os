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

void *consume(void *socketid){

  //int sockid = (int) socketid;
  printf("%d\n", (int) pthread_self());
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

  //connect to socket singularly just to test
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  struct addrinfo hints, *res;
  int sockfd, new_fd;

  //need to do error checking

  // first, load up address structs with getaddrinfo():
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  //hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

  getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &res);

  // make a socket, bind it, and listen on it:

  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  bind(sockfd, res->ai_addr, res->ai_addrlen);
  listen(sockfd, 5);

  // now accept an incoming connection:
  addr_size = sizeof their_addr;
  new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

  void *buf = malloc(sizeof(char) * 64);
  char word[64];
  char *yes = "Correct Spelling\n";
  char *no = "Incorrect Spelling\n";

  //need to work on word queue
  struct queue *word_queue = (queue *) malloc(sizeof(queue));
  word_queue->size = 0;

  while(1){
	recv(new_fd, buf, 64, 0);

	sscanf((char *) buf, "%s\n", word);

	enqueue(word_queue, word);


	if(check_word(wl, word) == 1){
	  send(new_fd, yes, strlen(yes), 0);
	}else{
	  send(new_fd, no, strlen(no), 0);
	}
  }

  print_queue(word_queue);

  delete_queue(word_queue);
  free(buf);
  freeaddrinfo(res);
  free(wl);

	/*

  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if(socket_desc == -1){
	fprintf(stderr, "%s: Could not create socket", argv[0]);
	exit(1);
  }

  struct sockaddr_in server;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_port = htons(80);



  if(listen(socket_desc, 5) < 0){
	fprintf(stderr, "%s: failed to listen on port %d\n", argv[0], DEFAULT_PORT);
  }
  printf("Listening on port: %d\n", DEFAULT_PORT);
  accept();

  close(socket_desc);

  */
  /*
  if(connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
	fprintf(stderr, "%s: failed to establish socket\n", argv[0]);
	return 1;
  }
  printf("Connected to socket\n");
  }else{
	wait(NULL);
  }
  */

  //create 5 sockets, read into circular array
  /*
  int socket_arr[5];
  int elem = 0;
  for(int i = 0; i < 5; i++){
	int socket_desc;
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_desc == -1){
	  fprintf(stderr, "Could not create socket");
	  exit(1);
	}
	socket_arr[elem] = socket_desc;
	elem = (elem + 1) % 5;
  }


  while(1){
	printf("%d\n", socket_arr[elem]);
	elem = (elem + 1) % 5;
  }
  */


  //save this for later
  //create 5 threads, send to consumer function
  /*
  int i;
  pthread_t tid;
  for(i = 0; i < 5; i++){
	pthread_create(&tid, NULL, consume(&socket_arr[5]), NULL);
  }
  pthread_exit(NULL);
  */

  /*
	parent:
	setup 5 sockets
	while(if word is in socket){
	  produce socket descriptor into circular array
	}
	child:
	consume socket descriptor from circular array {
	check-word
	return true or false to socket for the client
	release element of circular array (consumer process)
	}


  references used for wl, should free everything
  */
  return 0;
}

/*
  can a semaphore be used rather than a condition variable -- must use cv
  instructions say not to create a client -- don't need to make a client
 */

