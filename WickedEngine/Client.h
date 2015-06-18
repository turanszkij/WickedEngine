#ifndef CLIENT_H
#define CLIENT_H

#include "Network.h"

class Client : public Network
{
public:
	Client(const string& newName = "CLIENT", const string& ipaddress = "127.0.0.1", int port = PORT);
	~Client(void);

	string serverName;

	
	bool sendText(const string& text);
	bool receiveText(string& newName);

	template <typename T>
	bool sendData(const T& value){
		Network::sendData(PACKET_TYPE_OTHER,s);
		return Network::sendData(value,s);
	}
	template <typename T>
	bool receiveData(T& value){
		return Network::receiveData(value,s);
	}

	bool changeName(const string& newName);
	bool sendMessage(const string& text);
	
	bool ConnectToHost(int PortNo, const char* IPAddress)
	{
		WSAData wsadata;
		int error = WSAStartup(SCK_VERSION2,&wsadata);

		if(error)
			return false;

		if(wsadata.wVersion != SCK_VERSION2){
			WSACleanup();
			return false;
		}

		SOCKADDR_IN target;

		target.sin_family = AF_INET;
		target.sin_port = htons(PortNo);
		target.sin_addr.S_un.S_addr = inet_addr(IPAddress);

		s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(s == INVALID_SOCKET){
			return false;
		}

		int opt = TRUE;
		if( setsockopt(s, SOL_SOCKET, TCP_NODELAY, (char *)&opt, sizeof(opt)) < 0 )
			return false;

		//Try connecting:

		if(connect(s, (SOCKADDR*)&target, sizeof(target)) == SOCKET_ERROR){
			return false;
		}
		else
			return true;

	}

	template<typename T>
	void Poll(T& data)
	{

		if(!success)
			return;

		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(s, &readfds);


		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		timeval time=timeval();
		time.tv_sec=0;
		time.tv_usec=1;

		if ((select( s + 1 , &readfds , NULL , NULL , &time) < 0) && (errno!=EINTR))
		{
			BackLog::post("select error");
		}

		//If something happened on the master socket , then its an incoming connection
		if (FD_ISSET(s, &readfds))
		{
			int command;
			bool receiveSuccess = receiveData(command);

			if(!receiveSuccess){
				BackLog::post("Server no longer available. Please disconnect.");
				closesocket(s);
				success=false;
			}
			else{
				switch(command){
				case PACKET_TYPE_CHANGENAME:
					{
						string text;
						if(receiveText(text)){
							stringstream ss("");
							ss<<"New server name is: "<<text;
							BackLog::post(ss.str().c_str());
							serverName=text;
						}
						break;
					}
				case PACKET_TYPE_TEXTMESSAGE:
					{
						string text;
						if(receiveText(text)){
							stringstream ss("");
							ss<<serverName<<": "<<text;
							BackLog::post(ss.str().c_str());
						}
						break;
					}
				case PACKET_TYPE_OTHER:
					{
						Network::receiveData(data,s);
						break;
					}
				default:
					break;
				}
			}
		}

	} 


};

#endif