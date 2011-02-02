/*
 * arduino-read.c
 * Copyright (C) 2009 Mike Anchor <mikea@mjpa.co.uk>
 *
 * For details visit: http://mjpa.co.uk/blog/view/Arduino-Utils/
 *
 * License: http://mjpa.co.uk/licenses/GPL/2/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

// Location of the socket
#ifndef SOCKET_PATH
#define SOCKET_PATH "/tmp/arduino.sock"
#endif

// Quit handler
static int want_to_quit = 0;
void sighandler(int sig)
{
	want_to_quit = 1;
}

int main (int argc, char ** argv)
{
	// Generate the bitmask to send
	unsigned char bitmask = 0x00;
	int i;
	for (i = 1; i < argc; i++)
	{
		int input = atoi(argv[i]);
		if (input == 0)
			continue;

		bitmask |= (0x01 << (input - 1));
	}

	// No inputs?
	if (bitmask == 0x00)
	{
		fprintf(stderr, "Usage: %s [input1] [input2] ...\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Setup the socket
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
	{
        	perror("socket");
	        return EXIT_FAILURE;
	}

	// Create the struct for connecting to the socket
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	// Connect to it
	if (connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
	{
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
	while (want_to_quit == 0)
	{
		char buffer[16];
		int ret = recv(sock, buffer, sizeof(buffer), MSG_NOSIGNAL);
		if (ret == -1)
			break;
		printf("%*s", ret, buffer);
	}

	
	return EXIT_SUCCESS;
}
