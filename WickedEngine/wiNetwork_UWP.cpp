#include "wiPlatform.h"

#if defined(_WIN32) && defined(PLATFORM_UWP)
#include "wiNetwork.h"
#include "wiBacklog.h"

namespace wi::network
{
	bool CreateSocket(Socket* sock)
	{
		return false;
	}
	bool Destroy(Socket* sock)
	{
		return false;
	}

	bool Send(const Socket* sock, const Connection* connection, const void* data, size_t dataSize)
	{
		return false;
	}

	bool ListenPort(const Socket* sock, uint16_t port)
	{
		return false;
	}

	bool CanReceive(const Socket* sock, long timeout_microseconds)
	{
		return false;
	}

	bool Receive(const Socket* sock, Connection* connection, void* data, size_t dataSize)
	{
		return false;
	}

}

#endif // _WIN32 && PLATFORM_UWP
