#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>


void cd(char **input);
void pwd();
void cls();
void dir();
void _environ();
void echo(char **input);
void help();
void _pause();


void cd(char **input){
  if(input[1][0] == '\0'){
	pwd();
  }else if(chdir(input[1]) == -1){
	fprintf(stderr, "ERROR: Failed to change directory\n");
  }
}

void pwd(){
  char dir[1024];
  getcwd(dir, 1024);
  printf("%s\n", dir);
}

void cls(){
  printf("\033[H\033[J");
}

void dir(){
  DIR *d;
  struct dirent *dir;
  d = opendir(".");
  if(d){
	while((dir = readdir(d)) != NULL){
	  printf("%s\n", dir->d_name);
	}
	closedir(d);
  }
}

void _environ(){
  extern char **environ;
  while(*environ != NULL){
	printf("%s\n", *environ++);
  }
}
//use for loop to print elements of environ global variable

void echo(char **input){
  input++;
  while(**input != '\0'){
	printf("%s ", *input);
	input++;
  }
  printf("\n");
}

void help(){
  printf("All the help you need: 8==D \n");
}
//come up with help dialogue

void _pause(){
  while('\n' != getc(stdin));
}



