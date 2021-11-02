#include "wiPlatform.h"
#include <cstring>

#ifdef PLATFORM_LINUX
#include "wiNetwork.h"
#include "wiBackLog.h"
#include "wiTimer.h"

#include <sstream>
#include <string>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace wiNetwork
{
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

	void Initialize()
	{
		wiTimer timer;

		int result;

		wiBackLog::post("wiNetwork_Linux Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}

	bool CreateSocket(Socket* sock)
	{
		std::shared_ptr<SocketInternal> socketinternal = std::make_shared<SocketInternal>();
		sock->internal_state = socketinternal;

		socketinternal->handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if(socketinternal->handle == -1){
			std::stringstream ss;
			ss << "wiNetwork_Linux error in CreateSocket: Could not create socket";
			wiBackLog::post(ss.str().c_str());
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
			std::stringstream ipss;
			ipss << std::to_string(connection->ipaddress[0]);
			ipss << "." << std::to_string(connection->ipaddress[1]);
			ipss << "." << std::to_string(connection->ipaddress[2]);
			ipss << "." << std::to_string(connection->ipaddress[3]);
			int result = inet_pton(AF_INET, ipss.str().c_str(), &target.sin_addr);
			if (result < 0)
			{
				std::stringstream ss;
				ss << "wiNetwork_Linux error in Send: (Error Code: " + std::to_string(result) + ") " + strerror(result);
				wiBackLog::post(ss.str().c_str());
				return false;
			}

			auto socketinternal = to_internal(sock);
			result = sendto(socketinternal->handle, (const char*)data, (int)dataSize, 0, (const sockaddr*)&target, sizeof(target));
			if (result < 0)
			{
				std::stringstream ss;
				ss << "wiNetwork_Linux error in Send: (Error Code: " + std::to_string(result) + ") " + strerror(result);
				wiBackLog::post(ss.str().c_str());
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
				std::stringstream ss;
				ss << "wiNetwork_Linux error in Send: (Error Code: " + std::to_string(result) + ") " + strerror(result);
				wiBackLog::post(ss.str().c_str());
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
				std::stringstream ss;
				ss << "wiNetwork_Linux error in Send: (Error Code: " + std::to_string(result) + ") " + strerror(result);
				wiBackLog::post(ss.str().c_str());
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
				std::stringstream ss;
				ss << "wiNetwork_Linux error in Send: (Error Code: " + std::to_string(result) + ") " + strerror(result);
				wiBackLog::post(ss.str().c_str());
				assert(0);
				return false;
			}

			connection->port = htons(sender.sin_port); // reverse byte order from network to host
			//Converting binary address to connection's address
			char ipaddress_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &sender.sin_addr, ipaddress_str, INET_ADDRSTRLEN);
			std::istringstream iss(ipaddress_str);
			std::string part;
			int part_seek = 0;
			while(std::getline(iss,part,'.')){
				if(!part.empty()){
					connection->ipaddress[part_seek] = atoi(part.c_str());
					part_seek++;
				}
			}

			return true;
		}
		return false;
	}
}

#endif // LINUX
