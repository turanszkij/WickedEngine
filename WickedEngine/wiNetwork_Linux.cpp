#include "wiPlatform.h"

#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
#include "wiNetwork.h"
#include "wiBacklog.h"
#include "wiTimer.h"

#include <string>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>

namespace wi::network
{
	//For easy address conversion
	struct in_addr_union {
		union {
			struct { uint8_t s_b1,s_b2,s_b3,s_b4; } S_un_b;
			struct { uint16_t s_w1,s_w2; } S_un_w;
			uint32_t S_addr;
		};
	};

	struct SocketInternal{
		int handle;
		~SocketInternal(){
			int result = close(handle);
			if(result < 0){
				assert(0);
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
		sock->internal_state = socketinternal;

		socketinternal->handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if(socketinternal->handle == -1)
		{
			wi::backlog::post("wi::network_Linux error in CreateSocket: Could not create socket");
			return false;
		}

		return true;
	}

	bool Send(const Socket* sock, const Connection* connection, const void* data, size_t dataSize)
	{
		if (sock->IsValid()){
			sockaddr_in target;
			target.sin_family = AF_INET;
			target.sin_port = htons(connection->port);
			in_addr_union address;
			address.S_un_b.s_b1 = connection->ipaddress[0];
			address.S_un_b.s_b2 = connection->ipaddress[1];
			address.S_un_b.s_b3 = connection->ipaddress[2];
			address.S_un_b.s_b4 = connection->ipaddress[3];
			target.sin_addr.s_addr = address.S_addr;

			auto socketinternal = to_internal(sock);
			int result = sendto(socketinternal->handle, (const char*)data, (int)dataSize, 0, (const sockaddr*)&target, sizeof(target));
			if (result < 0)
			{
				wi::backlog::post("wi::network_Linux error in Send: (Error Code: " + std::to_string(result) + ") " + std::string(strerror(result)));
				return false;
			}

			return true;
		}
		return false;
	}

	bool ListenPort(const Socket* sock, uint16_t port)
	{
		if (sock->IsValid()){
			sockaddr_in target;
			target.sin_family = AF_INET;
			target.sin_port = htons(port);
			target.sin_addr.s_addr = htonl(INADDR_ANY);

			auto socketinternal = to_internal(sock);

			int result = bind(socketinternal->handle, (struct sockaddr *)&target , sizeof(target));
			if (result < 0)
			{
				wi::backlog::post("wi::network_Linux error in Send: (Error Code: " + std::to_string(result) + ") " + std::string(strerror(result)));
				return false;
			}

			return true;
		}
		return false;
	}

	bool CanReceive(const Socket* sock, long timeout_microseconds)
	{
		if (sock->IsValid()){
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
				wi::backlog::post("wi::network_Linux error in Send: (Error Code: " + std::to_string(result) + ") " + std::string(strerror(result)));
				assert(0);
				return false;
			}

			return FD_ISSET(socketinternal->handle, &readfds);
		}
		return false;
	}

	bool Receive(const Socket* sock, Connection* connection, void* data, size_t dataSize)
	{
		if (sock->IsValid()){
			auto socketinternal = to_internal(sock);

			sockaddr_in sender;
			int targetsize = sizeof(sender);
			int result = recvfrom(socketinternal->handle, (char*)data, (int)dataSize, 0, (sockaddr*)& sender, (socklen_t*)&targetsize);
			if (result < 0)
			{
				wi::backlog::post("wi::network_Linux error in Send: (Error Code: " + std::to_string(result) + ") " + std::string(strerror(result)));
				assert(0);
				return false;
			}

			connection->port = htons(sender.sin_port); // reverse byte order from network to host
			in_addr_union address;
			address.S_addr = sender.sin_addr.s_addr;
			connection->ipaddress[0] = address.S_un_b.s_b1;
			connection->ipaddress[1] = address.S_un_b.s_b2;
			connection->ipaddress[2] = address.S_un_b.s_b3;
			connection->ipaddress[3] = address.S_un_b.s_b4;

			return true;
		}
		return false;
	}
}

#endif // LINUX
