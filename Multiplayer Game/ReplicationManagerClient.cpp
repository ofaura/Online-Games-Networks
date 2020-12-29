#include "Networks.h"
#include "ReplicationManagerClient.h"

// TODO(you): World state replication lab session

void ReplicationManagerClient::Read(const InputMemoryStream& packet)
{
	while (packet.RemainingByteCount() > 0)
	{
		uint32 networkID;
		ReplicationAction repAction = ReplicationAction::None;

		packet >> networkID;
		packet >> repAction;
		
		switch (repAction)
		{
			case ReplicationAction::Create:
			{
				GameObject* gameObject = App->modLinkingContext->getNetworkGameObject(networkID);
				
				if (gameObject == nullptr)
				{
					gameObject = App->modGameObject->Instantiate();
					App->modLinkingContext->registerNetworkGameObjectWithNetworkId(gameObject, networkID);
					gameObject->ReadData(packet);
				}

				break;
			}
			case ReplicationAction::Update:
			{
				GameObject* gameObject = App->modLinkingContext->getNetworkGameObject(networkID);
				if (gameObject != nullptr)
				{
					packet >> gameObject->position.x;
					packet >> gameObject->position.y;
					packet >> gameObject->angle;
					packet >> gameObject->tag;
					packet >> gameObject->kills;

					gameObject->behaviour->read(packet);
				}

				break;
			}
			case ReplicationAction::Destroy:
			{
				GameObject* gameObject = App->modLinkingContext->getNetworkGameObject(networkID);
				if (gameObject != nullptr)
				{
					App->modLinkingContext->unregisterNetworkGameObject(gameObject);
					Destroy(gameObject);
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
