#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *DICT_PATH = "/usr/share/dict/words";

FILE *open_dict(char *path);
int num_lines(FILE *fp);
char **create_word_list(FILE *word_file);
void free_word_list(char **wl);


FILE *open_dict(char *path){
  FILE *fp = fopen(path, "r");
  if(fp == NULL){
	fprintf(stderr, "%s: no such file\n", path);
	exit(1);
	//return NULL;
  }
	return fp;
}

int num_lines(FILE *fp){
  int count = 0;
  char c;
  while((c = fgetc(fp)) > 0){
	if(c == '\n')
		count++;
  }
  return count;
}

char **create_word_list(FILE *word_file){
  int lines = num_lines(word_file);
  char **word_list = (char **) malloc((sizeof(char) * lines) + 1);
  char *word = NULL;
  while(fscanf(word_file, "%s", word) > 0){
	*word_list = word;
	word_list++;
  }
  //*word_list = NULL;
  /*
	for each line in fp
	malloc char * in wl
	add word to wl
	increment
  */

  return word_list;
}

void free_word_list(char **wl){
  while(wl++ != NULL){
	free(*wl);
  }
  free(wl);
}

int main(int argc, char **argv){
  //manually set dict file
  if(argc > 1){
	DICT_PATH = argv[1];
  }

  FILE *wf = open_dict(DICT_PATH);
  char **wl = create_word_list(wf);
  fclose(wf);

  printf("%s\n",*wl);
  //free_word_list(wl);
  return 0;
}
