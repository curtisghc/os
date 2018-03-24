#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *DICT_PATH = "/usr/share/dict/words";
int DEFAULT_PORT = 90002;

FILE *open_dict(char *path);
int num_words(FILE *fp);
int wl_size(FILE *fp);
void get_words(FILE *word_file, char *words);
void parse(char *words, char **word_list);
void free_wl(char **wl);


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
	DEFAULT_PORT = atoi(argv[port]);
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



  //references used for wl, should free everything
  free(words);
  return 0;
}
/*
  can a semaphore be used rather than a condition variable
  instructions say not to create a client
 */

