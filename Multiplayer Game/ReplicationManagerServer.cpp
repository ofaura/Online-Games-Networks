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

void ReplicationManagerServer::Write(OutputMemoryStream& packet)
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
    }

    repCommands.clear();
}
