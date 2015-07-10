#include "wiServer.h"

#ifndef WINSTORE_SUPPORT

using namespace std;


Server::Server(const string& newName, int port, const string& ipaddress)
{
	name=newName;
	if(ListenOnPort(port,ipaddress.length()<=1?"0.0.0.0":ipaddress.c_str())){
		stringstream ss("");
		ss<<"Listening as "<<name<<" ,IP: "<<ipaddress<<" [port: "<<port<<"]";
		wiBackLog::post(ss.str().c_str());
		success=true;
	}
	else{
		stringstream ss("");
		ss<<"Creating server on address: "<<ipaddress<< " [port "<<port<<"] FAILED with: "<<WSAGetLastError();
		wiBackLog::post(ss.str().c_str());
		success=false;
	}
}


Server::~Server(void)
{
	for (map<SOCKET,string>::iterator it = clients.begin(); it != clients.end(); ++it) {
		closesocket(it->first);
	}
	Network::~Network();
}

bool Server::ListenOnPort(int portno, const char* ipaddress)
{
	int error = WSAStartup(SCK_VERSION2,&w);

	if(error)
		return false;

	if(w.wVersion != SCK_VERSION2){
		WSACleanup();
		return false;
	}

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port= htons(portno);
	//addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); //or inet_addr("0.0.0.0");
	addr.sin_addr.S_un.S_addr = inet_addr(ipaddress);

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	

	//set master socket to allow multiple connections , this is just a good habit, it will work without this
	int opt = TRUE;
	if( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
		return false;


	if( setsockopt(s, SOL_SOCKET, TCP_NODELAY, (char *)&opt, sizeof(opt)) < 0 )
		return false;

	if(s == INVALID_SOCKET)
		return false;

	if(::bind(s, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
		return false;  //couldn't bind (for example if you try to bind more than once to the same socket

	//if( listen(s,SOMAXCONN) == SOCKET_ERROR )
	if( listen(s,SOMAXCONN) == SOCKET_ERROR )
		return false;



	return true;
}
SOCKET Server::CreateAccepter(){
	struct sockaddr_in caller;
	int addrlen = sizeof(caller);
	SOCKET newsock = accept(s,(struct sockaddr *) &caller, &addrlen);

	//inform user of socket number - used in send and receive commands
	if(newsock != SOCKET_ERROR){
		//printf("\nNew connection , socket fd is %d , ip is : %s , port : %d \n" , newsock , inet_ntoa(caller.sin_addr) , ntohs(caller.sin_port));
	}
	else{
		//printf("\nConnection FAILED on socket: fd is %d , ip is : %s , port : %d \n" , newsock , inet_ntoa(caller.sin_addr) , ntohs(caller.sin_port));
	}

	return newsock;
}



vector<string> Server::listClients()
{
	vector<string> ret(0);
	for (map<SOCKET,string>::iterator it = clients.begin(); it != clients.end(); ++it) {
		stringstream ss("");
		ss<<it->second<<":"<<it->first;
		ret.push_back(ss.str());
	}
	return ret;
}

bool Server::sendText(const string& text, int packettype, const string& clientName, int clientID){
	int sentTo=0;

	if(clientName.length()<=0){ //send to everyone
		for (map<SOCKET,string>::iterator it = clients.begin(); it != clients.end(); ++it) {
			sentTo += Network::sendData(packettype,it->first) && Network::sendText(text,it->first);
		}
	}
	else if(clientID<0){ //send to all of same name
		for (map<SOCKET,string>::iterator it = clients.begin(); it != clients.end(); ++it) {
			if(!clientName.compare(it->second)){
				sentTo += Network::sendData(packettype,it->first) && Network::sendText(text,it->first);
			}
		}
	}
	else{ //send to specific client
		if(clients.find(clientID) != clients.end()){
			sentTo += Network::sendData(packettype,clientID) && Network::sendText(text,clientID);
		}
	}

	if(sentTo<=0)
		wiBackLog::post("No client was found with the specified parameters");

	return sentTo>0;
}

bool Server::changeName(const string& newName){
	Network::changeName(newName);
	return sendText(newName, Network::PACKET_TYPE_CHANGENAME);
}
bool Server::sendMessage(const string& text, const string& clientName, int clientID){
	return sendText(text, Network::PACKET_TYPE_TEXTMESSAGE, clientName, clientID);
}

#endif
