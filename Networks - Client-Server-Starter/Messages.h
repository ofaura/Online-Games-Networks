#pragma once

// Add as many messages as you need depending on the
// functionalities that you decide to implement.

enum class ClientMessage
{
	Hello,
	Line
};

enum class ServerMessage
{
	Welcome,
	Help,
	List,
	Whisper,
	ChangeName,
	Unknown,
	Chat,
	Connected,
	Disconnected,
	Remove,
	Mute,
	UnMute,
	Clear,
	Ban,
	Unban,
	Error,
	NonWelcome
};

