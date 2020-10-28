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
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	if (iResult == SOCKET_ERROR)
		printWSErrorAndExit("WSAStartup");

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	
	if (s == INVALID_SOCKET)
		printWSErrorAndExit("socket");
	
	sockaddr_in toAddr;
	toAddr.sin_family = AF_INET;			
	toAddr.sin_port = htons(PORT);			
	const char* toAddrStr = SERVER_ADDRESS; 
	inet_pton(AF_INET, toAddrStr, &toAddr.sin_addr);

	iResult = connect(s, (sockaddr*)&toAddr, sizeof(toAddr));
	
	if (iResult == SOCKET_ERROR)
		printWSErrorAndExit("connect");
	
	for (int i = 0; i < 5; ++i)
	{
		char buffer[50] = { "Ping" };

		iResult = send(s, buffer, sizeof(buffer), 0);
		
		if (iResult == SOCKET_ERROR)
			printWSErrorAndExit("send");

		std::cout << i << " Client send: " << buffer << std::endl;

		iResult = recv(s, buffer, sizeof(buffer), 0);
		
		if (iResult == SOCKET_ERROR)
			printWSErrorAndExit("recv");

		std::cout << i << " Client receive: " << buffer << std::endl;

		Sleep(500);
	}

	iResult = closesocket(s);
	
	if (iResult == SOCKET_ERROR)
		printWSErrorAndExit("closesocket");

	iResult = WSACleanup();
	
	if (iResult == SOCKET_ERROR)
		printWSErrorAndExit("WSACleanup");

	system("pause");
	return 0;
}
