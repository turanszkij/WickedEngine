#include "wiPlatform.h"

#ifdef PLATFORM_LINUX
#include "wiNetwork.h"
#include "wiBackLog.h"

namespace wiNetwork
{
	void Initialize()
	{
		wiBackLog::post("TODO wiNetwork_Linux");
	}

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

#endif // LINUX
