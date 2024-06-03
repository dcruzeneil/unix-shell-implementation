#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>

/* Function Prototypes */
int tokenizer(char[], char*[]);
int countProcess(char*[], int);
void executeProcess(char*[], int, int, char[], char[]);
int formatNumbers(int, int);

/* Constant Values */
#define maxInputSize 4096
#define maxTokenNumber 4096
#define maxArgument 11
#define inDefaultOutPipe 03
#define inFileOutPipe 13
#define inPipeOutPipe 23
#define inFile 1
#define inPipe 2
#define outFileTruncate 1
#define outFileAppend 2
#define outPipe 3

/* Global Variables */
int pipeFileDescriptors[2];
int fdTemporary = 0;

int main(int arc, char* argv[]){
    /*
        IO Redirection: 
        INPUT:  STDIN (0),  < (1), | (2)
        OUTPUT: STDOUT (0), > (1), >> (2), | (3) 
    */ 
    char userInput[maxInputSize];
    char* tokens[maxTokenNumber];
    char* process[maxArgument];
    char filePathRead[FILENAME_MAX]; 
    char filePathWrite[FILENAME_MAX];    
    size_t inputLength; 
    int numberOfTokens;
    int numberOfProcesses;
    int input = 0;
    int output = 0;
    int counter;
    char* fgetsReturn;
    int pipingDone = 0;

    /* Infinite Loop: until user types "exit" or hits EOF */
    while(true){
        /* Initializing Variables */
        counter = 0;
        for(int j = 0; j < maxTokenNumber; j++){
            tokens[j] = 0;
        }
        pipingDone = 0;

        /* Printing the Prompt */
        printf("$ ");

        /* Getting User Input + Removing newline character */
        fgetsReturn = fgets(userInput, maxInputSize, stdin);
        if(fgetsReturn == NULL){
            break;
        }
        inputLength = strlen(userInput);
        userInput[inputLength - 1] = '\0';

        /* Exit Case: User has Entered "exit" */
        if(strcmp("exit", userInput) == 0){
            break;
        }

        /* Tokenizing the User Input */
        numberOfTokens = tokenizer(userInput, tokens); 
        numberOfProcesses = countProcess(tokens, numberOfTokens);

        /* Reading the Token and File Path */
        // going over total number of tokens
        while(counter < numberOfTokens){
            // reinitializing the variables
            input = 0;
            output = 0;
            filePathRead[0] = '\0';
            filePathWrite[0] = '\0';
            for(int j = 0; j < maxArgument; j++){
                process[j] = 0;
            } 

            // going over the command + parameters for a single command
            for(int j = 0; j < maxTokenNumber; j++){
                // piping
                if(strcmp(tokens[counter], "|") == 0){
                    pipingDone = 1;
                    // an input coming in from pipe
                    if(j == 0){
                        input = 2;
                        counter++;
                        j = j - 1; // to adjust the parsing because | sort
                        // PIPE CLOSING: when receiving from pipe - can close the write end of the pipe 
                        close(pipeFileDescriptors[1]); 
                    }
                    // an output going into a pipe
                    else{
                        output = 3;
                        executeProcess(process, input, output, filePathRead, filePathWrite);    
                        break;
                    }
                }
                // Input Redirection: < 
                else if(strcmp(tokens[counter], "<") == 0){
                    input = 1;
                    strcpy(filePathRead, tokens[++counter]);
                    counter++;
                }
                // Output Redirection: >
                else if(strcmp(tokens[counter], ">") == 0){
                    output = 1;
                    strcpy(filePathWrite, tokens[++counter]);
                    counter++;
                }
                // Output Redirection: >> 
                else if(strcmp(tokens[counter], ">>") == 0){
                    output = 2;
                    strcpy(filePathWrite, tokens[++counter]);
                    counter++;
                }
                // Otherwise needed to run process
                else{
                    process[j] = tokens[counter];
                    counter++;
                }

                // In case of single commands
                if(counter == numberOfTokens){
                    executeProcess(process, input, output, filePathRead, filePathWrite);
                    break;
                }
            }
        }

        // closing all open pipes
        if(pipingDone == 1){
            close(pipeFileDescriptors[0]);
            close(pipeFileDescriptors[1]);
        }

        // waiting for children to finish
        for(int j = 0; j < numberOfProcesses; j++){
            wait(NULL);
        }
    }
}

