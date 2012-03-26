/*
 * File arduino-read.c part of Arduino Utils - http://mjpa.in/arduino
 * 
 * Copyright (C) 2009 Mike Anchor
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

// Apple Mac OS X fixes
#ifdef __APPLE__
#define MSG_NOSIGNAL 0
#endif

// Location of the socket
#ifndef SOCKET_PATH
#define SOCKET_PATH "/tmp/arduino.sock"
#endif

#define ROW_SEPARATOR '\n'

// Quit handler
static int want_to_quit = 0;
void sighandler(int sig) {
  want_to_quit = 1;
}

int main(int argc, char ** argv) {
  // Do they just want a single reading?
  bool test_mode = false;
  if (argc > 1 && strcmp(argv[1], "-t") == 0) {
    test_mode = true;
  }

  // Generate the bitmask to send
  unsigned char bitmask = 0x00;
  int i = test_mode ? 2 : 1;
  for (; i < argc; i++) {
    int input = atoi(argv[i]);
    if (input == 0) {
      continue;
    }

    bitmask |= (0x01 << (input - 1));
  }

  // No inputs?
  if (bitmask == 0x00) {
    fprintf(stderr, "Usage: %s [-t] [input1] [input2] ...\n", argv[0]);
    fprintf(stderr, "Parameters:\n\t-t\tTest mode - only prints 1 reading per input then exists\n");
    return EXIT_FAILURE;
  }

  // Setup the socket
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  // Create the struct for connecting to the socket
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

  // Connect to it
  if (connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1) {
    perror("connect");
    return EXIT_FAILURE;
  }

  // OS X doesn't have MSG_NOSIGNAL so we have to do it this way...
#ifdef __APPLE__
  int set = 1;
  setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif

  // Send the inputs request
  send(sock, &bitmask, 1, MSG_NOSIGNAL);

  // Signal handlers
  signal(SIGINT, sighandler);
  signal(SIGQUIT, sighandler);
  signal(SIGABRT, sighandler);
  signal(SIGKILL, sighandler);
  signal(SIGTERM, sighandler);

  // Loop until asked to quit
  while (want_to_quit == 0) {
    char buffer[16];
    int ret = recv(sock, buffer, sizeof(buffer), MSG_NOSIGNAL);
    if (ret == -1) {
      break;
    }

    // If in test mode, look for the row separator. If it's found, zero out
    // the rest of the buffer then stop
    if (test_mode) {
      char *row_end = strchr(buffer, ROW_SEPARATOR);
      if (row_end != NULL) {
        *row_end = 0;
        printf("%s", buffer);
        break;
      }
    }

    printf("%*s", ret, buffer);
  }
  
  return EXIT_SUCCESS;
}
