#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "utilities.h"

void parse(char *args, char **args_parsed);
int execute_in_background(char **input);
void execute(char **input);
int check_builtin(char **input);


//each whitespace-separated word in args will be placed as elements of
//args_parsed, a 2d array.
void parse(char *args, char **args_parsed){
  //replace whitespace with '\0'
  while (*args != '\0'){
	while (*args == ' ' || *args == '\t' || *args == '\n'){
	  *args++ = '\0';
	}
	//add partitioned strings to 2d array
	*args_parsed++ = args;
	//skip over extra white space
	while (*args != '\0' && *args != ' ' && *args != '\t' && *args != '\n'){
		args++;
	}
  }
  //end of 2d array
  *args_parsed = '\0';
}

//returns 1 if there is a '&' char in the input, indicating the child
//process should not be waited for by the parent.
int execute_in_background(char **input){
  //set background boolean to true if input has '&'
  while(**input != '\0'){
	if(**input == '&'){
	  return 1;
	}
	input++;
  }
  return 0;
}

void execute(char **input){
  pid_t pid = fork();
  int status;
  int background = execute_in_background(input);

  if(pid < 0){
	fprintf(stderr, "ERROR: Failed to fork\n");
	exit(1);
  }else if(pid == 0){
	//child process execution of command
	//first, check if input matches builtin function

	if(check_builtin(input)){
	}else if(execvp(*input, input) <= 0){
	  fprintf(stderr, "ERROR: Failed to execute\n");
	  exit(1);
	}
  }else if(!background){
	//wait if child should not be in background
	while(wait(&status) != pid);
  }
}

//compare input to list of builtin functions, and execute if matched
//return 1 if match found
int check_builtin(char **input){
  char *command = *input;

  if(strcmp(command, "cd") == 0)
	cd(input);
  else if(strcmp(command, "pwd") == 0)
	pwd();
  else if(strcmp(command, "cls") == 0)
	cls();
  else if(strcmp(command, "dir") == 0)
	dir();
  else if(strcmp(command, "environ") == 0)
	environ();
  else if(strcmp(command, "echo") == 0)
	echo(input);
  else if(strcmp(command, "help") == 0)
	help(input);
  else if(strcmp(command, "_pause") == 0)
	_pause();
  else
	return 0;
  return 1;
}

int main(int argc, char **argv){
  char args[1024];
  char *args_parsed[64];

  while(1){
	printf("prompt > ");
	fgets(args, 1024, stdin);
	printf("\n");
	parse(args, args_parsed);
	if(strcmp(args_parsed[0], "exit") == 0){
	  return 0;
	}
	execute(args_parsed);
  }
  //fix issue with exit- for each command entered, exit must be entered
  //an extra time to actually exit
  //something to do with mock prompts, i think
}

