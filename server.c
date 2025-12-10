#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <pthread.h>
#include "message.h"
#include "socket.h"

intptr_t sockets[4];
char* usernames[4];
int counter = 0;

bool send_dm(char* username, char* message){
  for (int i = 0; i < 4; i++){
    if (sockets[i] != -1 && strcmp(usernames[i], username) == 0){
      int rc = send_message(sockets[i], message);
      if (rc == -1) {
        perror("Failed to send message to client");
        exit(EXIT_FAILURE);
      }
      return true;
    }
  }
  return false;
}

bool broadcast(char* message){
  for( int i = 0; i < 4; i++){
    if (sockets[i] == -1){
      return false;
    }
    int rc = send_message(sockets[i], message);
    if (rc == -1) {
      perror("Failed to send message to client");
      exit(EXIT_FAILURE);
    }
  }
  return true;
}

void* client_thread(void* args) {
  intptr_t client_socket_fd = (intptr_t)args;
  char* current_username;
  for(int i = 0; i < 4; i++) {
    if(client_socket_fd == sockets[i]) {
      current_username = usernames[i];
    }
  }
  printf("%s connected!\n", current_username);

  while(true) {
    // Read a message from the client
    char* message = receive_message(client_socket_fd);
    if (message == NULL) {
      perror("Failed to read message from client");
      exit(EXIT_FAILURE);
    } else if (strcmp(message, "quit\n") == 0) {
      // TODO: send message "player i ragequit"
      break;
    }

    // Print the message
    printf("%s sent: %s\n", current_username, message);

    char final_message[256];
    strcpy(final_message, current_username);
    strcat(final_message, ": ");
    strcat(final_message, message);
    broadcast(final_message);

    // Free the message string
    free(message);
  }
  close(client_socket_fd);
  return NULL;
}

int main() {
  // Open a server socket
  unsigned short port = 0;
  int server_socket_fd = server_socket_open(&port);
  if (server_socket_fd == -1) {
    perror("Server socket was not opened");
    exit(EXIT_FAILURE);
  }

  // Start listening for connections, with a maximum of one queued connection
  if (listen(server_socket_fd, 1)) {
    perror("listen failed");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < 4; i++){
    sockets[i] = -1;
  }

  printf("Server listening on port %u\n", port);
  while(true) {
    // Wait for a client to connect
    printf("making new thread\n");
    pthread_t client;

    intptr_t client_socket_fd = server_socket_accept(server_socket_fd);
    sockets[counter] = client_socket_fd;
    if (client_socket_fd == -1) {
      perror("accept failed");
      exit(EXIT_FAILURE);
    }

    // grabs the username from client
    char* message = receive_message(client_socket_fd);
    if (message == NULL) {
      perror("Failed to read message from client");
      exit(EXIT_FAILURE);
    } 
    usernames[counter] = message;
    counter++;


    pthread_create(&client, NULL, client_thread, (void*)client_socket_fd);
  }
  close(server_socket_fd);

  return 0;
}
