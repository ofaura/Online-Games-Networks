// Rafel Brau & Òscar Faura

#include "ModuleNetworking.h"
#include "Networks.h"

#include <list>

static uint8 NumModulesUsingWinsock = 0;


void ModuleNetworking::reportError(const char* inOperationDesc)
{
	LPVOID lpMsgBuf;
	DWORD errorNum = WSAGetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	ELOG("Error %s: %d- %s", inOperationDesc, errorNum, lpMsgBuf);
}

void ModuleNetworking::disconnect()
{
	for (SOCKET socket : sockets)
	{
		shutdown(socket, 2);
		closesocket(socket);
	}

	sockets.clear();
}

bool ModuleNetworking::init()
{
	if (NumModulesUsingWinsock == 0)
	{
		NumModulesUsingWinsock++;

		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		if (WSAStartup(version, &data) != 0)
		{
			reportError("ModuleNetworking::init() - WSAStartup");
			return false;
		}
	}

	return true;
}

bool ModuleNetworking::preUpdate()
{
	if (sockets.empty()) return true;

	// NOTE(jesus): You can use this temporary buffer to store data from recv()
	const uint32 incomingDataBufferSize = Kilobytes(1);
	byte incomingDataBuffer[incomingDataBufferSize];

	// TODO(jesus): select those sockets that have a read operation available
	
	// New socket set
	fd_set readfds;
	FD_ZERO(&readfds);
	
	// Fill the set
	for (auto s : sockets) 
	{
		FD_SET(s, &readfds);
	}

	// Timeout (return immediately)
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	// Select (check for readability)
	int res = select(0, &readfds, nullptr, nullptr, &timeout);
	if (res == SOCKET_ERROR) 
	{
		reportError("select 4 read");
	}

	// TODO(jesus): for those sockets selected, check wheter or not they are
	// a listen socket or a standard socket and perform the corresponding
	// operation (accept() an incoming connection or recv() incoming data,
	// respectively).
	// On accept() success, communicate the new connected socket to the
	// subclass (use the callback onSocketConnected()), and add the new
	// connected socket to the managed list of sockets.
	// On recv() success, communicate the incoming data received to the
	// subclass (use the callback onSocketReceivedData()).

	std::list<SOCKET> disconnectedSockets;

	// Read selected sockets
	for (auto s : sockets)
	{
		if (FD_ISSET(s, &readfds)) 
		{
			// Is the server socket
			// Accept stuff
			if (isListenSocket(s))
			{ 
				sockaddr_in address;
				int addressSize = sizeof(address);
				SOCKET connectedSocket = accept(s, (sockaddr*)&address, &addressSize);
				
				if (connectedSocket == INVALID_SOCKET) 
					reportError("accept reported INVALID_ERROR");
				
				else 
				{
					onSocketConnected(connectedSocket, address);
					addSocket(connectedSocket);
				}
			}
			// Is a client socket
			// Recv stuff
			else 
			{ 
				InputMemoryStream packet;
				int res = recv(s, packet.GetBufferPtr(), packet.GetCapacity(), 0);

				if (res == SOCKET_ERROR || res == 0)
				{
					reportError("recv");
					disconnectedSockets.push_back(s);
				}
				
				else 
				{
					packet.SetSize(res);
					onSocketReceivedData(s, packet);
				}
			}
		}
	}

	// TODO(jesus): handle disconnections. Remember that a socket has been
	// disconnected from its remote end either when recv() returned 0,
	// or when it generated some errors such as ECONNRESET.
	// Communicate detected disconnections to the subclass using the callback
	// onSocketDisconnected().

	// TODO(jesus): Finally, remove all disconnected sockets from the list
	// of managed sockets.

	for (auto& s : disconnectedSockets) 
	{
		onSocketDisconnected(s);
		sockets.erase(std::find(sockets.begin(), sockets.end(), s));
	}

	return true;
}

bool ModuleNetworking::cleanUp()
{
	disconnect();

	NumModulesUsingWinsock--;
	if (NumModulesUsingWinsock == 0)
	{

		if (WSACleanup() != 0)
		{
			reportError("ModuleNetworking::cleanUp() - WSACleanup");
			return false;
		}
	}

	return true;
}

bool ModuleNetworking::sendPacket(const OutputMemoryStream& packet, SOCKET socket)
{
	int result = send(socket, packet.GetBufferPtr(), packet.GetSize(), 0);
	
	if (result == SOCKET_ERROR)
	{
		reportError("send");
		return false;
	}

	return true;
}

void ModuleNetworking::addSocket(SOCKET socket)
{
	sockets.push_back(socket);
}
