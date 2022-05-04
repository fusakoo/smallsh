#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFSIZE 2048
#define ARGSIZE 512

struct input{
  char* command;
  char* args[ARGSIZE];
  char* input;
  char* output;
};

int main(){
  printf("Starting smallsh!\n");
  printf("Current parent process's pid = %d\n", getpid());

  //int   childStatus;
  //pid_t childPid = fork();

  char buffer[BUFFSIZE];

  while(1){
    fprintf(stdout, ": ");
    fflush(stdout);
    /* Loop if input is empty */
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
      char* inputs[ARGSIZE];
      int inputsize= 0;
      const char delim[2] = " ";
      char *token;
      token = strtok(buffer, delim);

      while ( token != NULL ){ 
        inputs[inputsize] = token;
        token = strtok(NULL, delim);
        inputsize++;
      }
      /* Check the arguments */
      /* Check if the input is a comment (#) */ 
      if(strcmp(inputs[0],"#") == 0) {
        fprintf(stdout, ": ");
        for (int i = 0; i < inputsize; i++) {
          if (i != 0) {
            fprintf(stdout, " ");
          }
          fprintf(stdout, "%s", inputs[i]);
        }
        fflush(stdout);
      } else {
        /* Otherwise, check the arguments */
        int inputarg = 0;
        struct input* currInput = malloc(sizeof(struct input));
        //currInput->args = malloc(sizeof(char) * inputsize);
        for (int i = 0; i < inputsize; i++) {
          /* Command */
          if (i == 0) {
            currInput->command = calloc(strlen(inputs[i]) + 1, sizeof(char));
            strcpy(currInput->command, inputs[i]);
          }
          /* Input file */
          else if (strcmp(inputs[i], "<") == 0) {
            i++;
            currInput->input = calloc(strlen(inputs[i]) + 1, sizeof(char));
            strcpy(currInput->input, inputs[i]);
          }
          /* Output file */
          else if (strcmp(inputs[i], ">") == 0) {
            i++;
            currInput->output = calloc(strlen(inputs[i]) + 1, sizeof(char));
            char* outfile;
            sscanf(inputs[i],"%s",outfile);
            strcpy(currInput->output, outfile);
          }
          /* Arguments */
          else {
            //strcpy(currInput->args[inputarg] = inputs[i]);
            currInput->args[inputarg] = inputs[i];
            inputarg++;
          }
        }
        printf("Command: %s\n", currInput->command);
        printf("Output: %s\n", currInput->output);
        printf("Input: %s\n", currInput->input);
        int i = 0;
        while (currInput->args[i] != NULL) {
          printf("Arguments: %s\n", currInput->args[i]); 
          i++;
        }
      }
    }
    /* If there's no input, loop again */
  }

  //if(childPid == -1){
  //  perror("fork() failed!");
  //  exit(1);
  //} else if(childPid == 0){
  //  // Child process executes this branch
  //  sleep(10);
  //} else{
  //  // The parent process executes this branch
  //  printf("Child's pid = %d\n", childPid);
  //  // WNOHANG specified. If the child hasn't terminated, waitpid will immediately return with value 0
  //  childPid = waitpid(childPid, &childStatus, WNOHANG);
  //  printf("In the parent process waitpid returned value %d\n", childPid);
  //}
  //printf("The process with pid %d is returning from main\n", getpid());
  return 0;
}
