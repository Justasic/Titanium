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

#ifndef HAVE_POLL
# error "You probably shouldn't be trying to compile an poll multiplexer on a poll-unsupported platform. Try again."
#endif

#include <sys/poll.h>
#include <sys/time.h>
#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <vector>

#include "multiplexer.h"
#include "misc.h"
#include "Config.h"

#ifndef _WIN32
# include <unistd.h>
#endif

typedef struct pollfd poll_t;
static std::vector<poll_t> events;

int AddToMultiplexer(Socket *s)
{
	poll_t ev;
	memset(&ev, 0, sizeof(poll_t));

	ev.fd = s->GetFD();
	ev.events = POLLIN;

	s->flags = SF_READABLE;

	events.push_back(ev);

	return 0;
}

int RemoveFromMultiplexer(Socket *s)
{
	for (std::vector<poll_t>::iterator it = events.begin(), it_end = events.end(); it != it_end; ++it)
	{
		if ((*it).fd == s->GetFD())
		{
			events.erase(it);
			return 0;
		}
	}

	return -1;
}

int SetSocketStatus(Socket *s, int status)
{
	for (auto it : events)
	{
		if (it.fd == s->GetFD())
		{
			poll_t *ev = &it;
			ev->events = (status & SF_READABLE ? POLLIN : 0) | (status & SF_WRITABLE ? POLLOUT : 0);
			s->flags = status;
			return 0;
		}
	}

	return -1;
}

int InitializeMultiplexer(void)
{
	// We have no vectors to initialize in C++.
	return 0;
}

int ShutdownMultiplexer(void)
{
	// Again, nothing to do.
	return 0;
}

void ProcessSockets(void)
{
	dprintf("Entering poll\n");

	int total = poll(events.data(), events.size(), c->readtimeout * 1000);

	if (total < 0)
	{
		if (errno != EINTR)
			fprintf(stderr, "Error processing sockets: %s\n", strerror(errno));
		return;
	}

	for (int i = 0, processed = 0; i < events.size() && processed != total; ++i)
	{
		poll_t *ev = &events[i];

		if (ev->revents != 0)
			processed++;
		else // Nothing to do, move on.
			continue;

		Socket *s = Socket::FindSocket(ev->fd);
		if (!s)
		{
			dfprintf(stderr, "Unknown FD in multiplexer: %d\n", ev->fd);
			// We don't know what socket this is. Someone added something
			// stupid somewhere so shut this shit down now.
			// We have to create a temporary socket_t object to remove it
			// from the multiplexer, then we can close it.
			Socket(ev->fd, socket_t());
			continue;
		}

		if (ev->revents & (POLLERR | POLLRDHUP))
		{
			dprintf("poll error reading socket %d, destroying.\n", s->GetFD());
			delete s;
			continue;
		}

		if (ev->revents & POLLIN && s->ReceiveData() == -1)
		{
			dprintf("Destorying socket due to receive failure!\n");
			delete s;
			continue;
		}

		if (ev->revents & POLLOUT && s->SendData() == -1)
		{
			dprintf("Destorying socket due to send failure!\n");
			delete s;
		}
	}
}
