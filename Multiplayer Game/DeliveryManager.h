#pragma once

// TODO(you): Reliability on top of UDP lab session

class DeliveryManager;

class DeliveryDelegate
{
public:
	virtual void onDeliverySuccess(DeliveryManager* deliveryManager) = 0;
	virtual void onDeliveryFailure(DeliveryManager* deliveryManager) = 0;
};

struct Delivery
{
	Delivery() {};
	~Delivery() { CleanUp(); }

	uint32 sequenceNumber = 0;
	double dispatchTime = 0.0;
	DeliveryDelegate* delegate = nullptr;

	void CleanUp();
};

class DeliveryManager
{
public:
	// For senders to write a new seq. numbers into a packet
	Delivery* writeSequenceNumber(OutputMemoryStream& packet);

	// For receivers to process the seq. number from an incoming packet
	bool processSequenceNumber(const InputMemoryStream& packet);

	// For receivers to write ack'ed seq. numbers into a packet
	bool hasPendingAcks() const;
	void writePendingAcks(OutputMemoryStream& packet);

	// For senders to process ack'ed seq numbers from a packet
	void processAcks(const InputMemoryStream& packet);
	void processTimedOutPackets();

	void clear();

private:
	// Sender side
	uint32 nextOutgoingSequenceNumber = 0; 			// The next outgoing sequence number
	std::deque<Delivery*> pendingDeliveries;		// A list of pending deliveries

	// Receiver side
	uint32 nextExpectedSequenceNumber = 0;			// The next expected sequence number
	std::deque<uint32> sequenceNumbersPendingAck;	// A list of sequence numbers pending ack
};