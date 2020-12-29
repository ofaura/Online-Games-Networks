#include "Networks.h"
#include "ReplicationManagerServer.h"

// TODO(you): World state replication lab session

void ReplicationManagerServer::Create(uint32 networkID)
{
	repCommands.push_back(ReplicationCommand(ReplicationAction::Create, networkID));
}

void ReplicationManagerServer::Update(uint32 networkID)
{
	repCommands.push_back(ReplicationCommand(ReplicationAction::Update, networkID));
}

void ReplicationManagerServer::Destroy(uint32 networkID)
{
    repCommands.insert(repCommands.begin(), ReplicationCommand(ReplicationAction::Destroy, networkID));
}

void ReplicationManagerServer::Write(OutputMemoryStream& packet, ReplicationManagerDelivery* delivery)
{
    for (const auto& replicationCommand : repCommands) 
    {
        uint32 networkID = replicationCommand.networkID;
        ReplicationAction repAction = replicationCommand.action;
        GameObject* gameObject = App->modLinkingContext->getNetworkGameObject(networkID);

        packet << networkID;
        packet << repAction;

        switch (repAction)
        {
            case ReplicationAction::Create: 
            {
                if (gameObject != nullptr)
                    gameObject->WriteData(packet);

                break;
            }

            case ReplicationAction::Update: 
            {
                if (gameObject != nullptr)
                {
                    packet << gameObject->position.x;
                    packet << gameObject->position.y;
                    packet << gameObject->angle;

                    gameObject->behaviour->write(packet);                 
                }

                break;
            }

            case ReplicationAction::Destroy: 
            {
                if (gameObject != nullptr)
                {
                    App->modLinkingContext->unregisterNetworkGameObject(gameObject);
                    ModuleGameObject::Destroy(gameObject);      
                }

                break;
            }

            default: 
            {
                break;
            }  
        }

        delivery->AddReplicationCommand(replicationCommand);
    }

    repCommands.clear();
}

ReplicationManagerDelivery::ReplicationManagerDelivery(ReplicationManagerServer* repManagerServer) : replicationManager(repManagerServer)
{
}

ReplicationManagerDelivery::~ReplicationManagerDelivery() {}

void ReplicationManagerDelivery::onDeliverySuccess(DeliveryManager* deliveryManager)
{
}

void ReplicationManagerDelivery::onDeliveryFailure(DeliveryManager* deliveryManager)
{
    for (const auto& replicationCommand : replicationCommands) 
    {
        switch (replicationCommand.action)
        {
            case ReplicationAction::Create:
            {
                if (App->modLinkingContext->getNetworkGameObject(replicationCommand.networkID) != nullptr)
                {
                    replicationManager->Create(replicationCommand.networkID);
                }
                break;
            }

            case ReplicationAction::Update:
            {
                if (App->modLinkingContext->getNetworkGameObject(replicationCommand.networkID) != nullptr)
                {
                    replicationManager->Update(replicationCommand.networkID);
                }
                break;
            }

            case ReplicationAction::Destroy:
            {

                if (App->modLinkingContext->getNetworkGameObject(replicationCommand.networkID) != nullptr)
                {
                    replicationManager->Destroy(replicationCommand.networkID);
                }
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

void ReplicationManagerDelivery::AddReplicationCommand(const ReplicationCommand& replicationCommand)
{
    replicationCommands.push_back(replicationCommand);
}
