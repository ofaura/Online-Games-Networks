#include "Networks.h"
#include "ModuleGameObject.h"

bool ModuleGameObject::init()
{
	return true;
}

bool ModuleGameObject::preUpdate()
{
	BEGIN_TIMED_BLOCK(GOPreUpdate);

	static const GameObject::State gNextState[] = {
		GameObject::NON_EXISTING, // After NON_EXISTING
		GameObject::STARTING,     // After INSTANTIATE
		GameObject::UPDATING,     // After STARTING
		GameObject::UPDATING,     // After UPDATING
		GameObject::DESTROYING,   // After DESTROY
		GameObject::NON_EXISTING  // After DESTROYING
	};

	for (GameObject &gameObject : gameObjects)
	{
		gameObject.state = gNextState[gameObject.state];
	}

	END_TIMED_BLOCK(GOPreUpdate);

	return true;
}

bool ModuleGameObject::update()
{
	// Delayed destructions
	for (DelayedDestroyEntry &destroyEntry : gameObjectsWithDelayedDestruction)
	{
		if (destroyEntry.object != nullptr)
		{
			destroyEntry.delaySeconds -= Time.deltaTime;
			if (destroyEntry.delaySeconds <= 0.0f)
			{
				Destroy(destroyEntry.object);
				destroyEntry.object = nullptr;
			}
		}
	}

	return true;
}

bool ModuleGameObject::postUpdate()
{
	return true;
}

bool ModuleGameObject::cleanUp()
{
	return true;
}

GameObject * ModuleGameObject::Instantiate()
{
	for (uint32 i = 0; i < MAX_GAME_OBJECTS; ++i)
	{
		GameObject &gameObject = App->modGameObject->gameObjects[i];

		if (gameObject.state == GameObject::NON_EXISTING)
		{
			gameObject = GameObject();
			gameObject.id = i;
			gameObject.state = GameObject::INSTANTIATE;
			return &gameObject;
		}
	}

	ASSERT(0); // NOTE(jesus): You need to increase MAX_GAME_OBJECTS in case this assert crashes
	return nullptr;
}

void ModuleGameObject::Destroy(GameObject * gameObject)
{
	ASSERT(gameObject->networkId == 0); // NOTE(jesus): If it has a network identity, it must be destroyed by the Networking module first

	static const GameObject::State gNextState[] = 
	{
		GameObject::NON_EXISTING, // After NON_EXISTING
		GameObject::DESTROY,      // After INSTANTIATE
		GameObject::DESTROY,      // After STARTING
		GameObject::DESTROY,      // After UPDATING
		GameObject::DESTROY,      // After DESTROY
		GameObject::DESTROYING    // After DESTROYING
	};

	ASSERT(gameObject->state < GameObject::STATE_COUNT);
	gameObject->state = gNextState[gameObject->state];
}

void ModuleGameObject::Destroy(GameObject * gameObject, float delaySeconds)
{
	for (uint32 i = 0; i < MAX_GAME_OBJECTS; ++i)
	{
		if (App->modGameObject->gameObjectsWithDelayedDestruction[i].object == nullptr)
		{
			App->modGameObject->gameObjectsWithDelayedDestruction[i].object = gameObject;
			App->modGameObject->gameObjectsWithDelayedDestruction[i].delaySeconds = delaySeconds;
			break;
		}
	}
}

GameObject * Instantiate()
{
	GameObject *result = ModuleGameObject::Instantiate();
	return result;
}

void Destroy(GameObject * gameObject)
{
	ModuleGameObject::Destroy(gameObject);
}

void Destroy(GameObject * gameObject, float delaySeconds)
{
	ModuleGameObject::Destroy(gameObject, delaySeconds);
}

void GameObject::WriteData(OutputMemoryStream& packet)
{
	packet << position.x;
	packet << position.y;
	packet << size.x;
	packet << size.y;
	packet << angle;
	
	packet << sprite->pivot.x;
	packet << sprite->pivot.y;
	packet << sprite->color.r;
	packet << sprite->color.g;
	packet << sprite->color.b;
	packet << sprite->color.a;
		
	//packet << sprite->texture->filename;
	std::string fileName = sprite->texture->filename;
	packet.Write(fileName);

	packet << sprite->order;
	
	if (sprite->texture->filename != "explosion1.png")
	{
		ColliderType colliderType = collider != nullptr ? collider->type : ColliderType::None;
		packet << colliderType;
	
		packet << collider->isTrigger;	
	}

	packet << tag;
	packet << kills;
}

void GameObject::ReadData(const InputMemoryStream& packet)
{
	packet >> position.x;
	packet >> position.y;
	packet >> size.x;
	packet >> size.y;
	packet >> angle;

	sprite = App->modRender->addSprite(this);
	
	packet >> sprite->pivot.x;
	packet >> sprite->pivot.y;
	packet >> sprite->color.r;
	packet >> sprite->color.g;
	packet >> sprite->color.b;
	packet >> sprite->color.a;
		
	std::string fileName;
	packet.Read(fileName);

	if (fileName == "space_background.jpg") 
		sprite->texture = App->modResources->space;
			
	else if (fileName == "asteroid1.png") 
		sprite->texture = App->modResources->asteroid1;
			
	else if (fileName == "asteroid2.png") 
		sprite->texture = App->modResources->asteroid2;
			
	else if (fileName == "spacecraft1.png") 
		sprite->texture = App->modResources->spacecraft1;
			
	else if (fileName == "spacecraft2.png") 
		sprite->texture = App->modResources->spacecraft2;
			
	else if (fileName == "spacecraft3.png") 
		sprite->texture = App->modResources->spacecraft3;
			
	else if (fileName == "laser.png") 
		sprite->texture = App->modResources->laser;
			
	else if (fileName == "explosion1.png") 
		sprite->texture = App->modResources->explosion1;		
		
	packet >> sprite->order;
	
	if (sprite->texture->filename != "explosion1.png")
	{
		ColliderType type = ColliderType::None;
		packet >> type;
		collider = App->modCollision->addCollider(type, this);
		packet >> collider->isTrigger;

		if (collider->type == ColliderType::Laser)
		{
			Laser* laserBehaviour = App->modBehaviour->addLaser(this);
			behaviour = laserBehaviour;
		}

		else if (collider->type == ColliderType::Player)
		{
			Spaceship* playerBehaviour = App->modBehaviour->addSpaceship(this);
			behaviour = playerBehaviour;
		}
	}

	else
	{
		animation = App->modRender->addAnimation(this);
		animation->clip = App->modResources->explosionClip;
	}

	packet >> tag;
	packet >> kills;
}
