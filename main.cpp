#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QUuid>
#include <QByteArray>
#include <QBuffer>
#include <QByteArray>
#include <QDataStream>

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <cstring>

#include <librdkafka/rdkafkacpp.h>
#include <json/json.h>
#include <SBReadFile.h>

class ExampleDeliveryReportCb : public RdKafka::DeliveryReportCb {
 public:
  void dr_cb (RdKafka::Message &message) {
    std::cout << "Message delivery for (" << message.len() << " bytes): " << message.errstr();
    if (message.key())
      std::cout << "Key: " << *(message.key()) << ";" << std::endl;
  }
};

class sld_kafka_exception : public std::exception
{
    using std::exception::exception;
};


QByteArray GetExperimentMetaData(std::string message){
    QByteArray byteArray;
    QDataStream out(&byteArray, QIODevice::ReadWrite);
    // this one doesnt seem to have any effect
    // set the java side to little endian - things are ok
    //out.setByteOrder(QDataStream::BigEndian);
    //version
    qint16 version = 0;
    out.writeRawData((const char*)&version, sizeof(version));

    // message type = 0 - meta data for experiment
    qint16 messageType = 0;
    out.writeRawData((const char*)&messageType, sizeof(messageType));

    qint32 messageSize = message.length();
    out.writeRawData((const char*)&messageSize, sizeof(messageSize));

    out.writeRawData(message.c_str(), messageSize);
    return byteArray;
}

/** time-point meta data
 * @brief GetFrameMetaData
 * @param message
 * @param messageId
 * @return
 */
QByteArray GetFrameMetaData(std::string message, std::string expId){
    QByteArray byteArray;
    QDataStream out(&byteArray, QIODevice::ReadWrite);
    // this one doesnt seem to have any effect
    // set the java side to little endian - things are ok
    //out.setByteOrder(QDataStream::BigEndian);
    //version
    qint16 version = 0;
    out.writeRawData((const char*)&version, sizeof(version));

    // message type = 1 - meta data for slices
    qint16 messageType = 1;
    out.writeRawData((const char*)&messageType, sizeof(messageType));

    //
    out.writeRawData(expId.c_str(), expId.length());

    qint32 messageSize = message.length();
    out.writeRawData((const char*)&messageSize, sizeof(messageSize));

    out.writeRawData(message.c_str(), messageSize);
    return byteArray;
}


QByteArray GetDataBuffer(std::string frameId,
                      qint32 sequenceNumber,
                      qint32 numberOfSequences,
                      UInt16* payload,
                      qint32 payloadSize){
    QByteArray byteArray;
    QDataStream out(&byteArray, QIODevice::ReadWrite);

    qint16 version = 0;
    out.writeRawData((const char*)&version, sizeof(version));

    //mesage type = 2 - real data of frame pieces
    qint16 messageType = 2;
    out.writeRawData((const char*)&messageType, sizeof(messageType));

    //
    out.writeRawData(frameId.c_str(), frameId.length());

    out.writeRawData((const char*)&sequenceNumber, sizeof(sequenceNumber));

    out.writeRawData((const char*)&numberOfSequences, sizeof(numberOfSequences));

    out.writeRawData((const char*)&payloadSize, sizeof(payloadSize));

    out.writeRawData((const char*)payload, payloadSize * sizeof(UInt16));

    return byteArray;
}

std::string GetImageName(III::SBReadFile * sb_read_file, int capture_index){
    std::string sb_data;
    int char_count = sb_read_file->GetImageName(NULL, capture_index);
    if (char_count > 0)
    {
        char * cbuff = new char[char_count];
        sb_read_file->GetImageName(cbuff, capture_index);
        sb_data = std::string(cbuff);
        delete[] cbuff;
    }
    return sb_data;
}

std::string GetImageComment(III::SBReadFile * sb_read_file, int capture_index){
    std::string sb_data;
    int char_count = sb_read_file->GetImageComments(NULL, capture_index);
    if (char_count > 0)
    {
        char * cbuff = new char[char_count];
        sb_read_file->GetImageComments(cbuff, capture_index);
        sb_data = std::string(cbuff);
        delete[] cbuff;
    }
    return sb_data;
}

