#ifndef LARGEMESSAGESEGMENT_H
#define LARGEMESSAGESEGMENT_H

#include <QUuid>
#include "fmt/format.h"
class LargeMessageSegment
{
private:
    QUuid messageId;
    int sequenceNumber;
    int numberOfSegment;
    int messageSizeInBytes;
    void* payload;
public:
    LargeMessageSegment(QUuid msgId, int seqNumber, int numOfSeg, int msgSize, void*data);
};

#endif // LARGEMESSAGESEGMENT_H
