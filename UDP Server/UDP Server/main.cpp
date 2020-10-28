#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define PORT 8000

#include "Windows.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"

#include <iostream>
#include <cstdlib>

void printWSErrorAndExit(const char* msg)
{
	wchar_t* s = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM| FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&s, 0, NULL);
	
	fprintf(stderr, "%s: %S\n", msg, s);
	LocalFree(s);
	system("pause");
	exit(-1);
}

int main(int argc, char** argv)
{
	// Initialize Sockets Library
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	if (iResult != NO_ERROR)
		printWSErrorAndExit("WSAStartup");

	// Create UDP Socket
	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (s == INVALID_SOCKET)
		printWSErrorAndExit("socket");

	// Forcefully attaching socket to the port 8000
	int opt = 1;
	iResult = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));	
	
	if (iResult == SOCKET_ERROR)
		printWSErrorAndExit("setsockopt");

	// Bind 
	sockaddr_in bindAddr;
	bindAddr.sin_family = AF_INET;				// IPv4
	bindAddr.sin_port = htons(PORT);			// Port
	bindAddr.sin_addr.S_un.S_addr = INADDR_ANY; // Any local IP address
	
	iResult = bind(s, (const struct sockaddr*)&bindAddr, sizeof(bindAddr));
	
	if (iResult == SOCKET_ERROR)
		printWSErrorAndExit("bind");

	for (int i = 0; i < 5; ++i)
	{
		char buffer[50] = { 0 };

		sockaddr_in remoteAddr;
		int fromSize = sizeof(remoteAddr);
		iResult = recvfrom(s, buffer, sizeof(buffer), 0, (sockaddr*)&remoteAddr, &fromSize);
		
		if (iResult == SOCKET_ERROR)
			printWSErrorAndExit("recvfrom");
		
		std::cout << i << " Server receive: "<< buffer << std::endl;

		char msg[50] = { "Pong" };

		iResult = sendto(s, msg, sizeof(msg), 0, (sockaddr*)&remoteAddr, fromSize);
		
		if (iResult == SOCKET_ERROR)
			printWSErrorAndExit("sendto");	

		std::cout << i << " Server send: "<< msg << std::endl;
	}

	// Close socket
	iResult = closesocket(s);
	if (iResult == SOCKET_ERROR)
		printWSErrorAndExit("closesocket");
	
	// Shutdown Winsock
	iResult = WSACleanup();
	if (iResult == SOCKET_ERROR)
		printWSErrorAndExit("WSACleanup");
	
	system("pause");
	return 0;
}