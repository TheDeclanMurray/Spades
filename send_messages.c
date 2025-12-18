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

/**
 * Sends a direct message to a single player
 * @return booleam
 */
bool send_dm(char* message, intptr_t socket){
    //checks if legal socket
    if (socket != -1){
      //sends message to a socket
      int rc = send_message(socket, message);
      if (rc == -1) {
        perror("Failed to send message to client");
        exit(EXIT_FAILURE);
      }
      return true;
    }
  return false;
}

/**
 * Sends a message to all connected clients
 * @return boolean
 */
bool broadcast(char* message, intptr_t* sockets){
  //iterates through all clients and sends the message to each one
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