QUuid sendExperimentMessage(RdKafka::Producer* producer,
                           RdKafka::Topic *topic,
                           int32_t partition,
                           SInt32 number_captures){
    Json::Value metaDataMsg;
    metaDataMsg["number_captures"] = number_captures;
    metaDataMsg["NA"] = 1.4;
    metaDataMsg["RI"] = 1.4;
    metaDataMsg["ns"] = 1.4;
    metaDataMsg["swapZT"] = true;
    QUuid expId = QUuid::createUuid();
    metaDataMsg["id"] = expId.toString().toStdString();
    //more
    Json::FastWriter fast;
    std::string metaDataStr = fast.write(metaDataMsg);
    QByteArray metaDataArray = GetExperimentMetaData( metaDataStr );
    int messageSize = metaDataArray.size();
    RdKafka::ErrorCode resp = producer->produce(topic, partition, RdKafka::Producer::RK_MSG_COPY,
                                                (void*)metaDataArray.data(), messageSize, NULL, NULL);
    if (resp != RdKafka::ERR_NO_ERROR){
        qDebug() << "% Produce failed: " << RdKafka::err2str(resp).c_str();
        throw sld_kafka_exception();
    }
    else{
        qDebug() << "% Produced message";
    }
    return expId;
}

QUuid sendFrameMessage(RdKafka::Producer* producer, RdKafka::Topic *topic,
                       int32_t partition, std::string expId,
                       std::string imageName, std::string imageComments,
                       SInt32 number_timepoints, SInt32 number_channels,
                       SInt32 xDim, SInt32 yDim, SInt32 zDim,
                       float voxel_size[], int number_of_chunks, int timepoint_index
                       )
{
    Json::Value metaDataMsg;
    metaDataMsg["name"] = imageName;
    metaDataMsg["comments"] = imageComments;
    metaDataMsg["number_timepoints"] = number_timepoints;
    metaDataMsg["number_channels"] = number_channels;
    metaDataMsg["xDim"] = xDim;
    metaDataMsg["yDim"] = yDim;
    metaDataMsg["zDim"] = zDim;
    metaDataMsg["voxel_size"] = voxel_size;
    metaDataMsg["number_of_chunks"] = number_of_chunks;
    metaDataMsg["timepoint_index"] = timepoint_index;
    QUuid frameId = QUuid::createUuid();
    metaDataMsg["frameid"] = frameId.toString().toStdString();
    //more
    Json::FastWriter fast;
    std::string metaDataStr = fast.write(metaDataMsg);
    QByteArray metaDataArray = GetFrameMetaData( metaDataStr, expId);
    int messageSize = metaDataArray.size();
    RdKafka::ErrorCode resp = producer->produce(topic, partition, RdKafka::Producer::RK_MSG_COPY,
                                                (void*)metaDataArray.data(), messageSize, NULL, NULL);
    if (resp != RdKafka::ERR_NO_ERROR){
        qDebug() << "% Produce failed: " << RdKafka::err2str(resp).c_str();
        throw sld_kafka_exception();
    }
    else{
        qDebug() << "% Produced message";
    }
    return frameId;
}

void sendFrameChunk(RdKafka::Producer* producer, RdKafka::Topic *topic,
                     int32_t partition, std::string frameId,
                     qint32 sequenceNumber, qint32 numberOfSequences,
                     UInt16* payload, qint32 payloadSizeInByte){

    QByteArray dataArray = GetDataBuffer(frameId, sequenceNumber, numberOfSequences, payload, payloadSizeInByte);
    int messageSize = dataArray.size();
    RdKafka::ErrorCode resp = producer->produce(topic, partition, RdKafka::Producer::RK_MSG_COPY,
                                                (void*)dataArray.data(), messageSize, NULL, NULL);
    if (resp != RdKafka::ERR_NO_ERROR){
        qDebug() << "% Failed to send frame data: " << RdKafka::err2str(resp).c_str();
        throw sld_kafka_exception();
    }
    else{
        qDebug() << "% Successfully send frame data";
    }
}

