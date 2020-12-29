// TODO(you): Reliability on top of UDP lab session

#include "DeliveryManager.h"
#include "Networks.h"

Delivery* DeliveryManager::writeSequenceNumber(OutputMemoryStream& packet)
{
	Delivery* delivery = new Delivery();
	delivery->sequenceNumber = nextOutgoingSequenceNumber++;
	delivery->dispatchTime = Time.time;

	pendingDeliveries.push_back(delivery);

	packet << delivery->sequenceNumber;

	return delivery;
}

bool DeliveryManager::processSequenceNumber(const InputMemoryStream& packet)
{
	uint32 sequenceNumber;
	packet >> sequenceNumber;

	if (sequenceNumber >= nextExpectedSequenceNumber)
	{
		sequenceNumbersPendingAck.push_back(sequenceNumber);
		nextExpectedSequenceNumber = sequenceNumber + 1;
		return true;
	}

	return false;
}

bool DeliveryManager::hasPendingAcks() const
{
	return !sequenceNumbersPendingAck.empty();
}

void DeliveryManager::writePendingAcks(OutputMemoryStream& packet)
{
	uint32 size = sequenceNumbersPendingAck.size();

	if (size > 0 && hasPendingAcks())
	{
		packet << size;
		packet << sequenceNumbersPendingAck.front();
		sequenceNumbersPendingAck.clear();
	}
}

void DeliveryManager::processAcks(const InputMemoryStream& packet)
{
	uint32 seqNumber;
	uint32 size;
	packet >> size;
	packet >> seqNumber;

	while (seqNumber < seqNumber + size
		&& !pendingDeliveries.empty())
	{
		Delivery* delivery = pendingDeliveries.front();

		if (delivery->sequenceNumber == seqNumber)
		{
			delivery->delegate->onDeliverySuccess(this);
			pendingDeliveries.erase(pendingDeliveries.begin());
			seqNumber++;
			delete delivery;
		}
		else if (delivery->sequenceNumber < seqNumber)
		{
			pendingDeliveries.erase(pendingDeliveries.begin());
			delivery->delegate->onDeliveryFailure(this);
			delete delivery;
		}
		else
		{
			seqNumber++;
		}
	}
}

void DeliveryManager::processTimedOutPackets()
{
	while (!pendingDeliveries.empty())
	{
		Delivery* delivery = pendingDeliveries.front();

		if (Time.time - delivery->dispatchTime >= 0.1f)
		{
			delivery->delegate->onDeliveryFailure(this);

			delivery->CleanUp();
			pendingDeliveries.pop_front();
		}
		else
		{
			break;
		}
	}
}

void DeliveryManager::clear()
{
	while (!pendingDeliveries.empty())
	{
		pendingDeliveries.front()->CleanUp();
		pendingDeliveries.pop_front();
	}

	pendingDeliveries.clear();
	sequenceNumbersPendingAck.clear();

	nextOutgoingSequenceNumber = 0;
	nextExpectedSequenceNumber = 0;
}

void Delivery::CleanUp()
{
	if (delegate != nullptr)
	{
		delete delegate;
		delegate = nullptr;
	}
}