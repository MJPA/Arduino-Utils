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
#define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

// Location of the socket
#ifndef SOCKET_PATH
#define SOCKET_PATH "/tmp/arduino.sock"
#endif

// Quit handler
static int want_to_quit = 0;
void sighandler(int sig) {
  want_to_quit = 1;
}

int main (int argc, char ** argv) {
  // Generate the bitmask to send
  unsigned char bitmask = 0x00;
  int i;
  for (i = 1; i < argc; i++) {
    int input = atoi(argv[i]);
    if (input == 0)
      continue;

    bitmask |= (0x01 << (input - 1));
  }

  // No inputs?
  if (bitmask == 0x00) {
    fprintf(stderr, "Usage: %s [input1] [input2] ...\n", argv[0]);
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
    if (ret == -1)
      break;
    printf("%*s", ret, buffer);
  }
  
  return EXIT_SUCCESS;
}
