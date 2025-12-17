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
#include "gamestate.h"
#include "send_messages.h"

int counter = 0;

int bot_num = 0;
gamestate_t *state;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
char action[256];

void* client_thread(void* args) {
  intptr_t client_socket_fd = (intptr_t)args;
  char* current_username;
  int current_user_num;
  for(int i = 0; i < 4; i++) {
    if(client_socket_fd == state -> sockets[i]) {
      current_username = state -> usernames[i];
      current_user_num = i;
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
    
    // check if its users turn
    if (state->current_player != current_user_num){
      char* rebuke = "Not your turn\n";
      send_dm(rebuke, state->sockets[current_user_num]);
      continue;
    }
    // set global action var to message
    strcpy(action, message);
    // signal condition variable 
    pthread_mutex_lock(&lock);
    pthread_cond_signal(&cond);
    
    pthread_mutex_unlock(&lock);
  

    // char final_message[256];
    // strcpy(final_message, current_username);
    // strcat(final_message, ": ");
    // strcat(final_message, message);
    // broadcast(final_message);

    // Free the message string
    free(message);
  }
  close(client_socket_fd);
  return NULL;
}

int main() {
  state = init_game(bot_num);

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
    state -> sockets[i] = -1;
  }

  printf("Server listening on port %u\n", port);

  // adding the users
  while(counter < 4) {
    // Wait for a client to connect
    printf("making new thread\n");
    pthread_t client;

    intptr_t client_socket_fd = server_socket_accept(server_socket_fd);
    state -> sockets[counter] = client_socket_fd;
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
    state -> usernames[counter] = message;
    counter++;

    pthread_create(&client, NULL, client_thread, (void*)client_socket_fd);
  }
  sleep(1);
  run_game(state, &lock, &cond, action);

  close(server_socket_fd);



  return 0;
}
