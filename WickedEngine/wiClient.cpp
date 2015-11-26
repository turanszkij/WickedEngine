#include "wiClient.h"


#ifndef WINSTORE_SUPPORT

wiClient::wiClient(const string& newName, const string& ipaddress, int port)
{
	if(ConnectToHost(port,ipaddress.length()<=1?"127.0.0.1":ipaddress.c_str())){
		success=true;
		string welcomeMsg;
		if(receiveText(welcomeMsg)){
			stringstream ss("");
			ss<<"Client connected to: "<<welcomeMsg<<" ,IP: "<<ipaddress<<" [port: "<<port<<"]";
			wiBackLog::post(ss.str().c_str());
			changeName(newName);
			serverName=welcomeMsg;
		}
	}
	else{
		success=false;
		stringstream ss("");
		ss<<"Connecting to server on address: "<<ipaddress<< " [port "<<port<<"] FAILED with: "<<WSAGetLastError();
		wiBackLog::post(ss.str().c_str());
	}
}


wiClient::~wiClient(void)
{
	wiNetwork::~wiNetwork();
}


bool wiClient::sendText(const string& text){
	return wiNetwork::sendText(text,s);
}
bool wiClient::receiveText(string& text){
	return wiNetwork::receiveText(text,s);
}
bool wiClient::changeName(const string& newName){
	wiNetwork::changeName(newName);
	wiNetwork::sendData(wiNetwork::PACKET_TYPE_CHANGENAME,s);
	return wiNetwork::sendText(newName,s);
}
bool wiClient::sendMessage(const string& text){
	wiNetwork::sendData(wiNetwork::PACKET_TYPE_TEXTMESSAGE,s);
	return wiNetwork::sendText(text,s);
}

#endif

