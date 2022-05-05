#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BUFFSIZE 2048
#define ARGSIZE 512

struct input{
  char* command;
  char* args[ARGSIZE];
  char* input;
  char* output;
};

int *var_expansion(char *input) {
  pid_t pid = getpid();
  char* pidstr;
  /* Reference: Ed Discussion post #355 */
  {
    int n = snprintf(NULL, 0, "%d", pid);
    pidstr = malloc((n + 1) * sizeof *pidstr);
    sprintf(pidstr, "%d", pid);
  }
  int pidlen = strlen(pidstr);
  while(1){
    char *p = strstr(input, "$$");
    if (p != NULL) {
      memmove(p + pidlen, p + 2, strlen(p+2) + 2);
      memcpy(p, pidstr, pidlen);
    } else {
      break;
    } 
  }
  return 0; 
}

int main(){
  printf("Starting smallsh!\n");

  int status;
  pid_t parentPid = getpid();
  printf("Current parent process's pid = %d\n", parentPid);

  char buffer[BUFFSIZE];

  while(1){
    /*
    * 1. Command Prompt
    */
    fprintf(stdout, ": ");
    fflush(stdout);
    /* Loop if input is empty */
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
      /*
      * 3. Expansion of Variable $$
      */
      printf("Old Buffer: %s", buffer);
      if (strstr(buffer,"$$")) {
        var_expansion(buffer);
      }
      printf("New Buffer: %s", buffer);

      /* 
      * 2. Comment & Blank Lines
      */
      char* inputs[ARGSIZE];
      int inputsize= 0;
      int inputarg = 0;
      const char delim[2] = " ";
      char *token;
      token = strtok(buffer, delim);

      /* Handle arguments (and blank inputs) */
      while ( token != NULL ) { 
        inputs[inputsize] = token;
        token = strtok(NULL, delim);
        inputsize++;
      }

      /* Check if the input is a comment (#) */ 
      if (strcmp(inputs[0],"#") == 0) {
        fprintf(stdout, ": ");
        for (int i = 0; i < inputsize; i++) {
          if (i != 0) {
            fprintf(stdout, " ");
          }
          fprintf(stdout, "%s", inputs[i]);
        }
        fflush(stdout);
      }
      else {
        /* Struct based on input to process command line */
        bool background = false;
        
        struct input* currInput = malloc(sizeof(struct input));
        for (int i = 0; i < inputsize; i++) {
          /* ToDo
          * Modularize the clean up 
          * Handle "   " (space) inputs
          */

          /* Clean this code later */
          if (i == inputsize - 1) {
            char temp[strlen(inputs[i])];
            sscanf(inputs[i], "%s", temp);
            strcpy(inputs[i],temp);
          }
          /* Command */
          if (i == 0) {
            currInput->command = calloc(strlen(inputs[i]) + 1, sizeof(char));
            strcpy(currInput->command, inputs[i]);
          }
          /* Input file */
          else if (strcmp(inputs[i], "<") == 0) {
            i++;
            /* Clean this code later */
            if (i == inputsize - 1) {
              char temp[strlen(inputs[i])];
              sscanf(inputs[i], "%s", temp);
              strcpy(inputs[i],temp);
            }
            currInput->input = calloc(strlen(inputs[i]) + 1, sizeof(char));
            strcpy(currInput->input, inputs[i]);
          }
          /* Output file */
          else if (strcmp(inputs[i], ">") == 0) {
            i++;
            /* Clean this code later */
            if (i == inputsize - 1) {
              char temp[strlen(inputs[i])];
              sscanf(inputs[i], "%s", temp);
              strcpy(inputs[i],temp);
            }
            currInput->output = calloc(strlen(inputs[i]) + 1, sizeof(char));
            strcpy(currInput->output, inputs[i]);
          }
          /* Background or foreground process */
          else if (strcmp(inputs[i], "&") == 0) {
            background = true;
          }
          /* Arguments */
          else {
            currInput->args[inputarg] = inputs[i];
            inputarg++;
          }
        }
        /* Debugging */
        printf("Command: %s\n", currInput->command);
        fflush(stdout);
        printf("Output: %s\n", currInput->output);
        fflush(stdout);
        printf("Input: %s\n", currInput->input);
        fflush(stdout);
        printf("Background process? %d\n", background);
        fflush(stdout);
        int i = 0;
        while (currInput->args[i] != NULL) {
          printf("Arguments: %s\n", currInput->args[i]); 
          fflush(stdout);
          i++;
        }
        /*
        * 4. Built-in Commands
        */

        /* exit */
        if (strcmp(currInput->command,"exit") == 0) {
          printf("Run exit command\n");
          fflush(stdout);
          exit(0);
          return(0);
        }
        /* cd */
        else if (strcmp(currInput->command, "cd") == 0) {
          printf("Run cd command\n");
          if (currInput->args[0] == NULL) {
            chdir(getenv("HOME"));
            char cwd[100];
            printf("Currently in: %s\n", getcwd(cwd,sizeof(cwd)));
            fflush(stdout);
          } else {
            chdir(currInput->args[0]);
            char cwd[100];
            printf("Currently in: %s\n", getcwd(cwd,sizeof(cwd)));
            fflush(stdout);
          }
        }
        /* status */
        /* Reference: Exploration: Process API - Monitoring Child Processes */
        else if (strcmp(currInput->command, "status") == 0) {
          printf("Run status command\n");   
          fflush(stdout);
          waitpid(parentPid, &status,WNOHANG);
          //printf("waitpid returned: %d\n", parentPid);
          fflush(stdout);
          if (WIFEXITED(status)) {
            printf("Last foreground process exited normally with status %d\n", WEXITSTATUS(status));
          } else {
            printf("Last foreground process %d exited abnormally due to signal %d\n", WTERMSIG(status));
          }
        }
        /*
        * 5. Execute Other Commands
        */
        else {
          /* Command + arguments + NULL */
          int argcount = 1 + inputarg + 1;
          char *newargv[argcount];
          newargv[0] = currInput->command;
          for (int i = 0; i < inputarg; i++) {
            newargv[i] = currInput->args[inputarg];
          }
          newargv[argcount - 1] = NULL;
          
          //int childStatus;
          pid_t spawnPid = fork();

          switch(spawnPid) {
          case -1:
            perror("Fork() failed");
            exit(1);
            break;
          case 0: 
            /* Child process */
            printf("Child (%d) is running the command\n", getpid());
            int outcome = execvp(newargv[0], newargv);
            perror("execvp");
            exit(2);
            break;
          default:
            /* Parent process */
            spawnPid = waitpid(spawnPid, &status, 0);
          }
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
