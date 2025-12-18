//Citation: Server Exercise in class
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
//initialize lock and condition variable
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
char action[256];
/**
 * Creates a client thread
 * @return NULL
 */
void* client_thread(void* args) {
  //gets socket from args
  intptr_t client_socket_fd = (intptr_t)args;
  char* current_username;
  int current_user_num;
  //finds username
  for(int i = 0; i < 4; i++) {
    if(client_socket_fd == state -> sockets[i]) {
      current_username = state -> usernames[i];
      current_user_num = i;
    }
  }
  //tells which player connected
  printf("%s connected!\n", current_username);

  while(true) {
    // Read a message from the client
    char* message = receive_message(client_socket_fd);
    if (message == NULL) {
      perror("Failed to read message from client");
      exit(EXIT_FAILURE);
    } else if (strcmp(message, "quit\n") == 0) {
      broadcast("A player quit. The game has ended", state -> sockets);
      break;
    }
    
    // check if its users turn and sends message if it isn't
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

    // Free the message string
    free(message);
  }
  //close client socket
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
  //set sockets to -1
  for (int i = 0; i < 4; i++){
    state -> sockets[i] = -1;
  }
  //init print
  printf("Server listening on port %u\n", port);

  // adding the users
  while(counter < 4) {
    // Wait for a client to connect

    //create client
    pthread_t client;

    intptr_t client_socket_fd = server_socket_accept(server_socket_fd);
    //update sockets
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
    //create client thread
    pthread_create(&client, NULL, client_thread, (void*)client_socket_fd);
  }
  //sleep for a second after everyone has joined before running game
  sleep(1);
  //start the game
  run_game(state, &lock, &cond, action);
  //close server
  close(server_socket_fd);



  return 0;
}
