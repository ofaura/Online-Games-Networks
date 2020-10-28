#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define PORT 8000
#define SERVER_ADDRESS "127.0.0.1"

#include "Windows.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"

#include <iostream>
#include <cstdlib>

void printWSErrorAndExit(const char* msg)
{
	wchar_t* s = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
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
	
	if (iResult == SOCKET_ERROR)
		printWSErrorAndExit("WSAStartup");
	
	// Create UDP Socket
	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (s == INVALID_SOCKET)
		printWSErrorAndExit("socket");
	
	// Create an address object with the server address
	sockaddr_in remoteAddr;
	remoteAddr.sin_family = AF_INET;		// IPv4
	remoteAddr.sin_port = htons(PORT);		// Port
	const char* toAddrStr = SERVER_ADDRESS; // Not so remote… :-P
	inet_pton(AF_INET, toAddrStr, &remoteAddr.sin_addr);
	
	for (int i = 0; i < 5; ++i)
	{
		char buffer[50] = { "Ping" };

		iResult = sendto(s, buffer, sizeof(buffer), 0, (sockaddr*)&remoteAddr, sizeof(remoteAddr));
		
		if (iResult == SOCKET_ERROR)
			printWSErrorAndExit("sendto");
		
		std::cout << i << " Client send: " << buffer << std::endl;

		sockaddr_in fromAddr;
		int fromSize = sizeof(fromAddr);
		iResult = recvfrom(s, buffer, sizeof(buffer), 0, (sockaddr*)&fromAddr, &fromSize);		
		
		std::cout << i << " Client receive: " << buffer << std::endl;

		if (iResult == SOCKET_ERROR)
				printWSErrorAndExit("recvfrom");
		
		Sleep(500);
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