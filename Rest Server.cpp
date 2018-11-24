#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <queue>
#include <sys/time.h>
#include <sys/types.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sstream>

using namespace std;
#define BUFFER_SIZE 1024
#define BACKLOG 10 
class RESTServer
{
private:
	int sockfd;
	string port;
	pthread_t listenThread;

	void *get_in_addr(struct sockaddr *);
	bool CheckAuthroization(string, string);
	bool ParseHTMLHeader(string, string);
	void ProcessConnection(int, string);
	void * StartServer(void *);
	static void *StartServer_helper(void *);
	bool Running;
	string AUTHORIZED_CLIENTS;

public:
	void Start(string _port);
	void Stop();
};

//{ RESTServer class Functions
void *RESTServer::get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//Check command is received from and authorized client and do what you want based on the command
bool RESTServer::CheckAuthroization(string client, string command)
{
	
	if (strstr(AUTHORIZED_CLIENTS.c_str(), client.c_str()) != NULL)
	{
		if (command.compare("close") == 0 || command.compare("Close") == 0)
		{			 
			return true;
		}
	}
	 
	return false;
}
bool RESTServer::ParseHTMLHeader(string text, string client)
{
	try
	{
		 
		string command = "";
		string get = "GET /";
		string host = "Host:";
		int lock = text.find(get.c_str());
		for (int i = lock + get.length(); ; i++)
		{
			if (text[i] == ' ')
			{
				break;
			}
			else

			{
				command += text[i];
			}
		}
		int lock2 = text.find(host.c_str());

		return CheckAuthroization(client, command); 
	}
	catch (std::exception& e)
	{
		 
		return false;
	}
}

void RESTServer::ProcessConnection(int new_fd, string client)
{
	char buffer[BUFFER_SIZE] = { 0 };
	string dataRead = "";

	while (Running)
	{
		int readAmount = read(new_fd, buffer, BUFFER_SIZE - 1);
		dataRead += buffer;
		if (buffer[readAmount - 2] == '\r' && buffer[readAmount - 1] == '\n')
		{
			break;
		}
		memset(buffer, 0, 1024);
	}
	bool var = ParseHTMLHeader(dataRead, client);
	time_t mytime = time(NULL);
	char * time_str = ctime(&mytime);
	time_str[strlen(time_str) - 1] = '\0';
	string CurrentTime(time_str);
	free((char*)time_str);

	if (var)
	{
		string msg = "HTTP/1.1 200 OK\nDate: " + CurrentTime + "\nServer: MyTestServer\nLast-Modified: Wed, 22 Jul 2009 19:15:56 GMT\nContent-Length: 84\nContent-Type: text/html\nConnection: Closed\r\n \n\n<html>\n<head>\n<title>Response</title>\n</head>\n<body>\n<h1>Success</h1>\n</body>\n</html>";
		if (send(new_fd, msg.c_str(), msg.length() - 1, 0) == -1)
			printf("Send failed\n"); 
	}
	else
	{
		string msg = "HTTP/1.1 401 Unauthorized\nDate: " + CurrentTime + "\nServer: MyTestServer\nLast-Modified: Wed, 22 Jul 2009 19:15:56 GMT\nContent-Length: 88\nContent-Type: text/html\nConnection: Closed\r\n \n\n<html>\n<head>\n<title>Response</title>\n</head>\n<body>\n<h1>Unauthorized</h1>\n</body>\n</html>";
		if (send(new_fd, msg.c_str(), msg.length() - 1, 0) == -1)
			 printf("Send failed\n");
	}
	close(new_fd);
	if (var)
	{
		//Do some operation based on the command here
	}
}

void * RESTServer::StartServer(void * object)
{
	 
	int new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	int yes = 1;
	char client_addr[INET6_ADDRSTRLEN];
	int rv;
	try
	{
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE; // use my IP

		if ((rv = getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0)
		{
			 
			return NULL;
		}

		// loop through all the results and bind to the first we can
		for (p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
				 
				continue;
			}

			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
				 
				return NULL;
			}

			if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(sockfd);
				 
				continue;
			}
			break;
		}

		freeaddrinfo(servinfo); // all done with this structure

		if (p == NULL) {
			 
			return NULL;
		}

		if (listen(sockfd, BACKLOG) == -1) {
			 
			return NULL;
		}

		 
	}
	catch (std::exception& e)
	{
		 
	}
	while (Running)
	{  // main accept() loop
		try
		{
			sin_size = sizeof their_addr;
			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (new_fd == -1) {
				 
				continue;
			}

			inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				client_addr, sizeof client_addr);
			 

			ProcessConnection(new_fd, client_addr);
		}
		catch (std::exception& e)
		{
			 
		}
	}
	return NULL;
}
void *RESTServer::StartServer_helper(void *context)
{
	return ((RESTServer *)context)->StartServer(context);
}
void RESTServer::Start(string _port)
{
	AUTHORIZED_CLIENTS = "[10.105.12.139][10.105.27.45]"; //Make a function to populate authorized clients in the format specified for ease
	Running = true;
	port = _port.c_str();
	pthread_create(&listenThread, NULL, RESTServer::StartServer_helper, this);
	pthread_detach(listenThread);
}
void RESTServer::Stop()
{
	close(sockfd);
	Running = false;
}

int main()
{
   RESTServer server;
   server.Start("8030");
    return 0;
}
//}/////////////////////////////////////////////////////////////////////////////////////