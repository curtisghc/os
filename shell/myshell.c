#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

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
	//child process try to execute
	if(execvp(*input, input) <= 0){
	  fprintf(stderr, "ERROR: Failed to execute\n");
	  exit(1);
	}
  }else if(!background){
	//wait if child should not be in background
	while(wait(&status) != pid);
  }
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

}
