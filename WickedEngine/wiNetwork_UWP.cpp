#ifdef WINSTORE_SUPPORT
#include "wiNetwork.h"
#include "wiBackLog.h"

namespace wiNetwork
{
	Socket::~Socket()
	{
	}

	void Initialize()
	{
		wiBackLog::post("TODO wiNetwork_UWP");
	}
	void CleanUp()
	{
	}

	HRESULT CreateSocket(Socket* sock)
	{
		return E_FAIL;
	}
	HRESULT Destroy(Socket* sock)
	{
		return E_FAIL;
	}

	HRESULT Send(const Socket* sock, const Connection* connection, const void* data, size_t dataSize)
	{
		return E_FAIL;
	}

	HRESULT ListenPort(const Socket* sock, uint16_t port)
	{
		return E_FAIL;
	}

	bool CanReceive(const Socket* sock, long timeout_microseconds)
	{
		return false;
	}

	HRESULT Receive(const Socket* sock, Connection* connection, void* data, size_t dataSize)
	{
		return E_FAIL;
	}

}

#endif // WINSTORE_SUPPORT
