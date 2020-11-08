#include "ModuleNetworkingServer.h"

#include <vector>
#include <string>

//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::start(int port)
{
	// TODO(jesus): TCP listen socket stuff
	// - Create the listenSocket
	// - Set address reuse
	// - Bind the socket to a local interface
	// - Enter in listen mode
	// - Add the listenSocket to the managed list of sockets using addSocket()
	
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	if (listenSocket == INVALID_SOCKET) 
	{
		reportError("socket reported INVALID_SOCKET");
		return false;
	}

	int enable = 1;
	
	if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)) == SOCKET_ERROR) 
	{
		reportError("setsockopt reported SOCKET_ERROR");
		return false;
	}

	sockaddr_in localAddr;
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(port);
	localAddr.sin_addr.S_un.S_addr = INADDR_ANY;

	if (bind(listenSocket, (const sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) 
	{
		reportError("bind reported SOCKET_ERROR");
		return false;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		reportError("listen reported SOCKET_ERROR");
		return false;
	}

	addSocket(listenSocket);

	state = ServerState::Listening;

	return true;
}

bool ModuleNetworkingServer::isRunning() const
{
	return state != ServerState::Stopped;
}



//////////////////////////////////////////////////////////////////////
// Module virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::update()
{
	return true;
}

bool ModuleNetworkingServer::gui()
{
	if (state != ServerState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Server Window");

		Texture *tex = App->modResources->server;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("List of connected sockets:");

		for (auto &connectedSocket : connectedSockets)
		{
			ImGui::Separator();
			ImGui::Text("Socket ID: %d", connectedSocket.socket);
			ImGui::Text("Address: %d.%d.%d.%d:%d",
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b1,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b2,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b3,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b4,
				ntohs(connectedSocket.address.sin_port));
			ImGui::Text("Player name: %s", connectedSocket.playerName.c_str());
		}

		ImGui::End();
	}

	return true;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::isListenSocket(SOCKET socket) const
{
	return socket == listenSocket;
}

void ModuleNetworkingServer::onSocketConnected(SOCKET socket, const sockaddr_in &socketAddress)
{
	// Add a new connected socket to the list
	ConnectedSocket connectedSocket;
	connectedSocket.socket = socket;
	connectedSocket.address = socketAddress;
	connectedSockets.push_back(connectedSocket);
}

void ModuleNetworkingServer::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	// Set the player name of the corresponding connected socket proxy
	ClientMessage clientMessage;
	packet >> clientMessage;

	switch (clientMessage)
	{
		case ClientMessage::Hello:
		{
			std::string playerName;
			packet >> playerName;

			for (auto& connectedSocket : connectedSockets)
			{
				std::string addr = std::to_string(connectedSocket.address.sin_addr.S_un.S_un_b.s_b1)
					+ "." + std::to_string(connectedSocket.address.sin_addr.S_un.S_un_b.s_b2)
					+ "." + std::to_string(connectedSocket.address.sin_addr.S_un.S_un_b.s_b3)
					+ "." + std::to_string(connectedSocket.address.sin_addr.S_un.S_un_b.s_b4);

				for (std::map<std::string, std::string>::iterator it = bannedSockets.begin(); it != bannedSockets.end(); ++it)
				{
					if (addr == (*it).second)
					{
						OutputMemoryStream packet;
						packet << ServerMessage::Remove;

						if (!sendPacket(packet, connectedSocket.socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}

						break;
					}
				}
			}

			bool newName = false;
			for (auto& connectedSocket : connectedSockets)
			{
				int count = 0;

				int length = playerName.length();
				int sLength = connectedSocket.playerName.length();

				if (connectedSocket.socket != socket & connectedSocket.playerName == playerName)
				{
					for (auto& cSocket : connectedSockets)
					{
						if (cSocket.playerName.find(playerName) != std::string::npos)
							count++;
					}

					playerName += std::to_string(count);
					newName = true;
					break;
				}
			}

			if (newName)
			{
				for (auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.socket == socket)
					{
						connectedSocket.playerName = playerName;
						OutputMemoryStream packet;
						packet << ServerMessage::ChangeName;
						packet << "Name changed to: " + playerName + "\nTo change your name use command: /change_name [newName]";

						if (!sendPacket(packet, socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}

						break;
					}
				}
			}

			for (auto& connectedSocket : connectedSockets)
			{
				OutputMemoryStream packet;

				if (connectedSocket.socket == socket)
					connectedSocket.playerName = playerName;		

				else
				{
					std::string message = "***** " + playerName + " joined *****";
					packet << ServerMessage::Connected;
					packet << message;

					if (!sendPacket(packet, connectedSocket.socket))
					{
						disconnect();
						state = ServerState::Stopped;

						break;
					}
				}
			}
			break;
		}

		case ClientMessage::Line:
		{
			std::string message;
			packet >> message;

			if (message == "/help")
			{
				OutputMemoryStream packet;
				packet << ServerMessage::Help;
				packet << "***** Commands list *****\n/help\n/list\n/kick [username]\n/whisper [username] [message]\n/change_name [username]\n/exit\n/mute [username]\n/unmute [username]\n/muted_list\n/clear\n/ban [username]\n/unban\n/banned_list";

				if (!sendPacket(packet, socket))
				{
					disconnect();
					state = ServerState::Stopped;
				}
			}

			else if (message == "/list")
			{
				std::string userList = "***** Users list *****";

 				for (const auto& connectedSocket : connectedSockets)
				{
					userList += "\n- " + connectedSocket.playerName;
				}

				OutputMemoryStream packet;
				packet << ServerMessage::List;
				packet << userList;

				if (!sendPacket(packet, socket))
				{
					disconnect();
					state = ServerState::Stopped;
				}
			}

			else if (message.find("/kick") != std::string::npos)
			{
				std::string line = "/kick ";
				
				if (message.length() <= line.length())
					break;
				
				std::string playerName = message.substr(line.length());

				if (!CheckUsername(playerName, socket))
					return;

				for (const auto& connectedSocket : connectedSockets) 
				{
					if (connectedSocket.playerName == playerName) 
					{
						OutputMemoryStream packet;
						packet << ServerMessage::Remove;

						if (!sendPacket(packet, connectedSocket.socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}

						break;
					}
				}
			}

			else if (message.find("/whisper") != std::string::npos)
			{
				std::string line = "/whisper ";

				if (message.length() <= line.length())
					break;
				
				std::string text = message.substr(line.length());

				std::string spacing = " ";
				std::size_t spacingIndex = text.find(spacing);
				std::string playerName = text.substr(0, spacingIndex);
				std::string sentence = text.substr(spacingIndex + spacing.length());

				if (!CheckUsername(playerName, socket))
					return;

				ConnectedSocket fromConnectedSocket;
				for (const auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.socket == socket)
					{
						fromConnectedSocket = connectedSocket;
						break;
					}
				}

				for (const auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.playerName == playerName)
					{
						OutputMemoryStream packet;
						packet << ServerMessage::Whisper;
						packet << fromConnectedSocket.playerName + " whispers to " + playerName + ": " + sentence;

						if (!sendPacket(packet, connectedSocket.socket) || !sendPacket(packet, fromConnectedSocket.socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}

						break;
					}
				}
			}

			else if (message.find("/change_name") != std::string::npos)
			{
				std::string line = "/change_name ";

				if (message.length() <= line.length())
					break;

				std::string playerName = message.substr(line.length());

				for (auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.socket == socket)
					{
						connectedSocket.playerName = playerName;

						OutputMemoryStream packet;
						packet << ServerMessage::ChangeName;
						packet << "Name changed to: " + playerName;

						if (!sendPacket(packet, connectedSocket.socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}

						break;
					}
				}
			}

			else if (message == "/exit")
			{
				for (const auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.socket == socket)
					{
						OutputMemoryStream packet;
						packet << ServerMessage::Remove;

						if (!sendPacket(packet, connectedSocket.socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}

						break;
					}
				}
			}

			else if (message == "/muted_list")
			{
				std::string userList = "***** Muted list *****";

				for (auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.socket == socket)
					{
						for (std::map<SOCKET, std::string>::iterator it = connectedSocket.mutedSockets.begin(); it != connectedSocket.mutedSockets.end(); ++it)
						{
							userList += "\n- " + (*it).second;
						}
					}
				}

				OutputMemoryStream packet;
				packet << ServerMessage::List;
				packet << userList;

				if (!sendPacket(packet, socket))
				{
					disconnect();
					state = ServerState::Stopped;
				}
			}

			else if (message.find("/mute") != std::string::npos)
			{
				std::string line = "/mute ";

				if (message.length() <= line.length())
					break;

				std::string playerName = message.substr(line.length());

				if (!CheckUsername(playerName, socket))
					return;		

				for (auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.socket == socket)
					{
						for (auto& cSocket : connectedSockets)
						{
							if (cSocket.playerName == playerName)
							{
								connectedSocket.mutedSockets.insert(std::make_pair(cSocket.socket, playerName));
								break;
							}
						}

						OutputMemoryStream packet;
						packet << ServerMessage::UnMute;
						packet << playerName + " has been muted";

						if (!sendPacket(packet, connectedSocket.socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}

						break;
					}
				}
			}

			else if (message.find("/unmute") != std::string::npos)
			{
				std::string line = "/unmute ";

				if (message.length() <= line.length())
					break;

				std::string playerName = message.substr(line.length());
				
				if (!CheckUsername(playerName, socket))
					return;

				unsigned int mutedSocket = 0;
				for (auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.playerName == playerName)
					{
						mutedSocket = connectedSocket.socket;
						break;
					}
				}

				for (auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.socket == socket)
					{
						for (std::map<SOCKET, std::string>::iterator it = connectedSocket.mutedSockets.begin(); it != connectedSocket.mutedSockets.end(); ++it)
						{
							if ((*it).first == mutedSocket)
							{
								connectedSocket.mutedSockets.erase(it);
								break;
							}
						}

						OutputMemoryStream packet;
						packet << ServerMessage::UnMute;
						packet << playerName + " has been unmuted";

						if (!sendPacket(packet, connectedSocket.socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}

						break;
					}					
				}
			}

			else if (message == "/clear")
			{
				OutputMemoryStream packet;
				packet << ServerMessage::Clear;

				if (!sendPacket(packet, socket))
				{
					disconnect();
					state = ServerState::Stopped;
				}
			}

			else if (message == "/banned_list")
			{
				std::string userList = "***** Banned list *****";

				for (std::map<std::string, std::string>::iterator it = bannedSockets.begin(); it != bannedSockets.end(); ++it)
				{
					userList += "\n- " + (*it).first;
				}

				OutputMemoryStream packet;
				packet << ServerMessage::List;
				packet << userList;

				if (!sendPacket(packet, socket))
				{
					disconnect();
					state = ServerState::Stopped;
				}
			}

			else if (message.find("/ban") != std::string::npos)
			{
				std::string line = "/ban ";

				if (message.length() <= line.length())
					break;

				std::string playerName = message.substr(line.length());

				if (!CheckUsername(playerName, socket))
					return;

				for (const auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.playerName == playerName)
					{
						OutputMemoryStream packet;
						packet << ServerMessage::Remove;

						std::string addr = std::to_string(connectedSocket.address.sin_addr.S_un.S_un_b.s_b1)
							+ "." + std::to_string(connectedSocket.address.sin_addr.S_un.S_un_b.s_b2)
							+ "." + std::to_string(connectedSocket.address.sin_addr.S_un.S_un_b.s_b3)
							+ "." + std::to_string(connectedSocket.address.sin_addr.S_un.S_un_b.s_b4);
						
						if (addr != "127.0.0.1")
						{
							bannedSockets.insert(std::make_pair(playerName, addr));

							if (!sendPacket(packet, connectedSocket.socket))
							{
								disconnect();
								state = ServerState::Stopped;
							}
						}
					}

					else
					{
						OutputMemoryStream packet;
						packet << ServerMessage::Ban;
						packet << playerName + " has been banned from the server";

						if (!sendPacket(packet, connectedSocket.socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}
					}
				}
			}

			else if (message.find("/unban") != std::string::npos)
			{
				std::string line = "/unban ";

				if (message.length() <= line.length())
					break;

				std::string playerName = message.substr(line.length());

				// Check if playerName is in the banned list
				{
					bool ret = false;
					for (std::map<std::string, std::string>::iterator it = bannedSockets.begin(); it != bannedSockets.end(); ++it)
					{
						if ((*it).first == playerName)
						{
							ret = true;
							break;
						}
					}

					if (!ret)
					{
						OutputMemoryStream packet;
						packet << ServerMessage::Error;
						packet << playerName + " is not a banned user";

						if (!sendPacket(packet, socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}
					}
				}
				
				for (const auto& connectedSocket : connectedSockets)
				{
					for (std::map<std::string, std::string>::iterator it = bannedSockets.begin(); it != bannedSockets.end(); ++it)
					{
						if (playerName == (*it).first)
						{
							bannedSockets.erase(it);
						}
					}

					if (connectedSocket.playerName != playerName)
					{
						OutputMemoryStream packet;
						packet << ServerMessage::Unban;
						packet << playerName + " has been unbanned from the server";

						if (!sendPacket(packet, connectedSocket.socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}
					}
				}
			}

			else if (message.at(0) == '/')
			{
				for (const auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.socket == socket)
					{
						OutputMemoryStream packet;
						packet << ServerMessage::Unknown;
						packet << message + "\nUnknown Command \nType /help to see all commands available";

						if (!sendPacket(packet, connectedSocket.socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}

						break;
					}
				}
			}

			else
			{
				std::string playerName;

				for (const auto& connectedSocket : connectedSockets)
				{			
					if (connectedSocket.socket == socket) 
					{
						playerName = connectedSocket.playerName;
						break;
					}
				}

				for (auto& connectedSocket : connectedSockets)
				{
					bool muted = false;

					for (std::map<SOCKET, std::string>::iterator it = connectedSocket.mutedSockets.begin(); it != connectedSocket.mutedSockets.end(); ++it)
					{
						if ((*it).first == socket)
							muted = true;
					}

					if (!muted)
					{
						OutputMemoryStream packet;
						packet << ServerMessage::Chat;
						packet << playerName + ": " + message;

						if (!sendPacket(packet, connectedSocket.socket))
						{
							disconnect();
							state = ServerState::Stopped;
						}
					}
				}
			}
		}
	}
}

void ModuleNetworkingServer::onSocketDisconnected(SOCKET socket)
{
	// Remove the connected socket from the list
	for (auto it = connectedSockets.begin(); it != connectedSockets.end(); ++it)
	{
		if ((*it).socket == socket)
		{
			std::string playerName = (*it).playerName;
			connectedSockets.erase(it);

			for (const auto& connectedSocket : connectedSockets) 
			{
				OutputMemoryStream packet;
				std::string message = "***** " + playerName + " left *****";
				packet << ServerMessage::Disconnected;
				packet << message;

				if (!sendPacket(packet, connectedSocket.socket))
				{
					disconnect();
					state = ServerState::Stopped;
				}
			}

			break;
		}
	}
}

bool ModuleNetworkingServer::CheckUsername(std::string name, SOCKET s)
{
	int sameUser = 0;

	for (auto& connectedSocket : connectedSockets)
	{
		if (connectedSocket.playerName == name & connectedSocket.socket == s)
			sameUser = 1;
		
		else if (connectedSocket.playerName == name & connectedSocket.socket != s)
			sameUser = 2;
	}

	if (sameUser == 1)
		return false;
	
	else if (sameUser == 2)
		return true;
	
	else 
	{
		OutputMemoryStream packet;
		packet << ServerMessage::Error;
		packet << name + " is not a user";

		if (!sendPacket(packet, s))
		{
			disconnect();
			state = ServerState::Stopped;
		}

		return false;
	}
}

