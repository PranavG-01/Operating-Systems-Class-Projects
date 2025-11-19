#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/stat.h>

//node functions for the tree
typedef struct Node {
    char *name;
    unsigned char type;
    struct Node *first;
    struct Node *next;
}Node;

void make_tree(const char *dirname, Node* parent);
void traverseTree(Node* root);
void freeMemory(Node* root);

Node* root = NULL;


Node* createNode(const char* name, unsigned char type) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->name = strdup(name);
    newNode->type = type;
    newNode->first = NULL;
    newNode->next = NULL;
    return newNode;
}

void addNode(Node* parent, Node* child) {
    if (parent->first == NULL) {
        parent->first = child;
    } else {
        Node* temp = parent->first;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = child;
    }
}

//functions for the linked list queue

//defining the node of the queue
typedef struct qNode {
    Node* treeNode;
    char* fullPath;
    int level;
    struct qNode* next;
} qNode;

//defining the queue
typedef struct {
    qNode* front;
    qNode* rear;
} Queue;

//dont necessarily need this function, but I got errors from trying to make the queue in the code, this solution seemed to fix it
//function to create a new queue
void makeQueue(Queue* q) {
    q->front = NULL;
    q->rear = NULL;
}

//function to check if the queue is empty
//dont necessarily need this function either, but I got errors from trying to check if it is empty in the code, this solution seemed to fix it as well
int isEmpty(Queue* q) {
    if (q->front == NULL) {
        return 1;
    }
    return 0;
}

//enqueue function
void enqueue(Queue* q, Node* treeNode, const char* fullPath, int level) {
    qNode* newNode = (qNode*)malloc(sizeof(qNode));
    newNode->treeNode = treeNode;
    newNode->fullPath = strdup(fullPath);
    newNode->level = level;
    newNode->next = NULL;
    if (q->rear == NULL) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

//dequeue function
Node* dequeue(Queue* q, char** fullPath, int *level) {
    if (isEmpty(q)) {
        *fullPath = NULL;
        return NULL;
    }
    qNode* temp = q->front;
    Node* treeNode = temp->treeNode;
    *fullPath = temp->fullPath;
    *level = temp->level;
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(temp);
    return treeNode;
}

//given the comman line argument (directory name), argc is number of command line arguments
    //argc should be 2 (program name and directory name)
//argv is array of strings (char pointers) containing each argument
int main(int argc, char *argv[]) {

    //if argc is not 2 then the input is invalid
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }    

    root = createNode(argv[1], DT_DIR);

    make_tree(argv[1], root);
    traverseTree(root);
    freeMemory(root);
    return 0;
    
}

void make_tree(const char *dirname, Node* parent) {
    //directory stream pointer, uses library to open and read directories
        //opendir opens the directory stream
    DIR *directory = opendir(dirname);

    //struct pointer, as we read each file this struct has properties that will help distinguish between a file of subdirectory
        //whether it's a file, its size, name, etc
    struct dirent *currentObj;

    //simple check for errors throughout the code, it's a good practice
    if (directory == NULL) {
        perror(dirname);
        return;
    }

    //while loop: while the current object read is not NULL (NULL would mean the end of the directory)
        //readdir reads the next entry in the directory stream
    while ((currentObj = readdir(directory)) != NULL) {

        //recieved errors when trying to open . and .. directories
            //so I added this check to skip them
            //code written with the help of Github Copilot
        if (currentObj->d_name[0] == '.') {
            continue;
        }
        


        //add it to the tree
        Node* newNode = createNode(currentObj->d_name, currentObj->d_type);
        addNode(parent, newNode);

        size_t pathLength = strlen(dirname) + strlen(currentObj->d_name) + 2;
        char* path = malloc(pathLength);
        snprintf(path, pathLength, "%s/%s", dirname, currentObj->d_name);

        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            make_tree(path, newNode);
        } 

        free(path);

    }
    
    closedir(directory);
}

//using queue to traverse the tree and print the names of each node
void traverseTree(Node* root) {
    if (root == NULL) return;

    Queue q;
    makeQueue(&q);
    enqueue(&q, root, root->name, 1);

    int previousLevel = -1;
    int index = 0;

    while (!isEmpty(&q)) {
        char* currentPath;
        int level;

        Node* currentNode = dequeue(&q, &currentPath, &level);

        if (level != previousLevel) {
            index = 1;
            previousLevel = level;
        } else {
            index++;
        }

        printf("%d:%d:%s\n", level, index, currentPath);

        for (Node* child = currentNode->first; child != NULL; child = child->next) {
            size_t childPathLength = strlen(currentPath) + strlen(child->name) + 2;
            char* childPath = malloc(childPathLength);
            if (!childPath) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            snprintf(childPath, childPathLength, "%s/%s", currentPath, child->name);

            enqueue(&q, child, childPath, level + 1);

            free(childPath);
        }   

        free(currentPath);
    }
 
}

//Just as make_tree recursively goes through the directory to create the tree,
    //this function recursively goes through the tree to free memory
void freeMemory(Node* root) {
    if (root == NULL) { 
        return;
    }
    Node* iter = root->first;
    while(iter != NULL) {
        Node* iterNext = iter->next;
        freeMemory(iter);
        iter = iterNext;
    }
    free(root->name);
    free(root);
}

