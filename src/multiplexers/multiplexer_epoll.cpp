/*
 * Copyright (c) 2014, Justin Crawford <Justasic@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright notice
 * and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "sysconf.h"

#ifndef HAVE_SYS_EPOLL_H
# error "You probably shouldn't be trying to compile an epoll multiplexer on a epoll-unsupported platform. Try again."
#endif

#include "multiplexer.h"
#include "misc.h"
#include "Config.h"
#include "Socket.h"

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

typedef struct epoll_event epoll_t;
// Static because it never leaves this file.
static int EpollHandle = -1;
static std::vector<epoll_t> events;

int AddToMultiplexer(Socket *s)
{
	epoll_t ev;
	memset(&ev, 0, sizeof(epoll_t));

	ev.events = EPOLLIN;
	ev.data.fd = s->GetFD();
	s->flags = SF_READABLE;

	if (epoll_ctl(EpollHandle, EPOLL_CTL_ADD, s->GetFD(), &ev) == -1)
	{
		fprintf(stderr, "Unable to add fd %d from epoll: %s\n", s->GetFD(), strerror(errno));
		return -1;
	}

	return 0;
}

int RemoveFromMultiplexer(Socket *s)
{
	epoll_t ev;
	memset(&ev, 0, sizeof(epoll_t));
	ev.data.fd = s->GetFD();

	if (epoll_ctl(EpollHandle, EPOLL_CTL_DEL, ev.data.fd, &ev) == -1)
	{
		fprintf(stderr, "Unable to remove fd %d from epoll: %s\n", s->GetFD(), strerror(errno));
		return -1;
	}

	return 0;
}

int SetSocketStatus(Socket *s, int status)
{
	epoll_t ev;
	memset(&ev, 0, sizeof(epoll_t));

	ev.events = (status & SF_READABLE ? EPOLLIN : 0) | (status & SF_WRITABLE ? EPOLLOUT : 0);
	ev.data.fd = s->GetFD();
	s->flags = status;

	if (epoll_ctl(EpollHandle, EPOLL_CTL_MOD, ev.data.fd, &ev) == -1)
	{
		fprintf(stderr, "Unable to set fd %d from epoll: %s\n", s->GetFD(), strerror(errno));
		return -1;
	}
	return 0;
}

int InitializeMultiplexer(void)
{
	EpollHandle = epoll_create(4);

	if (EpollHandle == -1)
		return -1;

	// Default of 5 events, it will expand if we get more sockets
	events.reserve(5);

	if (errno == ENOMEM)
		return -1;

	return 0;
}

int ShutdownMultiplexer(void)
{
	// Close the EPoll handle.
	close(EpollHandle);

	return 0;
}

void ProcessSockets(void)
{
	if (Socket::sockets.size() >= events.capacity())
	{
		printf("Reserving more space for events\n");
		events.reserve(Socket::sockets.size() * 2);
	}

	printf("Entering epoll_wait\n");

	int total = epoll_wait(EpollHandle, events.data(), events.capacity(), c->readtimeout * 1000);

	if (total == -1)
	{
		if (errno != EINTR)
			fprintf(stderr, "Error processing sockets: %s\n", strerror(errno));
		return;
	}

	for (int i = 0; i < total; ++i)
	{
		epoll_t *ev = &events[i];

		Socket *s = Socket::FindSocket(ev->data.fd);
		if (!s)
		{
			fprintf(stderr, "Unknown FD in multiplexer: %d\n", ev->data.fd);
			// We don't know what socket this is. Someone added something
			// stupid somewhere so shut this shit down now.
			// We have to create a temporary Socket object to remove it
			// from the multiplexer, then we can close it.
			Socket(ev->data.fd, socket_t());
			continue;
		}

		if (ev->events & (EPOLLHUP | EPOLLERR))
		{
			printf("Epoll error reading socket %d, destroying.\n", s->GetFD());
			delete s;
			continue;
		}

		// process socket read events.
		if (ev->events & EPOLLIN && s->ReceiveData() == -1)
		{
			printf("Destorying socket due to receive failure!\n");
			delete s;
		}

		// Process socket write events
		if (ev->events & EPOLLOUT && s->SendData() == -1)
		{
			printf("Destorying socket due to send failure!\n");
			delete s;
		}
	}
}
