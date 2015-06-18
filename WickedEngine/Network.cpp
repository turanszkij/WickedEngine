#include "Network.h"


Network::Network(void)
{
	name="UNNAMED_NETWORK";
}


Network::~Network(void)
{
	CloseConnection();
}


bool Network::sendText(const std::string& text, SOCKET socket){
	if(sendData((int)text.length(),socket)){
		int sent = send(socket, text.c_str(), text.length(), 0);

		if(sent==SOCKET_ERROR) {
			//cout << "\n[Error] Sending text '"<<text<<"' failed with error: " << WSAGetLastError();
			BackLog::post("[Error][sendText] Sending text failed!");
			return false;
		}

		return true;
	}
	BackLog::post("[Error][sendText] Sending text length failed!");
	return false;
}

bool Network::receiveText(std::string& text, SOCKET socket){
	int textlen;
	if(receiveData(textlen,socket)){
		char* puffer=new char[textlen];
		memset(puffer, '\0', textlen);
		int received = recv(socket, puffer, textlen, 0);
		if(received<=0){
			//cout<< "[Error][receiveText] The recv call failed with error!\n";
			BackLog::post("[Error][receiveText] The recv call failed with error!");
		} 
		else if (received <= textlen) {
			puffer[received]='\0';
			text = puffer;
			return true;
		}
		else{
			//cout<< "[Error][receiveText] Too long request!\n";
			BackLog::post("[Error][receiveText] Too long request!");
		}
		return false;
	}
	BackLog::post("[Error][receiveText] Didn't receive text length!");
	return false;
}

