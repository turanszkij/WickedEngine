#include "Client.h"


Client::Client(const string& newName, const string& ipaddress, int port)
{
	if(ConnectToHost(port,ipaddress.length()<=1?"127.0.0.1":ipaddress.c_str())){
		success=true;
		string welcomeMsg;
		if(receiveText(welcomeMsg)){
			stringstream ss("");
			ss<<"Client connected to: "<<welcomeMsg<<" ,IP: "<<ipaddress<<" [port: "<<port<<"]";
			BackLog::post(ss.str().c_str());
			changeName(newName);
			serverName=welcomeMsg;
		}
	}
	else{
		success=false;
		stringstream ss("");
		ss<<"Connecting to server on address: "<<ipaddress<< " [port "<<port<<"] FAILED with: "<<WSAGetLastError();
		BackLog::post(ss.str().c_str());
	}
}


Client::~Client(void)
{
	Network::~Network();
}


bool Client::sendText(const string& text){
	return Network::sendText(text,s);
}
bool Client::receiveText(string& text){
	return Network::receiveText(text,s);
}
bool Client::changeName(const string& newName){
	Network::changeName(newName);
	Network::sendData(Network::PACKET_TYPE_CHANGENAME,s);
	return Network::sendText(newName,s);
}
bool Client::sendMessage(const string& text){
	Network::sendData(Network::PACKET_TYPE_TEXTMESSAGE,s);
	return Network::sendText(text,s);
}

