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
	fprintf(stderr, "%s: No such directory\n", input[1]);
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

//print help dialogue into less for readability
void help(){
  printf("Basic help dialogue for \"myshell\"\n");
  printf("Navigate to \"readme\" for more help\n");
  printf("\n");
  printf("These shell commands are defined internally:\n");
  printf("cd [dir] - change direcotries\n");
  printf("pwd - print current working directory\n");
  printf("cls - clear screen\n");
  printf("dir - print contents of current working directory\n");
  printf("environ - print all evnironment variables\n");
  printf("echo [args] - print args separated by whitespace\n");
  printf("help - print this help message\n");
  printf("pause - stop all processes, wait for return key\n");
}

//uses getc to wait until return is entered
void _pause(){
  while('\n' != getc(stdin));
}

