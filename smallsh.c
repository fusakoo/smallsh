#define _POSIX_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <err.h>
#include <errno.h>
#include <signal.h>

#define BUFFSIZE 2048
#define ARGSIZE 512
#define MAXCHILDREN 10

/*
 * TODO
 * Ctrl + C shouldn't print newline
 * Figure out the empty input issue
 * Modularization
 * Signal output
 * Kill defunct (zombie) process
 */

volatile sig_atomic_t gRunForeground = 1;
volatile sig_atomic_t gForegroundActive = 0;
volatile sig_atomic_t gForegroundMode = 0;

struct input{
  char* command;
  char* args[ARGSIZE];
  char* input;
  char* output;
};

// Handler for SIGINT
void handle_SIGINT(int signo) {
  if (gForegroundActive == 1) {
    gForegroundActive = 0;
    gRunForeground = 0;
    char* message = "\nTerminated by signal 2\n";
    write(STDOUT_FILENO, message, 24);
  }
}

// Handler for SIGTSTP
void handle_SIGTSTP(int signo) {
  if (gForegroundMode == 0) {
    char* entryMessage = "\nEntering foreground-only mode (& is now ignored)\n";
    write(STDOUT_FILENO, entryMessage, 50);
    gForegroundMode = 1;
  }
  else {
    char* exitMessage = "\nExiting foreground-only mode\n";
    write(STDOUT_FILENO, exitMessage, 30);
    gForegroundMode = 0;  
  }
}

