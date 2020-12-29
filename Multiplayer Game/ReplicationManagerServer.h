#pragma once

// TODO(you): World state replication lab session
// TODO(you): World state replication lab session
class ReplicationManagerDelivery;

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

	void Write(OutputMemoryStream& packet, ReplicationManagerDelivery* delivery);

public:

	std::vector<ReplicationCommand> repCommands;
};

class ReplicationManagerDelivery : public DeliveryDelegate
{
public:
	ReplicationManagerDelivery(ReplicationManagerServer* repManagerServer);
	~ReplicationManagerDelivery();

	void onDeliverySuccess(DeliveryManager* deliveryManager) override;
	void onDeliveryFailure(DeliveryManager* deliveryManager) override;

	void AddReplicationCommand(const ReplicationCommand& replicationCommand);


private:
	ReplicationManagerServer* replicationManager;
	std::vector<ReplicationCommand> replicationCommands;
};