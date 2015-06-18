#ifndef SERVER_H
#define SERVER_H

#include "Network.h"
#include <ctime>
#include <map>


class Server : public Network
{
private:
	map<SOCKET,string> clients;
public:
	Server(const string& newName = "SERVER", int port = PORT, const string& ipaddress = "0.0.0.0");
	~Server(void);

	bool active(){return !clients.empty();}

	
	bool sendText(const string& text, int packettype, const string& clientName = "", int clientID = -1);
	template <typename T>
	bool sendData(const T& value){
		if(clients.begin()==clients.end())
			return false;
		Network::sendData(PACKET_TYPE_OTHER,clients.begin()->first);
		return Network::sendData(value,clients.begin()->first);
	}
	template <typename T>
	bool receiveData(T& value){
		if(clients.begin()==clients.end())
			return false;
		return Network::receiveData(value,clients.begin()->first);
	}

	
	bool changeName(const string& newName);
	bool sendMessage(const string& text, const string& clientName = "", int clientID = -1);

	bool ListenOnPort(int portno, const char* ipaddress);
	SOCKET CreateAccepter();

	template<typename T>
	void Poll(T& data)
	{
		

		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(s, &readfds);
		SOCKET max_sd = s;


		for (map<SOCKET,string>::iterator it = clients.begin(); it != clients.end(); ++it) {
			if(it->first>0)
				FD_SET( it->first, &readfds );
			if(it->first>max_sd)
				max_sd=it->first;
		}


		timeval time=timeval();
		time.tv_sec=0;
		time.tv_usec=1;

		if ((select( max_sd + 1 , &readfds , NULL , NULL , &time) < 0) && (errno!=EINTR))
		{
			BackLog::post("select error");
		}

		//If something happened on the master socket , then its an incoming connection
		if (FD_ISSET(s, &readfds))
		{
			SOCKET new_socket = CreateAccepter();

			if(new_socket != SOCKET_ERROR){
				if(Network::sendText(name,new_socket)){
					stringstream ss("");
					ss<<"Name sent, client ["<<new_socket<<"] connected";
					BackLog::post(ss.str().c_str());

					ss.str("");
					ss<<"Unnamed_client_"<<new_socket;
					clients.insert(pair<SOCKET,string>(new_socket,ss.str()));

				}
				else
					BackLog::post("Welcome message could NOT be sent, client ignored");

			}
		}

		//else its some IO operation on some other socket :)
		for (map<SOCKET,string>::iterator it = clients.begin(); it != clients.end(); ) {

			if (FD_ISSET( it->first , &readfds))
			{
				int command;
				bool receiveSuccess = Network::receiveData(command,it->first);

				if(!receiveSuccess){
					BackLog::post("Client disconnected.");
					closesocket(it->first);
					clients.erase(it++);
					continue;
				}
				else{

					switch(command){
					case PACKET_TYPE_CHANGENAME:
						{
							string text;
							if(receiveText(text,it->first)){
								stringstream ss("");
								ss<<"Client "<<it->second<<" now registered as "<<text;
								BackLog::post(ss.str().c_str());
								it->second=text;
							}
							break;
						}
					case PACKET_TYPE_TEXTMESSAGE:
						{
							string text;
							if(receiveText(text,it->first)){
								stringstream ss("");
								ss<<it->second<<": "<<text;
								BackLog::post(ss.str().c_str());
							}
							break;
						}
					case PACKET_TYPE_OTHER:
						{
							Network::receiveData(data,it->first);
							break;
						}
					default:
						break;
					}
				}

			}
			++it;
		}

	} 


	vector<string> listClients();
	

};

#endif