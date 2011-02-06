/*
 * File arduino.c part of Arduino Utils - http://mjpa.in/arduino
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
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <termios.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <map>
#include <list>

// Location of the socket
#ifndef SOCKET_PATH
#define SOCKET_PATH "/tmp/arduino.sock"
#endif

// Arduino settings, change if you wish
#define ARDUINO "/dev/arduino"
#define FIELD_SEPARATOR '\r'
#define ROW_SEPARATOR '\n'

// The arduino fd
static int arduino = -1;

// Clients map
static std::map<int, std::list<int> > clients;

// Client requests to process at 0 input
typedef struct _client_request
{
	int socket;
	unsigned char bitmask;
} client_request;
static std::list<client_request> client_requests;

// Lock for the client requests
static bool client_lock = false;
static void get_client_lock() { while (client_lock == true); client_lock = true; }
static void release_client_lock() { client_lock = false; }

// Quit handler
static int want_to_quit = 0;
void sighandler(int sig)
{
	want_to_quit = 1;
}

// A thread for accepting clients
void *handle_clients(void *sock_ptr)
{
	int sock = *((int*)sock_ptr);

	// Loop until we quit then...
	while (want_to_quit == 0)
	{
		// Get a new client (or be told there is none)
		int new_client = accept(sock, NULL, NULL);
		if (new_client == -1)
		{
			perror("accept()");
			want_to_quit = 1;
			break;
		}

		// NOTE: This loop can be blocked if the client doesn't send any data so
		// ensure you know what connects here actually sends data!

		// First byte signifies what inputs they want
		unsigned char input_request = 0x00;
		if (recv(new_client, &input_request, 1, MSG_NOSIGNAL) == -1)
			continue;

		// If the request isn't 0 they just want the inputs so parse the request
		if (input_request != 0x00)
		{
			// Set the client socket to be nonblocking so the data doesn't get held up when sending
			fcntl(new_client, F_SETFL, O_NONBLOCK);

			// Add the new client request
			get_client_lock();
				client_request cr;
				cr.socket = new_client;
				cr.bitmask = input_request;
				client_requests.push_back(cr);
			release_client_lock();
		}
		else
		{
			// They're sending data, next byte gives the length
			unsigned char data_length = 0;
			if (recv(new_client, &data_length, 1, MSG_NOSIGNAL) == -1)
				continue;

			// Read the data now
			char *data = (char*)malloc(data_length);
			if (recv(new_client, data, data_length, MSG_NOSIGNAL) == -1)
				continue;

			// Send the data to the arduino
			write(arduino, data, data_length);
			free(data);
		}
	}

	pthread_exit(0);
}

int main (int argc, char ** argv)
{
	// Setup the socket
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
	{
        	perror("socket");
	        return EXIT_FAILURE;
	}

	// Create the struct for listening on the socket
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	// Bind to it
	unlink(SOCKET_PATH);
	if (bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
	{
		perror("bind");
		return EXIT_FAILURE;
	}

	// And finally listen...
	if (listen(sock, 5) == -1)
	{
		perror("listen");
		return EXIT_FAILURE;
	}

	// Allow everyone to communicate
	chmod(SOCKET_PATH, 0777);

	// Try opening the arduino board
	arduino = open(ARDUINO, O_RDWR | O_NOCTTY | O_NDELAY);
	if (arduino == -1)
	{
		fprintf(stderr, "This program requires an arduino board.\n");
		fprintf(stderr, "If you have an arduino board setup and working ensure it is available at " ARDUINO "\n");
		fprintf(stderr, "If it helps, the error when trying to open the arduino was: ");
		perror("");
		return EXIT_FAILURE;
	}

	// Get the options for the arduino
	struct termios toptions;
	if (tcgetattr(arduino, &toptions) < 0)
	{
		fprintf(stderr, "Couldn't get the term attributes, something went wrong here.\n");
		return EXIT_FAILURE;
	}

	// Set the baud rate
	speed_t brate = B9600;
	cfsetispeed(&toptions, brate);
	cfsetospeed(&toptions, brate);

	// Set some options for the arduino now
	toptions.c_cflag &= ~PARENB;
	toptions.c_cflag &= ~CSTOPB;
	toptions.c_cflag &= ~CSIZE;
	toptions.c_cflag |= CS8;
	toptions.c_cflag &= ~CRTSCTS;
	toptions.c_cflag |= CREAD | CLOCAL;
	toptions.c_iflag &= ~(ICANON | ICRNL | IXON | IXOFF | IXANY);
	toptions.c_lflag &= ~(ECHO | ECHOE | ISIG);
	toptions.c_oflag &= ~OPOST;
	toptions.c_cc[VMIN]  = 0;
	toptions.c_cc[VTIME] = 20;
	if (tcsetattr(arduino, TCSANOW, &toptions) < 0)
	{
		fprintf(stderr, "Couldn't set the term attributes, something went wrong here.\n");
		return EXIT_FAILURE;
	}

	// Fork
	switch(fork())
	{
		case -1:
			perror("fork");
			return EXIT_FAILURE;

		case 0:
			break;

		default:
			return EXIT_SUCCESS;
	}

	// Signal handlers
	signal(SIGINT, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGABRT, sighandler);
	signal(SIGKILL, sighandler);
	signal(SIGTERM, sighandler);

	// Sync ourselves with the output - wait for the first ROW_SEPARATOR
	while (want_to_quit == 0)
	{
		char c;
		int ret = read(arduino, &c, 1);
		if (ret == -1 && errno != EAGAIN)
		{
			// Some error reading, don't start the next loop, just quit asap
			perror("read()");
			want_to_quit = 1;
			break;
		}

		// Got it, break.
		if (c == ROW_SEPARATOR) break;
	}

	// We have everything opened and the input is synced and ready to go, spawn client accepting thread
	pthread_t client_thread;
	pthread_create(&client_thread, NULL, handle_clients, &sock);

	int idx = 0, pos = 0;
	char line[512];
	memset(line, 0, sizeof(line));

	// Loop until quit while reading the arduino data
	while (want_to_quit == 0)
	{
		// Start of a fresh input line? Handle new client requests
		if (idx == 0 && pos == 0)
		{
			get_client_lock();
				std::list<client_request>::iterator cr_iter;
				for (cr_iter = client_requests.begin(); cr_iter != client_requests.end(); cr_iter++)
				{
					client_request cr = *cr_iter;
					unsigned char input_request = cr.bitmask;
					int input = 0;
					for (input = 0; input < 8; input++)
					{
						// If the bit is set, add it to the map
						if ((input_request & 0x01) == 0x01)
						{
							(clients[input]).push_back(cr.socket);
						}
	
						// Next bit please
						input_request = input_request >> 1;
					}
				}
				client_requests.clear();
			release_client_lock();
		}

		char c;
		int ret = read(arduino, &c, 1);

		// Error?
		if (ret == -1)
		{
			if (errno == EAGAIN)
			{
				usleep(250000);
				continue;
			}
			else
			{			
				perror("read()");
				want_to_quit = 1;
				break;
			}
		}

		// No more space - bail as its a problem
		if (pos == sizeof(line)) break;

		// Separator or new row - handle what we've got
		if (c == ROW_SEPARATOR || c == FIELD_SEPARATOR)
		{
			// Get the sockets for this input
			std::list<int> *sockets = &clients[idx];
			std::list<int>::iterator iter;

			// List of sockets to erase
			std::list<int> dead_sockets;

			// Add a new line to the data
			strcat(line, "\n");

			// Loop through the sockets and send the data
			for (iter = sockets->begin(); iter != sockets->end(); iter++)
			{
				// Send the data, on error remove the socket
				if (send(*iter, line, strlen(line), MSG_NOSIGNAL) == -1)
					dead_sockets.push_back(*iter);
			}

			// Actually remove the dead sockets...
			for (iter = dead_sockets.begin(); iter != dead_sockets.end(); iter++)
			{
				sockets->remove(*iter);
				close(*iter);
			}

			// Reset pos and move the input index on
			pos = 0;
			idx = c == FIELD_SEPARATOR ? (idx + 1) : 0;
		}
		else
		{
			// Normal data, save it
			line[pos++] = c;
			line[pos] = 0;
		}
	}

	return EXIT_SUCCESS;
}
