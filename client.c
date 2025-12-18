//Citation: Server Exercise in class
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "message.h"
#include "socket.h"

/**
 * Listens for a message from server
 * @return NULL
 */
void* listen_thread(void* args) {
  //get socket from args
  intptr_t socket_fd = (intptr_t)args;
    // Read a message back from the server
    while(true) {
      //create and print message
      char* in_message = receive_message(socket_fd);
      if (in_message == NULL) {
        perror("Failed to read message from server");
        exit(EXIT_FAILURE);
      }
      printf("%s", in_message);
    }
  return NULL;
}

int main(int argc, char** argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <server name> <port> <username>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Read command line arguments
  char* server_name = argv[1];
  unsigned short port = atoi(argv[2]);
  char* username = argv[3];

  // Connect to the server
  int socket_fd = socket_connect(server_name, port);
  if (socket_fd == -1) {
    perror("Failed to connect");
    exit(EXIT_FAILURE);
  }


  char message[256];

  // send username to server
  int rc = send_message(socket_fd, username);
  if (rc == -1) {
    perror("Failed to send message to server");
    exit(EXIT_FAILURE);
  } 

  //create a listener thread
  pthread_t listener;
  pthread_create(&listener, NULL, listen_thread, (void*)(intptr_t)socket_fd);
  while(true) {
    //get message
    fgets(message, sizeof(message), stdin);

    // Send a message to the server
    int rc = send_message(socket_fd, message);
    if (rc == -1) {
      perror("Failed to send message to server");
      exit(EXIT_FAILURE);
    } else if (strcmp(message, "quit\n") == 0) {
      break;
    }
  }

  // Close socket
  close(socket_fd);

  return 0;
}
