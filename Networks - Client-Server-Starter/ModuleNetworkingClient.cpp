#include "ModuleNetworkingClient.h"

bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	playerName = pplayerName;

	// TODO(jesus): TCP connection stuff
	// - Create the socket
	// - Create the remote address object
	// - Connect to the remote address
	// - Add the created socket to the managed list of sockets using addSocket()

	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s == INVALID_SOCKET) 
	{
		reportError("socket reported INVALID_ERROR");
		return false;
	}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPort);
	const char* toAddrStr = serverAddressStr;
	inet_pton(AF_INET, toAddrStr, &serverAddress.sin_addr);

	if (connect(s, (const sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) 
	{
		reportError("connect reported SOCKET_ERROR");
		return false;
	}

	addSocket(s);

	messagesList.clear();

	// If everything was ok... change the state
	state = ClientState::Start;

	return true;
}

bool ModuleNetworkingClient::isRunning() const
{
	return state != ClientState::Stopped;
}

bool ModuleNetworkingClient::update()
{
	if (state == ClientState::Start)
	{
		// TODO(jesus): Send the player name to the server
		OutputMemoryStream packet;
		packet << ClientMessage::Hello;
		packet << playerName;

		if (sendPacket(packet, s))
			state = ClientState::Logging;
		
		else
		{
			disconnect();
			state = ClientState::Stopped;
		}
	}

	return true;
}

bool ModuleNetworkingClient::gui()
{
	if (state != ClientState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Client Window");
		Texture *tex = App->modResources->client;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("Hello %s! Welcome to the chat.", playerName.c_str());
		
		ImGui::SameLine();
		
		if (ImGui::Button("Logout"))
		{
			disconnect();
			state = ClientState::Stopped;
		}

		ImGui::Spacing();
		
		ImGui::BeginChild("Scroll", ImVec2(400, 450), true);

		ImGui::TextColored(ImVec4(1, 1, 0, 1), "  **************************************************");
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "                WELCOME TO THE CHAT                 ");
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "   Please type /help to see the available commands. ");
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "  **************************************************");

		for (const auto& message : messagesList)
		{
			ImGui::TextColored(message.color, message.msg.c_str());
		}

		ImGui::EndChild();

		static char text[256] = "";
		if (ImGui::InputText("Line", text, IM_ARRAYSIZE(text), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			OutputMemoryStream packet;
			packet << ClientMessage::Line;
			packet << text;

			if (!sendPacket(packet, s))
			{
				disconnect();
				state = ClientState::Stopped;
			}

			strcpy_s(text, 256, "");
		}

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	ServerMessage serverMessage;
	packet >> serverMessage;

	switch (serverMessage)
	{
	case ServerMessage::Help:
	case ServerMessage::List:
	{
		std::string msg;
		packet >> msg;

		messagesList.push_back(Message(msg, ImVec4(1, 1, 0, 1)));
		break;
	}
	case ServerMessage::Ban:
	{
		std::string msg;
		packet >> msg;

		messagesList.push_back(Message(msg, ImVec4(1, 0, 0, 1)));
		break;
	}
	case ServerMessage::Whisper:
	case ServerMessage::Unknown:
	case ServerMessage::Disconnected:
	{
		std::string msg;
		packet >> msg;

		messagesList.push_back(Message(msg, ImVec4(0.5, 0.5, 0.5, 1)));
		break;
	}
	case ServerMessage::ChangeName:
	{
		std::string msg;
		packet >> msg;

		std::string line = "Name changed to: ";
		std::string newPlayerName = msg.substr(line.length());

		playerName = newPlayerName;

		messagesList.push_back(Message(msg, ImVec4(1, 1, 1, 1)));
		break;
	}
	case ServerMessage::Chat:
	{
		std::string msg;
		packet >> msg;

		messagesList.push_back(Message(msg, ImVec4(1, 1, 1, 1)));
		break;
	}
	case ServerMessage::Connected:
	{
		std::string msg;
		packet >> msg;

		messagesList.push_back(Message(msg, ImVec4(0, 1, 0, 1)));
		break;
	}
	case ServerMessage::Remove:
	{
		disconnect();
		state = ClientState::Stopped;

		break;
	}
	case ServerMessage::Clear:
	{
		messagesList.clear();
		break;
	}
	case ServerMessage::Unban:
	{
		std::string msg;
		packet >> msg;

		messagesList.push_back(Message(msg, ImVec4(0, 0.5, 0, 1)));
		break;
	}
	default:
		break;
	}
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}

