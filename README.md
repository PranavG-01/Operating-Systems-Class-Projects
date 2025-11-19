# Operating-Systems-Class-Projects
These are the projects I completed while in UofL CSE 420

Project 1:
  C Code that takes in a directory path as an input and outputs a structured, level-by-level list of all files and folders starting from within the the root directory (the given folder). Designed to run in a Linux terminal. More specific info on the README file in the Project-1 folder.

Project 2:
  This project implements a client-server architecture. The overall application is a multi-process, multi-threaded keyword search system. The ks_server.c file (server) runs in the background while the user would interface with ks_client.c (the client). the "client" code takes in a directory path and keyword as inputs and sends it to the "server", which forks a child process to handle the request. When the request is complete, the outputs from the "server" are sent to the "client", which outputs them in an organized list for the user to see. More specific info on the README file in the Project-2 folder.
