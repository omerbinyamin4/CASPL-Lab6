#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_ARGUMENTS 256

typedef struct cmdLine {
    char pipe;                               /* boolean incicating pipe*/
    char * firstCmd[MAX_ARGUMENTS];          /* command line arguments (arg 0 is the command)*/
    int firstArgCount;	                     /* number of arguments */
    char firstBlocking;	                     /* boolean indicating blocking/non-blocking */
    char *firstInputRedirect;	             /* input redirection path. NULL if no input redirection */
    char *firstOutputRedirect;	             /* output redirection path. NULL if no output redirection */
    
    
    char * secondCmd[MAX_ARGUMENTS];         /* command line arguments (arg 0 is the command)*/
    int secondArgCount;	                     /* number of arguments */
    char secondBlocking;	                 /* boolean indicating blocking/non-blocking */
    char *secondInputRedirect;	             /* input redirection path. NULL if no input redirection */
    char *secondOutputRedirect;	             /* output redirection path. NULL if no output redirection */
} cmdLine;

void changeDir(char* path);
void p_execute(cmdLine* cmd);
void runExecute(cmdLine* cmd);
void execute(cmdLine* cmd);
int shouldQuit(cmdLine* cmd);
int isCd(cmdLine* cmd);
cmdLine* parseLine(char* line);


int debugFlag = 0;

void runExecute(cmdLine* cmd) {
    if (cmd->pipe) p_execute(cmd);
    else execute(cmd);
}

void p_execute(cmdLine* cmd) {
    int pidChild1, pidChild2, status;
    int write_dup;
    int p[2];

    if (pipe(p) < 0) {
        perror("Failed to pipe: ");
        exit(1);
    }

    /* first child */ 
    if (!(pidChild1=fork())) {
        fclose(stdout);
        write_dup = dup(p[1]);
        close(p[1]);
        if (cmd->firstInputRedirect != NULL) {
            fclose(stdin);
            fopen(cmd->firstInputRedirect, "r");
        }
        execvp(cmd->firstCmd[0], cmd->firstCmd);
        perror("");
        exit(1);
    }

    close(p[1]);

    /* second child */
    if (!(pidChild2=fork())) {
        fclose(stdin);
        write_dup = dup(p[0]);
        close(p[0]);
        if (cmd->secondOutputRedirect != NULL) {
            fclose(stdout);
            fopen(cmd->secondOutputRedirect, "a");
        }
        execvp(cmd->secondCmd[0], cmd->secondCmd);
        perror("");
        exit(1);
    }

    close(p[0]);

    // if (debugFlag) fprintf(stderr, "PID: %d, Executing command: %s\n", pid, pCmdLine->arguments[0]);
    waitpid(pidChild1, &status, 0);
    waitpid(pidChild2, &status, 0);
}

void execute(cmdLine* cmd) {
    int pid, status;
    if (!(pid=fork())) {
        if (cmd->firstInputRedirect != NULL) {
            fclose(stdin);
            fopen(cmd->firstInputRedirect, "r");
        }
        if (cmd->firstOutputRedirect != NULL) {
            fclose(stdout);
            fopen(cmd->firstOutputRedirect, "a");
        }
        execvp(cmd->firstCmd[0], cmd->firstCmd);
        perror("");
        exit(1);
    }
    if (cmd->firstBlocking == 1) waitpid(pid, &status, 0);
}

int shouldQuit(cmdLine* cmd) {
    return strcmp(cmd->firstCmd[0], "quit") == 0;
}

int isCd(cmdLine* cmd) {
    return strcmp(cmd->firstCmd[0], "cd") == 0;
}

void changeDir(char* path) {
    int status = chdir(path);
    if (status == -1) perror("Failed to change directory: ");
}

cmdLine* parseLine(char* line) {
    line[strlen(line)-1] = '\0';
    int i = 0;
    char* argument;
    char* command;
    char* copy = strdup(line);
    cmdLine* CmdLine = (cmdLine*)malloc( sizeof(cmdLine));
    CmdLine->pipe = 0;
    command = strtok(line, "|");
    if (command != NULL) {
        argument = strtok(command, " ");
        while (argument != NULL) {
            if (strcmp(argument, "<") == 0) {
                argument = strtok(NULL, " ");
                CmdLine->firstInputRedirect = (char*) malloc(strlen(argument) + 1);
                strcpy(CmdLine->firstInputRedirect, argument);
                argument = strtok(NULL, " ");
                continue;
            }
            if (strcmp(argument, ">") == 0) {
                argument = strtok(NULL, " ");
                CmdLine->firstOutputRedirect = (char*) malloc(strlen(argument) + 1);
                strcpy(CmdLine->firstOutputRedirect, argument);
                argument = strtok(NULL, " ");
                continue;  
            }
            CmdLine->firstCmd[i] = (char*) malloc(strlen(argument) + 1);
            strcpy(CmdLine->firstCmd[i], argument);
            CmdLine->firstArgCount++;
            argument = strtok(NULL, " ");
            i++;
        }
        i = 0;
    }
    command = strchr(copy, '|');
    if (command != NULL) {
        command += 2;
        CmdLine->pipe = 1;
        argument = strtok(command, " ");
        while (argument != NULL) {
            if (strcmp(argument, "<") == 0) {
                argument = strtok(NULL, " ");
                CmdLine->secondInputRedirect = (char*) malloc(strlen(argument) + 1);
                strcpy(CmdLine->secondInputRedirect, argument);
                argument = strtok(NULL, " ");
                continue;
            }
            if (strcmp(argument, ">") == 0) {
                argument = strtok(NULL, " ");
                CmdLine->secondOutputRedirect = (char*) malloc(strlen(argument) + 1);
                strcpy(CmdLine->secondOutputRedirect, argument);
                argument = strtok(NULL, " ");
                continue;  
            }
            CmdLine->secondCmd[i] = (char*) malloc(strlen(argument) + 1);
            strcpy(CmdLine->secondCmd[i], argument);
            CmdLine->secondArgCount++;
            argument = strtok(NULL, " ");
            i++;
        }
    }
    return CmdLine;        
}

void addCmdLine() {
    
}

int main(int argc, char **argv) {
    char dirBuff[PATH_MAX];
    char userBuff[2048];
    int shouldQuitLoop = 0;
    cmdLine* parsedLine;
    

    if (argc > 1) debugFlag = !(strcmp(argv[1], "-d"));

    while (!shouldQuitLoop) {
        getcwd(dirBuff, PATH_MAX);
        printf("%s ", dirBuff);
        fgets(userBuff, 2048, stdin);
        parsedLine = parseLine(userBuff);
        if (parsedLine->firstArgCount <= 0)
            continue;
        shouldQuitLoop = shouldQuit(parsedLine);
        if (!shouldQuitLoop) {
            if (isCd(parsedLine)) changeDir(parsedLine->firstCmd[1]);
            else runExecute(parsedLine);
        }
    }
    return 0;
}