// Function to process $$ expansion to pid
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
  printf("Welcome to smallsh!\n");
  fflush(stdout);

  int status;
  pid_t pids[MAXCHILDREN] = {0};
  int childcount = 0;

  pid_t parentPid = getpid();
  char buffer[BUFFSIZE];

  while(1){
    /* Clear out the input buffer*/
    memset(buffer, 0, sizeof(buffer));
    
    /*
     * 8. Signals SIGINT & SIGTSTP
     * Reference: Exploration - Signal Handling API
     * Reference: Ed Discussion post #442
     */
    struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};

    // Fill out the SIGTSTP_action struct
    // Register handle_SIGTSTP as the signal handler
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    
    // Fill out the SIGINT_action struct
    // Register handle_SIGINT as the signal handler
    SIGINT_action.sa_handler = handle_SIGINT;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    /* Check status of the background process */
    for (int i = 0; i < MAXCHILDREN; i++) {
      if (pids[i] != 0){
        if (waitpid(pids[i], &status, WNOHANG) != 0) {
          printf("Background pid %d complete: ", pids[i]);
          fflush(stdout);
          if (WIFEXITED(status)) {
            printf("Exit status %d\n", WEXITSTATUS(status));
            fflush(stdout);
          } else {
            printf("Terminated by signal %d\n", WTERMSIG(status));
            fflush(stdout);
          }
          pids[i] = 0;
          childcount--;
        }
      }
    }
    
    /*
    * 1. Command Prompt
    */
    printf(": ");
    fflush(stdout);
    /* Loop if input is empty */
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
      /*
      * 3. Expansion of Variable $$
      */
      if (strstr(buffer,"$$")) {
        var_expansion(buffer);
      }

      /* 
      * 2. Comment & Blank Lines
      */
      char* inputs[ARGSIZE];
      int inputsize = 0;
      int inputarg = 0;
      const char delim[2] = " ";
      char* token;
      token = strtok(buffer, delim);

      /* Handle arguments (and blank inputs) */
      while ( token != NULL ) {
        inputs[inputsize] = token;
        token = strtok(NULL, delim);
        inputsize++;
      }

      /* Clear out the input buffer*/

      /* Check if the input is a comment (#) */
      // TODO: Check if it's an empty string, e.g. " "
      if ((strpbrk(inputs[0],"#") == NULL) || (strcmp(inputs[0], "\0") == 0)) {
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
            if (gForegroundMode == 0) {
              background = true;
            }
          }
          /* Arguments */
          else {
            currInput->args[inputarg] = inputs[i];
            inputarg++;
          }
        }
        
        /*
        * 4. Built-in Commands
        */

        /* exit */
        if (strcmp(currInput->command,"exit") == 0) {
          printf("Exiting smallsh!\n");
          fflush(stdout);
          // Kill any child process before exiting
          for (int i = 0; i < MAXCHILDREN; i++) {
            if (pids[i] != 0) {
              kill(pids[i], SIGTERM);
            }
          }
          free(currInput->command);
          free(currInput);
          exit(0);
          return(0);
        }
        /* cd */
        else if (strcmp(currInput->command, "cd") == 0) {
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
        // Reference: Exploration: Process API - Monitoring Child Processes
        else if (strcmp(currInput->command, "status") == 0) {
          waitpid(parentPid, &status, WNOHANG);
          if (WIFEXITED(status)) {
            printf("Exit status %d\n", WEXITSTATUS(status));
            fflush(stdout);
          } else {
            printf("Terminated by signal %d\n", WTERMSIG(status));
            fflush(stdout);
          }
        }
        /*
        * 5. Execute Other Commands
        */
        else {
          /* Command + arguments + NULL */
          int component = 1 + inputarg + 1;
          char *newargv[component];
          newargv[0] = currInput->command;
          if (inputarg > 0) {
            int argcount = 0;
            for (int i = 1; i <= inputarg; i++) {
              newargv[i] = currInput->args[argcount];
              argcount++;
            }
          }
          newargv[component - 1] = NULL;
          
          // Prevent forking if max number of child processes has been reached
          if (childcount == 10) {
            err(errno=EAGAIN, "Max number of child process reached");
            fflush(stderr);
            exit(0);
            break;
          }

          pid_t spawnPid = fork();

          switch(spawnPid) {
          case -1:
            perror("Fork() failed");
            fflush(stderr);
            exit(1);
            break;
          case 0: 
            /* Child process */
            
            /*
             * 6. Input and output redirection
             */
            /* Input */
            if (currInput->input != NULL ) {
              int sourceFD = open(currInput->input, O_RDONLY);
              fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
              if (sourceFD == -1){
                perror("Open()");
                fflush(stderr);
                exit(1);
              }
              int result = dup2(sourceFD, 0);
              if (result == -1){
                perror("Source dup2()");
                fflush(stderr);
                exit(2);
              }
            }
            else {
              // No input dest specified
              // If background process -> direct to /dev/nul
              if (background == true) {
                int sourceFD = open("/dev/null", O_RDONLY);
                fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
                if (sourceFD == -1){
                  perror("Open()");
                  fflush(stderr);
                  exit(1);
                }
                int result = dup2(sourceFD, 0);
                if (result == -1){
                  perror("Source dup2()");
                  fflush(stderr);
                  exit(2);
                }
              }
            }
            /* Output */
            if (currInput->output != NULL) {
              int targetFD = open(currInput->output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
              fcntl(targetFD, F_SETFD, FD_CLOEXEC);
              if (targetFD == -1){
                perror("Open()");
                fflush(stderr);
                exit(1);
              }
              int result = dup2(targetFD, 1);
              if (result == -1){
                perror("Target dup2()");
                fflush(stderr);
                exit(2);
              }
            }
            else {
              // No output dest specified
              // If background process -> direct to /dev/nul
              if (background == true) {
                int targetFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                fcntl(targetFD, F_SETFD, FD_CLOEXEC);
                if (targetFD == -1){
                  perror("Open()");
                  fflush(stderr);
                  exit(1);
                }
                int result = dup2(targetFD, 1);
                if (result == -1){
                  perror("Target dup2()");
                  fflush(stderr);
                  exit(2);
                }
              }
            }
            execvp(newargv[0], newargv);
            perror(newargv[0]);
            fflush(stderr);
            exit(2);
            break;
          default:
            /* Background process */
            // If it's a background process specified with &
            if (background == 1) {
              printf("Running the command in the background (%d).\n", spawnPid);
              fflush(stdout);
              //store_pid(pids, spawnPid, childcount);
              for (int i = 0; i < MAXCHILDREN; i++) {
                if (pids[i] == 0) {
                  pids[i] = spawnPid;
                  childcount++;
                  break;
                }
                // If there is too many children
                else if ((i == (MAXCHILDREN - 1)) && (pids[i] != 0) ){
                  err(errno=EAGAIN, "Max child process count reached");
                  fflush(stderr);
                  exit(1);
                }
              }
              spawnPid = waitpid(spawnPid, &status, WNOHANG);
            }
            else {
              /* Foreground process */
              // TODO: make sure to remove this
              gRunForeground = 1;
              gForegroundActive = 1;
              while (gRunForeground) {
                spawnPid = waitpid(spawnPid, &status, 0);
                gRunForeground = 0;
              }
            }
          }
        }
      }
    }
    /* If there's no input, loop again */
  }

  return 0;
}
