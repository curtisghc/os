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


//use chdir to switch directory to argument
void cd(char **input){
  if(input[1] == NULL){
	pwd();
  }else if(chdir(input[1]) == -1){
	fprintf(stderr, "ERROR: Failed to change directory\n");
  }
}

//print directory using getcwd()
void pwd(){
  char dir[1024];
  getcwd(dir, 1024);
  printf("%s\n", dir);
}

//ascii escape key to clear screen
void cls(){
  printf("\033[H\033[J");
}

//iterate through directory contents and print each
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

//iterate through external environment variables and print each
void _environ(){
  extern char **environ;
  while(*environ != NULL){
	printf("%s\n", *environ++);
  }
}

//iterate through input and print each separated by one space
void echo(char **input){
  input++;
  while(*input != NULL){
	printf("%s ", *input);
	input++;
  }
  printf("\n");
}

void help(){
  printf("All the help you need: 8==D \n");
}
//come up with help dialogue

//uses getch to wait until return is entered
void _pause(){
  while('\n' != getc(stdin));
}



