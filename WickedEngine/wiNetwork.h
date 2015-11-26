#ifndef NETWORK_H
#define NETWORK_H

#include "wiBackLog.h"

#include <Windows.h>
#include <winsock.h>
#include <string>
#include <sstream>


#ifndef WINSTORE_SUPPORT

class wiNetwork
{
protected:
	
#define SCK_VERSION2            0x0202

	SOCKET s;
	WSADATA w;
	fd_set readfds;

	std::string name;

public:
	
	static const int PORT = 65000;

	static const int PACKET_TYPE_CHANGENAME = 0;
	static const int PACKET_TYPE_TEXTMESSAGE = 1;
	static const int PACKET_TYPE_OTHER = 2;
	bool success;

	wiNetwork(void);
	~wiNetwork(void);

	virtual bool changeName(const string& newName){
		name=newName;
		return true;
	}
	
	void CloseConnection()
	{
		if(s)
			closesocket(s);

		WSACleanup();


		//wiBackLog::post("\n\nClosed Connection");
	}

	static bool sendText(const std::string& text, SOCKET socket);
	template <typename T>
	static bool sendData(const T& value, SOCKET socket){
		int sent = send(socket, (const char*)&value, sizeof(value), 0);

		if(sent==SOCKET_ERROR) {
			//stringstream ss("");
			//ss << "\n[Error] Sending data failed with error: " << WSAGetLastError();
			//wiBackLog::post(ss.str().c_str());
			return false;
		}

		return true;
	}

	static bool receiveText(std::string& text, SOCKET socket);
	template <typename T>
	static bool receiveData(T& value, SOCKET socket){
		char puffer[sizeof(value)];
		int received = recv(socket, puffer, sizeof(puffer), 0);
		if(received<=0){
			//cout<< "[Error][receiveNumber] The recv call failed with error!\n";
		} 
		else if (received <= sizeof(value)) {
			value = *((T*)puffer);
			return true;
		}
		else{
			//cout<< "[Error][receiveNumber] Too long request!\n";
		}
		return false;
	}
};

#endif //FAMILY

#endif

