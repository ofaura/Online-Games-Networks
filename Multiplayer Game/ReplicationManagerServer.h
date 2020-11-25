#pragma once

// TODO(you): World state replication lab session
// TODO(you): World state replication lab session
enum class ReplicationAction
{
	None, Create, Update, Destroy
};

struct ReplicationCommand
{
	ReplicationCommand(ReplicationAction _action, uint32 _networkID) : action(_action), networkID(_networkID) {}

	ReplicationAction action;
	uint32 networkID;
};

class ReplicationManagerServer
{
public:

	void Create(uint32 networkID);
	void Update(uint32 networkID);
	void Destroy(uint32 networkID);

	void Write(OutputMemoryStream& packet);

public:

	std::vector<ReplicationCommand> repCommands;
};