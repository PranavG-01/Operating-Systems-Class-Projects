
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//needed for the key_t, ftok, msgget, msgsnd, msgrcv...
#include <sys/ipc.h>
#include <sys/msg.h>
//for pid and fork
#include <unistd.h>


#define MAXDIRPATH 1024
#define MAXKEYWORD 256
#define MAXLINESIZE 1024
#define MAXOUTSIZE 2048

//structure for the message sent from client to server
    //required info is the keyword, dirpath, and a unique key for the client

struct request {
    //msg_type is universally required as the first element of the structure, it tells the kernel the type of message being sent
    //you'll use different msg_types when there are differen readers of the queue, but for this project all client requests will be of the same type (1)
    //the kernel uses FIFO framework to go through queue, message type does NOT indicate priority, more info on this in the server code
    long msg_type;
    char keyword[MAXKEYWORD]; //keyword
    char dirpath[MAXDIRPATH]; //dirpath
    key_t client_key; //unique key for the client
};

//structure for the message sent from server to client
    //required info is the keyword and matched line
struct response {
    long msg_type;
    char keyword[MAXKEYWORD]; //keyword
    char sentence[MAXOUTSIZE]; //matched line
};
//problem with typedef struct is that when I try to initialize a server_msg variable, it gives an error unless I do struct server_msg varname;
//dont need typedef but don't know why I'm getting the error even when I have it



int main(int argc, char *argv[]) {
    //input goes as: ks_client keyword dirpath
    if (argc != 3) {
        printf("Usage: ./ksclient keyword dirpath\n");
        return 1;
    }

    //collect the command line arguments
    char *keyword = argv[1];
    char *dirpath = argv[2];

    //making sure the keyword is a single word (no spaces, tabs, newlines)
    for (char *c = keyword; *c != '\0'; c++) {
        if (*c == ' ' || *c == '\t' || *c == '\n') {
            printf("Error: keyword must be a single word\n");
            return 1;
        }
    }

    //prodcess identifier of the client
    pid_t pid = getpid();

    //key_t (ususally typedef int, long) is the data type used for inter-process communication (IPC)
        //used in shared memory, message queues, semaphores
    //ftok generates a key based on a given file path and an integer id, for communication between the client 
        //and server
        //if successful, it returns a key_t value, or it will return -1
    //key_t ftok(char *path, int proj_id);
    key_t serverKey = ftok("ks_server.c", 1);

    key_t clientKey = ftok("ks_client.c", (int)pid);
    
    //msgget is a system call that creates or retrieves a message queue identified by a key
        //like a location on a map, the key helps processes find the message queue they want to use
        //parameters: key_t key, int msgflg
        //msgflg specifies permissions and flags for the message queue (how the queue is accessed or created)
            //0666 = allow read/write for all users
            //IPC_CREAT = create the message queue if it does not already exist
            //IPC_EXCL = used with IPC_CREAT, ensures that a new queue is created and fails if the queue already exists

    //the masterQueues holds requests from each client (if there's more than one)
    int masterQueue = msgget(serverKey, 0666);
    //given that there may be multiple clients, this ensures that each client recieves responses intended for it
    //once the server program recieives the request, it will create threads to go through each file and return a response
        //all those reponses will be sent to this unique client queue
    int clientQueue = msgget(clientKey, IPC_CREAT | 0666);

    //now it's time to send the request message to the server

    //creating the message object to send to the server
    struct request message;
    message.msg_type = 1;
    message.client_key = clientKey;

    //copying arrays, cant jus do message.keyword = keyword;
        strncpy(message.keyword, keyword, MAXKEYWORD - 1);
        message.keyword[MAXKEYWORD - 1] = '\0';

        strncpy(message.dirpath, dirpath, MAXDIRPATH - 1);
        message.dirpath[MAXDIRPATH - 1] = '\0';

    //sending the message
    msgsnd(masterQueue, &message, sizeof(struct request) - sizeof(long), 0);

    if (strcmp(message.keyword, "exit") == 0) {
        //remove the client message queue from the system
        msgctl(clientQueue, IPC_RMID, NULL);
        return 0;
    }

    //now to wait for a response from the server
    struct response messageBack;
    while (1) {
        msgrcv(clientQueue, &messageBack, sizeof(struct response) - sizeof(long), 0, 0);

        //server letting client know that all responses have been sent
        if (strcmp(messageBack.sentence, "end") == 0) {
            break;
        }
        
        printf("%s:%s\n", keyword, messageBack.sentence);
    }

    //remove the client message queue from the system
    msgctl(clientQueue, IPC_RMID, NULL);

    return 0;
}