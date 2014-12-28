#pragma once
#include <cstdint>
#include <vector>
#include <tuple>
#include <arpa/inet.h>

// A union to define the various socket structures
// which are always casted among themselves (which is retarded)
typedef union {
	struct sockaddr_in in;
	struct sockaddr_in6 in6;
	struct sockaddr sa;
} socket_t;


class Socket
{
	int fd;
	socket_t addr;
	std::vector<std::tuple<socket_t, void *, size_t>> dataqueue;
public:
	Socket(int fd, socket_t addr);
	~Socket();

	// flags on this socket
	int flags;

	inline const int GetFD() const { return this->fd; }

	// Data to be sent
	void QueueData(socket_t, void *data, size_t len);

	/// These functions should not be called outside the multiplexing system
	// Receive data from the socket when called
	int ReceiveData();
	// Send data out of the socket when called.
	int SendData();


	// Static member functions because we don't have a proper
	// socket management class (since we're not really managing thousands of sockets)
	static std::vector<Socket*> sockets;
	static Socket *FindSocket(int fd);
	static Socket *CreateSocket(const std::string &str, unsigned short port);

};


extern Socket *FindSocket(int fd);
// All the sockets in the application which are not managed by FastCGI
extern std::vector<Socket*> sockets;
