# smallsh
This is a C implementation of a bash-like shell that was created as part of CS344 - Operating Systems I course at Oregon State University.

## Features
The following features are implemented in this program:
1. Provide a prompt for running commands
2. Handle blank lines and comments, which are lines beginning with the # character
3. Provide expansion for the variable $$
4. Execute 3 commands exit, cd, and status via code built into the shell
5. Execute other commands by creating new processes using a function from the exec family of functions
6. Support input and output redirection
7. Support running commands in foreground and background processes
8. Implement custom handlers for 2 signals, SIGINT and SIGTSTP

## Complile Code
To complile the code, use the following line using gcc:
```gcc -std=gnu99 -o smallsh smallsh.c```
