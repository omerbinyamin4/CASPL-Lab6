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
    
    char * secondCmd[MAX_ARGUMENTS];         /* command line arguments (arg 0 is the command)*/
    int secondArgCount;	                     /* number of arguments */
    char secondBlocking;	                 /* boolean indicating blocking/non-blocking */
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
            CmdLine->secondCmd[i] = (char*) malloc(strlen(argument) + 1);
            strcpy(CmdLine->secondCmd[i], argument);
            CmdLine->secondArgCount++;
            argument = strtok(NULL, " ");
            i++;
        }
        (CmdLine->secondCmd[i-1])[strlen(CmdLine->secondCmd[i-1])-1] = '\0';
    }
    if (!(CmdLine->pipe)) 
        (CmdLine->firstCmd[CmdLine->firstArgCount-1])[strlen(CmdLine->firstCmd[CmdLine->firstArgCount-1])-1] = '\0';
    return CmdLine;




    // int i = 0, j = 0;
    // char** output;

    // char* command;
    // char* argument;

    // while( (command = strsep(&line," | ")) != NULL ) {
    //     *(output + i) = (char*) malloc(strlen(command)*4);
    //     while ( (argument = strsep(&command, " ")) != NULL) {
    //         *(*(output + i) + j) = argument;
    //         j++;
    //     }
    //     i++;
    // }
        
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