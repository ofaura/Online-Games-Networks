// TODO(you): Reliability on top of UDP lab session

#include "DeliveryManager.h"
#include "Networks.h"

Delivery* DeliveryManager::writeSequenceNumber(OutputMemoryStream& packet)
{
	Delivery* delivery = new Delivery();
	delivery->sequenceNumber = nextOutgoingSequenceNumber++;
	delivery->dispatchTime = Time.time;

	packet << delivery->sequenceNumber;

	pendingDeliveries.push_back(delivery);

	return delivery;
}

bool DeliveryManager::processSequenceNumber(const InputMemoryStream& packet)
{
	uint32 sequenceNumber;
	packet >> sequenceNumber;

	if (sequenceNumber == nextExpectedSequenceNumber)
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
		packet.Write(size);
		packet.Write(sequenceNumbersPendingAck.front());
		sequenceNumbersPendingAck.clear();
	}
}

void DeliveryManager::processAcks(const InputMemoryStream& packet)
{
	uint32 seqNumber;
	uint32 size;
	packet.Read(size);
	packet.Read(seqNumber);

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

		if (Time.time - delivery->dispatchTime >= PACKET_DELIVERY_TIMEOUT_SECONDS)
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