/*
 * File arduino-send.c part of Arduino Utils - http://mjpa.in/arduino
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
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
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

int main(int argc, char ** argv) {
  // Ensure we have something to send
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <data>\n", argv[0]);
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

  // Send that we don't want any inputs
  send(sock, "\0", 1, MSG_NOSIGNAL);

  // Send how long the data is
  unsigned char data_length = strlen(argv[1]);
  send(sock, &data_length, 1, MSG_NOSIGNAL);

  // And finally send the data
  send(sock, argv[1], data_length, MSG_NOSIGNAL);

  return EXIT_SUCCESS;
}