int tokenizer(char userInput[], char* tokens[]){ 
    char* currentToken;
    int counter = 0;

    currentToken = strtok(userInput, " ");
    tokens[counter] = currentToken;

    while(currentToken != NULL){
        currentToken = strtok(NULL, " ");
        tokens[++counter] = currentToken;
    }

    return counter;
}

void executeProcess(char* processArguments[], int input, int output, char filePathRead[], char filePathWrite[]){
    int dupStatusR = 0;
    int dupStatusW = 0;
    int fd_read = 0;
    int fd_write = 0;
    int pipeStatus = 0;
    mode_t mode = 0666;
    int ioRedirection = formatNumbers(input, output);
    pid_t child; 
    int executionStatus;

    // pipes needs to happen in parent function
    switch(ioRedirection){
        case inDefaultOutPipe:
            pipeStatus = pipe(pipeFileDescriptors);
            break;
        case inFileOutPipe:
            pipeStatus = pipe(pipeFileDescriptors);
            break;
        case inPipeOutPipe:
            fdTemporary = pipeFileDescriptors[0];
            // PIPE CLOSING: closing the write end of the pipe 
            close(pipeFileDescriptors[1]);
            pipeStatus = pipe(pipeFileDescriptors);
            break;
        default: 
            break;
    }

    if(pipeStatus == -1){
        perror("pipe");
        return;
    }

    // only the child execs - the parent goes back to the main function 
    child = fork();

    if(child == 0){
        // dup only happens within child
        switch(input){
            // Input: <  
            case inFile: 
                fd_read = open(filePathRead, O_RDONLY, mode);
                dupStatusR = dup2(fd_read, 0);
                break;
            // Input: |             
            case inPipe:
                if(output == 3){
                    dupStatusR = dup2(fdTemporary, 0);
                    break;
                }
                dupStatusR = dup2(pipeFileDescriptors[0], 0);
                break;
            default:
                break;
        }
        if(dupStatusR == -1){
            perror("dup2");
            exit(1);
        }
        if(fd_read == -1){
            perror("open");
            exit(1);
        }

        switch(output){
            // Output: > 
            case outFileTruncate:
                fd_write = open(filePathWrite, O_CREAT | O_WRONLY | O_TRUNC, mode);
                dupStatusW = dup2(fd_write, 1);
                break;
            // Output: >> 
            case outFileAppend:
                fd_write = open(filePathWrite, O_CREAT | O_WRONLY | O_APPEND, mode);
                dupStatusW = dup2(fd_write, 1);
                break;
            // Output: |
            case outPipe:
                close(pipeFileDescriptors[0]);
                dupStatusW = dup2(pipeFileDescriptors[1], 1);
                break;
            default:
                break;
        }
        if(dupStatusW == -1){
            perror("dup2");
            exit(1);
        }
        if(fd_write == -1){
            perror("open");
            exit(1);
        }

        executionStatus = execvp(processArguments[0], processArguments);
        if(executionStatus == -1){
            exit(1);
        }
    }
}

int formatNumbers(int a, int b){
    char firstDigit[2];
    char secondDigit[2];
    int formattedNumber;
    // converting integers to strings
    sprintf(firstDigit, "%d", a);
    sprintf(secondDigit, "%d", b);
    // concatenating the strings
    strcat(firstDigit, secondDigit);
    // converting the string to an integer
    formattedNumber = atoi(firstDigit);
    return formattedNumber;
}

int countProcess(char* tokens[], int number){
    int processes = 1;
    int i;

    for(i = 0; i < number; i++){
        if(strcmp(tokens[i], "|") == 0){
            processes += 1;
        }
    }
    return processes;
}