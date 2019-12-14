#ifndef WINSTORE_SUPPORT
#include "wiNetwork.h"
#include "wiBackLog.h"

#include <sstream>

#include <winsock.h>
#pragma comment(lib,"ws2_32.lib")

namespace wiNetwork
{
	Socket::~Socket()
	{
		Destroy(this);
	}

	void Initialize()
	{
		int result;
		WSAData wsadata;
		result = WSAStartup(MAKEWORD(2, 2), &wsadata); 
		if (result)
		{
			int error = WSAGetLastError();
			std::stringstream ss("");
			ss << "wiNetwork Initialization FAILED with error: " << error;
			wiBackLog::post(ss.str().c_str());
			WSACleanup();
			assert(0);
		}

		wiBackLog::post("wiNetwork Initialized");
	}

	bool CreateSocket(Socket* sock)
	{
		Destroy(sock);

		SOCKET handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (handle == INVALID_SOCKET)
		{
			int error = WSAGetLastError();
			std::stringstream ss;
			ss << "wiNetwork error in CreateSocket: " << error;
			wiBackLog::post(ss.str().c_str());
			return false;
		}

		sock->handle = (wiCPUHandle)handle;

		return true;
	}
	bool Destroy(Socket* sock)
	{
		if (socket != nullptr && sock->handle != WI_NULL_HANDLE)
		{
			int result = closesocket((SOCKET)sock->handle);
			if (result == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				std::stringstream ss;
				ss << "wiNetwork error in Destroy: " << error;
				wiBackLog::post(ss.str().c_str());
				return false;
			}

			sock->handle = WI_NULL_HANDLE;
			return true;
		}
		return false;
	}

	bool Send(const Socket* sock, const Connection* connection, const void* data, size_t dataSize)
	{
		if (socket != nullptr && sock->handle != WI_NULL_HANDLE)
		{
			sockaddr_in target;
			target.sin_family = AF_INET;
			target.sin_port = htons(connection->port); // reverse byte order from host to network
			target.sin_addr.S_un.S_un_b.s_b1 = connection->ipaddress[0];
			target.sin_addr.S_un.S_un_b.s_b2 = connection->ipaddress[1];
			target.sin_addr.S_un.S_un_b.s_b3 = connection->ipaddress[2];
			target.sin_addr.S_un.S_un_b.s_b4 = connection->ipaddress[3];

			int result = sendto((SOCKET)sock->handle, (const char*)data, (int)dataSize, 0, (const sockaddr*)& target, sizeof(target));
			if (result == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				std::stringstream ss;
				ss << "wiNetwork error in Send: " << error;
				wiBackLog::post(ss.str().c_str());
				return false;
			}

			return true;
		}
		return false;
	}

	bool ListenPort(const Socket* sock, uint16_t port)
	{
		if (socket != nullptr && sock->handle != WI_NULL_HANDLE)
		{
			sockaddr_in target;
			target.sin_family = AF_INET;
			target.sin_port = htons(port);
			target.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

			int result = bind((SOCKET)sock->handle, (const sockaddr*)& target, sizeof(target));
			if (result == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				std::stringstream ss;
				ss << "wiNetwork error in ListenPort: " << error;
				wiBackLog::post(ss.str().c_str());
				return false;
			}

			return true;
		}
		return false;
	}

	bool CanReceive(const Socket* sock, long timeout_microseconds)
	{
		if (socket != nullptr && sock->handle != WI_NULL_HANDLE)
		{
			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET((SOCKET)sock->handle, &readfds);
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = timeout_microseconds;
			int result = select(0, &readfds, NULL, NULL, &timeout);
			if (result < 0)
			{
				std::stringstream ss;
				ss << "wiNetwork error in CanReceive: " << WSAGetLastError();
				wiBackLog::post(ss.str().c_str());
				assert(0);
				return false;
			}

			return FD_ISSET((SOCKET)sock->handle, &readfds);
		}
		return false;
	}

	bool Receive(const Socket* sock, Connection* connection, void* data, size_t dataSize)
	{
		if (socket != nullptr && sock->handle != WI_NULL_HANDLE)
		{
			sockaddr_in sender;
			int targetsize = sizeof(sender);
			int result = recvfrom((SOCKET)sock->handle, (char*)data, (int)dataSize, 0, (sockaddr*)& sender, &targetsize);
			if (result == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				std::stringstream ss;
				ss << "wiNetwork error in Receive: " << error;
				wiBackLog::post(ss.str().c_str());
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

#endif // WINSTORE_SUPPORT
