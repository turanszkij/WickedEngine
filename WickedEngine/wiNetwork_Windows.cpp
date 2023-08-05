#include "wiPlatform.h"

#ifdef PLATFORM_WINDOWS_DESKTOP
#include "wiNetwork.h"
#include "wiBacklog.h"
#include "wiTimer.h"

#include <string>

#include <winsock.h>
#pragma comment(lib,"ws2_32.lib")

namespace wi::network
{
	struct NetworkInternal
	{
		WSAData wsadata;

		NetworkInternal()
		{
			wi::Timer timer;

			int result;

			result = WSAStartup(MAKEWORD(2, 2), &wsadata);
			if (result)
			{
				int error = WSAGetLastError();
				wi::backlog::post("wi::network Initialization FAILED with error: " + std::to_string(error));
				assert(0);
			}

			wi::backlog::post("wi::network Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
		}
		~NetworkInternal()
		{
			WSACleanup();
		}
	};
	inline std::shared_ptr<NetworkInternal>& network_internal()
	{
		static std::shared_ptr<NetworkInternal> internal_state = std::make_shared<NetworkInternal>();
		return internal_state;
	}

	struct SocketInternal
	{
		std::shared_ptr<NetworkInternal> networkinternal;
		SOCKET handle = NULL;

		~SocketInternal()
		{
			int result = closesocket(handle);
			if (result == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				assert(0 && error);
			}
		}
	};
	SocketInternal* to_internal(const Socket* param)
	{
		return static_cast<SocketInternal*>(param->internal_state.get());
	}

	bool CreateSocket(Socket* sock)
	{
		std::shared_ptr<SocketInternal> socketinternal = std::make_shared<SocketInternal>();
		socketinternal->networkinternal = network_internal();
		sock->internal_state = socketinternal;

		socketinternal->handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (socketinternal->handle == INVALID_SOCKET)
		{
			int error = WSAGetLastError();
			wi::backlog::post("wi::network error in CreateSocket: " + std::to_string(error));
			return false;
		}

		return true;
	}

	bool Send(const Socket* sock, const Connection* connection, const void* data, size_t dataSize)
	{
		if (socket != nullptr && sock->IsValid())
		{
			sockaddr_in target;
			target.sin_family = AF_INET;
			target.sin_port = htons(connection->port); // reverse byte order from host to network
			target.sin_addr.S_un.S_un_b.s_b1 = connection->ipaddress[0];
			target.sin_addr.S_un.S_un_b.s_b2 = connection->ipaddress[1];
			target.sin_addr.S_un.S_un_b.s_b3 = connection->ipaddress[2];
			target.sin_addr.S_un.S_un_b.s_b4 = connection->ipaddress[3];

			auto socketinternal = to_internal(sock);

			int result = sendto(socketinternal->handle, (const char*)data, (int)dataSize, 0, (const sockaddr*)& target, sizeof(target));
			if (result == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				wi::backlog::post("wi::network error in Send: " + std::to_string(error));
				return false;
			}

			return true;
		}
		return false;
	}

	bool ListenPort(const Socket* sock, uint16_t port)
	{
		if (socket != nullptr && sock->IsValid())
		{
			sockaddr_in target;
			target.sin_family = AF_INET;
			target.sin_port = htons(port);
			target.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

			auto socketinternal = to_internal(sock);

			int result = bind(socketinternal->handle, (const sockaddr*)& target, sizeof(target));
			if (result == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				wi::backlog::post("wi::network error in ListenPort: " + std::to_string(error));
				return false;
			}

			return true;
		}
		return false;
	}

	bool CanReceive(const Socket* sock, long timeout_microseconds)
	{
		if (socket != nullptr && sock->IsValid())
		{
			auto socketinternal = to_internal(sock);

			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(socketinternal->handle, &readfds);
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = timeout_microseconds;
			int result = select(0, &readfds, NULL, NULL, &timeout);
			if (result < 0)
			{
				int error = WSAGetLastError();
				wi::backlog::post("wi::network error in CanReceive: " + std::to_string(error));
				assert(0);
				return false;
			}

			return FD_ISSET(socketinternal->handle, &readfds);
		}
		return false;
	}

	bool Receive(const Socket* sock, Connection* connection, void* data, size_t dataSize)
	{
		if (socket != nullptr && sock->IsValid())
		{
			auto socketinternal = to_internal(sock);

			sockaddr_in sender;
			int targetsize = sizeof(sender);
			int result = recvfrom(socketinternal->handle, (char*)data, (int)dataSize, 0, (sockaddr*)& sender, &targetsize);
			if (result == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				wi::backlog::post("wi::network error in Receive: " + std::to_string(error));
				return false;
			}

			connection->port = htons(sender.sin_port); // reverse byte order from network to host
			connection->ipaddress[0] = sender.sin_addr.S_un.S_un_b.s_b1;
			connection->ipaddress[1] = sender.sin_addr.S_un.S_un_b.s_b2;
			connection->ipaddress[2] = sender.sin_addr.S_un.S_un_b.s_b3;
			connection->ipaddress[3] = sender.sin_addr.S_un.S_un_b.s_b4;

			return true;
		}
		return false;
	}

}

#endif // PLATFORM_WINDOWS_DESKTOP