const int max_message_bytes = 10485760; // 10 Mbs
const int message_overhead = 2 + 2 + 38 + 12; // version - msgtype - messageid - sequenceNum - numOfSefments -msgSize

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("TestSlidebookKafka");
    QCoreApplication::setApplicationVersion("1.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("Test helper");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "Slidebook file."));
    parser.process(a);
    const QStringList args = parser.positionalArguments();
    /**** KAFKA Stuff *********/
    ///////////////////////////////////////
    std::string brokers = "localhost";
    std::string errstr;
    std::string topic_str = "test";
    int32_t partition = RdKafka::Topic::PARTITION_UA;
    /*
    * Create configuration objects
    */
    RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    RdKafka::Conf *tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
    // compression
    // none|gzip|snappy
    std::string compression = "snappy";
    if(conf->set("compression.codec", compression, errstr) != RdKafka::Conf::CONF_OK){
        std::cerr << "Cannot set compression " << errstr << std::endl;
        exit(1);
    }
    // brokers
    conf->set("metadata.broker.list", brokers, errstr);

    // delivery reports
    ExampleDeliveryReportCb ex_dr_cb;
    /* Set delivery report callback */
    conf->set("dr_cb", &ex_dr_cb, errstr);


    RdKafka::Producer *producer = RdKafka::Producer::create(conf, errstr);
    if (!producer) {
      std::cerr << "Failed to create producer: " << errstr << std::endl;
      exit(1);
    }
    std::cout << "% Created producer " << producer->name();

    tconf->set("max_message_bytes", "10485760", errstr);
    RdKafka::Topic *topic = RdKafka::Topic::create(producer, topic_str, tconf, errstr);
    if (!topic) {
      std::cerr << "Failed to create topic: " << errstr << std::endl;
      exit(1);
    }
    /**************************/

    ///////////////////////////////////////
    QString file= QString("/home/hoangnguyen177/Desktop/working/Streaming/slidebook_files/Slide4.sld");
    try{
        III::SBReadFile* sb_read_file = III_NewSBReadFile(file.toStdString().c_str(), III::kNoExceptionsMasked);
        CaptureIndex number_captures = sb_read_file->GetNumCaptures();
        // send metadata about the experiment
        QUuid expId = sendExperimentMessage(producer, topic, partition, number_captures);
        for (int capture_index = 0; capture_index < number_captures; capture_index++)
        {
            PositionIndex number_channels = sb_read_file->GetNumChannels(capture_index);
            ChannelIndex number_positions = sb_read_file->GetNumPositions(capture_index);
            TimepointIndex number_timepoints = sb_read_file->GetNumTimepoints(capture_index);

            SInt32 xDim = sb_read_file->GetNumXColumns(capture_index);
            SInt32 yDim = sb_read_file->GetNumYRows(capture_index);
            SInt32 zDim = sb_read_file->GetNumZPlanes(capture_index);

            bool has_voxel_size = false;
            float voxel_size[3];
            has_voxel_size = sb_read_file->GetVoxelSize(capture_index, voxel_size[0], voxel_size[1], voxel_size[2]);
            if (!has_voxel_size)
            {
                voxel_size[0] = voxel_size[1] = voxel_size[2] = 1.0;
            }

            std::string imageName = GetImageName(sb_read_file, capture_index);
            std::string imageComments = GetImageComment(sb_read_file, capture_index);

            int bufferSize = xDim * yDim * zDim;
            int planeSize = xDim * yDim;
            int bufferSizeInBytes = bufferSize*sizeof(UInt16);
            UInt16* buffer = new UInt16[bufferSize];
            // read in buffer now
            for (int timepoint_index = 0;  timepoint_index < number_timepoints; timepoint_index++)
            {
                //////////////////// splitting
                int max_chunk_size = max_message_bytes - message_overhead;
                int number_of_chunks = bufferSizeInBytes/max_chunk_size;
                int some_more = bufferSizeInBytes% (max_chunk_size);
                if(some_more > 0)
                    number_of_chunks += 1;
                QUuid frameId = sendFrameMessage(producer, topic, partition, expId.toString().toStdString(),
                                 imageName, imageComments, number_timepoints, number_channels,
                                 xDim, yDim, zDim, voxel_size, number_of_chunks, timepoint_index);
                // read the data of this frame into buffer
                for (int channel = 0; channel < number_channels; channel++)
                {
                    for (int z = 0; z < zDim; z++)
                    {
                        sb_read_file->ReadImagePlaneBuf(buffer + (z * planeSize),
                                                        capture_index, 0, timepoint_index, z, channel);
                    }
                }

                // send out chunks
                int seqIndex = 0;
                while(seqIndex < number_of_chunks){
                    int chunkSizeInBytes = max_chunk_size;
                    if(seqIndex == number_of_chunks - 1){
                        chunkSizeInBytes = bufferSizeInBytes - seqIndex*max_chunk_size;
                    }
                    sendFrameChunk(producer, topic, partition, frameId.toString().toStdString(),
                                   seqIndex, number_of_chunks,
                                   buffer + seqIndex * max_chunk_size, chunkSizeInBytes);
                    seqIndex++;
                }
            }

            producer->poll(0);
            // then send the data
        }
    }
    catch(sld_kafka_exception* e){
        qDebug() << "Failed with kafka slidebook exception";
        exit(1);
        delete e;
    }

    catch (const III::Exception * e)
    {
        qDebug() << "Failed with exception" << e->GetDescription();
        exit(1);
        delete e;
    }


    return a.exec();
}

