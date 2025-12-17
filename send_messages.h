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

bool send_dm(char* message, intptr_t socket);

bool broadcast(char* message, intptr_t* sockets);