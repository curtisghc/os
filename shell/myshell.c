#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "utilities.h"

void parse(char *args, char **args_parsed);
int *check_redirect(char **input);
int check_background(char **input);
void execute(char **input);
int redirected_execute(int *type, char **input);
int out_to_file(char **command, char *file);
int run_builtin(char **input);
int run_script(char *file);
int dispatch(char **input);


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
int check_background(char **input){
  //set background boolean to true if input has '&'
  while(**input != '\0' && **input != '\n'){
	if(**input == '&'){
	  return 1;
	}
	input++;
  }
  return 0;
}

//returns array indicating type of redirection and location of redirect token
int *check_redirect(char **input){
  char *tokens[3] = {">", "<", "|"};
  int *type = (int *) malloc(sizeof(int) * 2);
  //initialize type
  *type = -1;

  //for each token
  for(int i = 0; i < 3; i++){
	int location = 0;
	//for each element of the input
	while(**input != '\0'){
	  if(strcmp(*input, tokens[i]) == 0){
		type[0] = i;
		type[1] = location;
		return type;
	  }
	  input++;
	  location++;
	}
  }
  return type;
}

void execute(char **input){
  pid_t pid = fork();
  int status;
  int background = check_background(input);

  if(pid < 0){
	fprintf(stderr, "ERROR: Failed to fork\n");
	exit(1);
  }else if(pid == 0){
	//child process execution of command
	if(run_builtin(input)){
	  //first, check and run if input matches builtin function
	}else if(execvp(*input, input) <= 0){
	  //attempt to execute command from $PATH
	  fprintf(stderr, "ERROR: Failed to execute\n");
	  exit(1);
	}
  }else if(!background){
	//wait if child should not be in background
	while(wait(&status) != pid);
  }
}

int redirected_execute(int *type, char **input){
  if(*type == 2){
	//pipe stdout -> stdin
	char **second_command = &input[(*++type) + 1];
	input[*type] = '\0';
	printf("%s\n", *second_command);
  }else{
	char *file = (char *) malloc(sizeof(char) * 64);
	//set file to second bit of input, using location of type array
	file = input[(*++type) + 1];
	//truncate anything past redirect from input
	input[*type] = '\0';
	return out_to_file(input, file);
  }
  return 0;
}

int out_to_file(char **command, char *file){
  FILE *fp = fopen(file, "w");
  if(fp == NULL){
	fprintf(stderr, "ERROR: file not found %s", file);
	return -1;
  }
  int fd = fileno(fp);
  pid_t pid = fork();
  int out_pipe[2];
  int status;
  if(pipe(out_pipe) == -1){
	fprintf(stderr, "ERROR: pipe failed to initialize");
	return -1;
  }

  if(pid < 0){
	fprintf(stderr, "ERROR: Failed to fork\n");
	exit(1);
  }else if(pid == 0){
	close(out_pipe[0]);
	//redirect to stdout
	dup2(out_pipe[1], 1);
	//child process execution of command
	if(run_builtin(command)){
	  //first, check and run if input matches builtin function
	}else if(execvp(*command, command) <= 0){
	  //attempt to execute command from $PATH
	  fprintf(stderr, "ERROR: Failed to execute\n");
	  exit(1);
	}
  }else{
	close(out_pipe[1]);
	char read_buffer[1];
	while(read(out_pipe[0], read_buffer, 1) > 0){
	  write(fd, read_buffer, 1);
	}
	close(out_pipe[0]);
	while(wait(&status) != pid);
  }

  return 0;
}

//compare input to list of builtin functions, and execute if matched
//return 1 if match found
int run_builtin(char **input){
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
	_environ();
  else if(strcmp(command, "echo") == 0)
	echo(input);
  else if(strcmp(command, "help") == 0)
	help(input);
  else if(strcmp(command, "pause") == 0)
	_pause();
  else
	return 0;
  return 1;
}

int run_script(char *file){
  char args[1024];
  char *args_parsed[64];
  FILE *fp = fopen(file, "r");
  if(fp != NULL){
	return 1;
	fprintf(stderr, "ERROR: File not found %s\n", file);
  }else{
	//parse and execute script file line by line
	while(fgets(args, 1024, fp) != NULL){
	parse(args, args_parsed);
	execute(args_parsed);
	}
  }
  return 0;
}

int dispatch(char **input){
  int *redirect = check_redirect(input);
  if(*redirect != -1){
	//check for output redirection and execute there if required
	if(redirected_execute(redirect, input) == -1){
	  fprintf(stderr, "ERROR: Failed to redirect\n");
	  return -1;
	}
  }else{
	//otherwise execute normally
	execute(input);
  }
  return 0;
}

int main(int argc, char **argv){
  char args[1024];
  char *args_parsed[64];
  //if shell passed with argument
  if(argc >= 2){
	run_script(argv[1]);
	//otherwise prompt user
  }else{
	//char *exit_tokens[2] = {"exit", "q"}
	while(1){
		printf("MYSHELL > ");
		fgets(args, 1024, stdin);
		printf("\n");
		parse(args, args_parsed);

		//iterate through exit terms, return if input matches
		/*
		while(*exit_tokens != NULL){
		  if(strcmp(args_parsed[0], *exit_tokens))
			  return 0;
		  exit_tokens++;
		}
		*/
		if(strcmp(args_parsed[0], "exit") == 0 ){
		  return 0;
		}
		dispatch(args_parsed);
	}
  }
}

  //fix issue with exit- for each command entered, exit must be entered
  //an extra time to actually exit
  //something to do with mock prompts, i think

//still need to add io redirection
