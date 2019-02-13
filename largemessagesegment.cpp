#include "largemessagesegment.h"


LargeMessageSegment::LargeMessageSegment(QUuid msgId, int seqNumber, int numOfSeg, int msgSize, void*data)
{
    this->messageId = msgId;
    this->sequenceNumber = seqNumber;
    this->numberOfSegment = numOfSeg;
    this->messageSizeInBytes = msgSize;
    this->payload = data;
}

