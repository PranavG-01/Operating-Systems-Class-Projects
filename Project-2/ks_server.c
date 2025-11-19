#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
//for threading
#include <pthread.h>
//for directory reading
#include <dirent.h>
#include <sys/stat.h>
//for fork and wait
#include <wait.h>
//used to differentiate word boundaries (spaces, punctuation, newlines)
//isspace and ispunct
#include <ctype.h>

//keeping constants and struct definitions consistent with client
#define MAXDIRPATH 1024
#define MAXKEYWORD 256
#define MAXLINESIZE 1024
#define MAXOUTSIZE 2048

struct request {
    long msg_type;
    char keyword[MAXKEYWORD];
    char dirpath[MAXDIRPATH];
    key_t client_key;
};

struct response {
    long msg_type;
    char keyword[MAXKEYWORD];
    char sentence[MAXOUTSIZE];
};

struct threadData {
    char filepath[MAXDIRPATH];
    char keyword[MAXKEYWORD];
    int clientId;
};

//originally this function checked for spaces, tabs, newlines only to differentiate the vs another, but forgot to check for special characters
//found this solution online to add the c != ... checks to the ispunct function call
int isWordBoundary(char c) {
    return c == '\0' || isspace((unsigned char)c) || (ispunct((unsigned char)c) && c != '-' && c != '.' && c != '(' && c != ')');
}

//search each file for the keyword
void *searchPath(void *arg) {
    //the pointer of the thread data
    struct threadData *data = (struct threadData *)arg;

    FILE *entry = fopen(data->filepath, "r");

    char line[MAXLINESIZE];
    struct response reply;
    reply.msg_type = 1;
    strcpy(reply.keyword, data->keyword);

    size_t keylength = strlen(data->keyword);
    //fgets function reads the lines one by one until the EOF
    while (fgets(line, sizeof(line), entry)) {
        char *p = line;
        //the strstr function finds the first occurrence of the keyword in the line
        char beforeWord, afterWord;
        while ((p = strstr(p, data->keyword)) != NULL) {
            if (p == line) {
                beforeWord = '\0';
            }
            else {
                beforeWord = *(p - 1);
            }
            afterWord = *(p + keylength);

            //checking to make sure the word is not part of another word and if not, send the matched line to the client
            if (isWordBoundary(beforeWord) && isWordBoundary(afterWord))
            {
                //removes the chance for any garbage output after the newline character
                line[strcspn(line, "\n")] = '\0';
                //making sure to not overflow the sentence buffer
                strncpy(reply.sentence, line, MAXLINESIZE);
                reply.sentence[MAXLINESIZE - 1] = '\0';
                msgsnd(data->clientId, &reply, sizeof(struct response) - sizeof(long), 0);
                //even if the keyword is presented multiple times, break prevents the message from sending again
                //this takes care of the "a a a" case
                break;
            }
            p += keylength;
        }
    }

    fclose(entry);
    pthread_exit(NULL);
}

int main() {
    key_t key = ftok("ks_server.c", 1);
    if (key == -1) 
    { 
        perror("ftok server"); return 1; 
    }

    int masterQueue = msgget(key, IPC_CREAT | 0666);
    if (masterQueue == -1) 
    { 
        perror("msgget server"); return 1;
    }

    //once running, server stays on until terminated with the 'exit' keyword
    //still need the directory path to avoid errors, but it won't be used
    while (1) {
        struct request message;
        if (msgrcv(masterQueue, &message, sizeof(struct request) - sizeof(long), 0, 0) == -1) {
            perror("[Server] msgrcv");
            continue;
        }

        if (strcmp(message.keyword, "exit") == 0) {
            msgctl(masterQueue, IPC_RMID, NULL);
            exit(0);
        }
        
        //creating the new process
        pid_t pid = fork();
        if (pid < 0) {
            perror("error fork");
            continue;
        } 
        else if (pid == 0) {
            DIR *dir = opendir(message.dirpath);
            if (!dir) {
                perror("error opendir");
                exit(1);
            }

            int clientQueue = msgget(message.client_key, 0666);
            if (clientQueue == -1) {
                perror("error msgget client");
                exit(1);
            }

            struct dirent *entry;
            //creating threads for each file in the directory
            pthread_t threads[100];
            struct threadData tdata[100];
            int threadCount = 0;

            while ((entry = readdir(dir)) != NULL) {
                char fullpath[MAXDIRPATH];
                //for both snfprint statements the third argument uses a trick to prevent buffer overflow
                //It ensures that the combined length of the directory path, filename, and null terminator does not exceed MAXDIRPATH
                //I got this answer using ChatGPT, after struggling to figure it out myself and with resources online
                snprintf(fullpath, sizeof(fullpath), "%.*s/%s", (int)(MAXDIRPATH - 1 - strlen(entry->d_name)), message.dirpath, entry->d_name);

                
                struct stat directoryPath;
                //checking if the entry is a file and if the information can be retrieved successfully
                if (stat(fullpath, &directoryPath) == 0 && S_ISREG(directoryPath.st_mode)) {

                    snprintf(tdata[threadCount].filepath, MAXDIRPATH, "%.*s/%s", (int)(MAXDIRPATH - 1 - strlen(entry->d_name)), message.dirpath, entry->d_name);
                    strcpy(tdata[threadCount].keyword, message.keyword);
                    tdata[threadCount].clientId = clientQueue;
                    //create thread to search the file
                    pthread_create(&threads[threadCount], NULL, searchPath, &tdata[threadCount]);
                    threadCount++;
                }
            }
            closedir(dir);

            //wait for each thread to complete before sending the "end" message
            for (int i = 0; i < threadCount; i++) {
                pthread_join(threads[i], NULL);
            }

            // Send "end" message to client
            struct response endMsg;
            endMsg.msg_type = 1;
            strcpy(endMsg.keyword, message.keyword);
            strcpy(endMsg.sentence, "end");
            msgsnd(clientQueue, &endMsg, sizeof(struct response) - sizeof(long), 0);

            exit(0);
        } 
        //if pid is greater than 0, parent proces
        else {
            int status;
            waitpid(pid, &status, WNOHANG);
        }
    }

    return 0;
}
