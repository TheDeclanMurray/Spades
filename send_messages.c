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

bool send_dm(char* message, intptr_t socket){
    if (socket != -1){
      int rc = send_message(socket, message);
      if (rc == -1) {
        perror("Failed to send message to client");
        exit(EXIT_FAILURE);
      }
      return true;
    }
  return false;
}

bool broadcast(char* message, intptr_t* sockets){
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