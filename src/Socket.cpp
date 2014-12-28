#include <vector>
#include <cstdio>
#include <cstdint>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#include "Socket.h"
#include "multiplexer.h"
#include "tinyformat.h"
#include "misc.h"

std::vector<Socket*> Socket::sockets;

Socket::Socket(int fd, socket_t addr) : fd(fd)
{
	// Copy the socket address union
	memcpy(&this->addr, &addr, sizeof(socket_t));
	// Add to socket array
	sockets.push_back(this);
	// add to multiplexer so we can start being processed.
	AddToMultiplexer(this);
}

Socket::~Socket()
{
	// remove ourselves.
	for (std::vector<Socket*>::iterator it = sockets.begin(), it_end = sockets.end(); it != it_end; ++it)
		if ((*it)->fd == this->fd)
			sockets.erase(it);

	RemoveFromMultiplexer(this);

	// Close the file descriptor
	close(this->fd);
}


void Socket::QueueData(socket_t client, void *data, size_t len)
{
	// Queue the data
	this->dataqueue.push_back(std::make_tuple(client, data, len));
	// Inform the multiplexer we're ready to write.
	SetSocketStatus(this, SF_READABLE | SF_WRITABLE);
}

// These functions should not be called outside the multiplexing system
int Socket::ReceiveData()
{
	dprintf("Weee! Multiplexer says we received some data!\n");

	typedef struct information_s {
		struct sysinfo s;
		struct utsname u;
		time_t timestamp;
	} information_t;


	socket_t ss;
	socklen_t addrlen = sizeof(ss);
	uint8_t buf[512];
	errno = 0;
	memset(buf, 0, sizeof(buf));
	size_t recvlen = recvfrom(this->fd, buf, 512, 0, &ss.sa, &addrlen);
	// The kernel either told us that we need to read again
	// or we received a signal and are continuing from where
	// we left off.
	if (recvlen == -1 && (errno == EAGAIN || errno == EINTR))
		return 0;
	else if (recvlen == -1)
	{
		fprintf(stderr, "Socket: Received an error when reading from the socket: %s\n", strerror(errno));
		return -1;
	}

	dprintf("Received %lu bytes\n", recvlen);

	information_t i;
	information_t *info = &i;
	memcpy(info, buf, 516);


	printf("%ld seconds uptime\n", info->timestamp - info->s.uptime);
	printf("%lu %lu %lu load times\n", info->s.loads[0], info->s.loads[1], info->s.loads[2]);
	printf("%d processes running\n", info->s.procs);

	printf("Hostname: %s\n", info->u.nodename);
	printf("Architecture: %s\n", info->u.machine);
	printf("OS: %s %s\n", info->u.sysname, info->u.release);

	// Tell the multiplexer we're done.
	SetSocketStatus(this, SF_READABLE);

	return 0;
}

int Socket::SendData()
{
	dprintf("Weeee! Multiplexer is ready to write some data!\n");
	return 0;
}

// This function will create and bind to a port and address. It will create and add
// the socket to the epoll loop. This should be called for each socket created by
// the config.
Socket *Socket::CreateSocket(const std::string &str, unsigned short port)
{
	int fd = 0;
	socket_t saddr;
	memset(&saddr, 0, sizeof(socket_t));
	// Get the address fanily
	saddr.sa.sa_family = str.find(":") != std::string::npos ? AF_INET6 : AF_INET;
	dprintf("Address family is: %s\n", saddr.sa.sa_family == AF_INET6 ? "AF_INET6" : "AF_INET");
	// Set our port
	*(saddr.sa.sa_family == AF_INET ? &saddr.in.sin_port : &saddr.in6.sin6_port) = htons(port);
	// Stupid C++ pointer types.
	switch (inet_pton(saddr.sa.sa_family, str.c_str(), (saddr.sa.sa_family == AF_INET ? reinterpret_cast<void*>(&saddr.in.sin_addr) : reinterpret_cast<void*>(&saddr.in6.sin6_addr))))
	{
		case 1: // Success.
			break;
		case 0:
			tfm::fprintf(stderr, "Invalid %s bind address: %s\n",
				saddr.sa.sa_family == AF_INET ? "IPv4" : "IPv6", str);
		default:
			perror("inet_pton");
			return nullptr;
	}
	// Create the UDP listening socket
	if ((fd = socket(saddr.sa.sa_family, SOCK_DGRAM, 0)) < 0)
	{
		perror("Cannot create socket");
		return nullptr;
	}

	// Set it as non-blocking
	int flags = fcntl(fd, F_GETFL, 0);
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		perror("fcntl O_NONBLOCK");
		close(fd);
		return nullptr;
	}

	// Bind to the socket
	if (bind(fd, &saddr.sa, saddr.sa.sa_family == AF_INET ? sizeof(saddr.in) : sizeof(saddr.in6)) < 0)
	{
		perror("bind failed");
		close(fd);
		return nullptr;
	}

	Socket *s = new Socket(fd, saddr);

	// Return success
	return s;
}

Socket *Socket::FindSocket(int fd)
{
	for (auto it : sockets)
		if (it->GetFD() == fd)
			return it;

	return nullptr;
}
