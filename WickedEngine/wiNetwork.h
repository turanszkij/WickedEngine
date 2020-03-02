#pragma once
#include "CommonInclude.h"

#include <memory>
#include <array>

namespace wiNetwork
{
	struct Socket
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }
	};

	static const uint16_t DEFAULT_PORT = 777;
	struct Connection
	{
		std::array<uint8_t, 4> ipaddress = { 127,0,0,1 };
		uint16_t port = DEFAULT_PORT;
	};

	void Initialize();

	// Creates a socket that can be used to send or receive data
	bool CreateSocket(Socket* sock);

	// Destroys socket
	bool Destroy(Socket* sock);

	// Sends data packet to destination connection
	//	sock		:	socket that sends the packet
	//	connection	:	connection to the receiver, it is provided by the call site
	//	data		:	buffer that contains data to send
	//	dataSize	:	size of the data to send in bytes
	bool Send(const Socket* sock, const Connection* connection, const void* data, size_t dataSize);

	// Enables the socket to receive data on a port
	//	sock		:	socket that receives packet
	//	port		:	port number to open
	bool ListenPort(const Socket* sock, uint16_t port = DEFAULT_PORT);

	// Checks whether any data can be received at the moment, returns immediately
	//	sock		:	socket that receives packet
	//	timeout_microseconds : timeout period in microseconds. Specify this to let the function block
	//	returns true if there are any messages in the queue, false otherwise
	bool CanReceive(const Socket* sock, long timeout_microseconds = 1);

	// Receive data. This function will block until a packet has been received. Use CanReceive() function to check if this function will block or not.
	//	sock		:	socket that receives packet
	//	connection	:	sender's connection data will be written to it when the function returns
	//	data		:	buffer to hold received data, must be already allocated to a sufficient size
	//	dataSize	:	expected data size in bytes
	bool Receive(const Socket* sock, Connection* connection, void* data, size_t dataSize);
}
