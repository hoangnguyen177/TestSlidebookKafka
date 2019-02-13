#include <QCoreApplication>
#include <QCommandLineParser>
#include "slidebook.h"
#include "fmt/format.h"
#include <QDebug>
#include <QUuid>
#include <QByteArray>
#include <QBuffer>
#include <QByteArray>
#include <QDataStream>

#include "json/json.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <cstring>

#include <librdkafka/rdkafkacpp.h>

class ExampleDeliveryReportCb : public RdKafka::DeliveryReportCb {
 public:
  void dr_cb (RdKafka::Message &message) {
    std::cout << "Message delivery for (" << message.len() << " bytes): " << message.errstr();
    if (message.key())
      std::cout << "Key: " << *(message.key()) << ";" << std::endl;
  }
};


QByteArray getMetaDataBuffer(QString message){
    QByteArray byteArray;
    QDataStream out(&byteArray, QIODevice::ReadWrite);
    // this one doesnt seem to have any effect
    // set the java side to little endian - things are ok
    //out.setByteOrder(QDataStream::BigEndian);
    //version
    qint16 version = 0;
    out.writeRawData((const char*)&version, sizeof(version));

    //
    qint16 messageType = 0;
    out.writeRawData((const char*)&messageType, sizeof(messageType));

    //
    QUuid msgId = QUuid::createUuid();
    std::string msgStr = msgId.toString().toStdString();
    out.writeRawData(msgStr.c_str(), msgStr.length());

    qint32 sequenceNumber =0;
    out.writeRawData((const char*)&sequenceNumber, sizeof(sequenceNumber));

    qint32 numberOfSequences =0;
    out.writeRawData((const char*)&numberOfSequences, sizeof(numberOfSequences));

    qint32 messageSize = message.length();
    out.writeRawData((const char*)&messageSize, sizeof(messageSize));

    std::string messageStr = message.toStdString();
    out.writeRawData(messageStr.c_str(), messageStr.length());

    return byteArray;
}



//void* getDataMsg(void* payload, int payloadSize){
//    int message_overhead = 2 + 2 + 38 + 12;
//    // create meta data message
//    void *buffer = malloc(sizeof(message) + message_overhead);
//    // version
//    char version = 0;
//    memcpy(buffer, &version, 1);
//    // message type 0 is meta data, 1 is data
//    char messageType = 0;
//    memcpy(buffer, &messageType, 1);
//    // message id
//    QUuid msgId = QUuid::createUuid();
//    const char* messageIdDat = msgId.toByteArray().data();
//    memcpy(buffer, messageIdDat, 16);
//    //sequence number
//    long sequenceNumber =0;
//    memcpy(buffer, &sequenceNumber, 4);
//    //number of sequence
//    long numberOfSequences =0;
//    memcpy(buffer, &numberOfSequences, 4);
//    // long messageSize
//    long messageSize = sizeof(message);
//    memcpy(buffer, &messageSize, 4);
//    // message
//    memcpy(buffer, message, messageSize);
//    return buffer;
//}

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
    int max_message_bytes = 209715200; // 200 Mbs
    int message_overhead = 2 + 2 + 38 + 12; // version - msgtype - messageid - sequenceNum - numOfSefments -msgSize
    int max_chunk_size = max_message_bytes - message_overhead;
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

    tconf->set("max_message_bytes", "209715200", errstr);
    RdKafka::Topic *topic = RdKafka::Topic::create(producer, topic_str, tconf, errstr);
    if (!topic) {
      std::cerr << "Failed to create topic: " << errstr << std::endl;
      exit(1);
    }
    /**************************/

    ///////////////////////////////////////
    //    for(QString argument: args){
    //        qDebug() << ">>>> Slidebook file:" << argument << "<<<<<";
    //        Slidebook slidebook = Slidebook(argument.toStdString());
    //        for (int capture_index = 0; capture_index < slidebook.number_captures; capture_index++)
    //        {
    //            CaptureDataFrame capture = slidebook.getDataFrame(capture_index);
    //        }
    //    }
    QString file= QString("/home/hoangnguyen177/Desktop/working/Streaming/slidebook_files/Slide4.sld");
    try{
        Slidebook slidebook = Slidebook(file.toStdString());
        qDebug()<< slidebook.number_captures;
        for (int capture_index = 0; capture_index < slidebook.number_captures; capture_index++)
        {
            CaptureDataFrame capture = slidebook.getDataFrame(capture_index);
            int number_of_chunks = capture.bufferSizeInBytes /max_chunk_size;
            int some_more = capture.bufferSizeInBytes % (max_chunk_size);
            if(some_more > 0)
                number_of_chunks += 1;
            // create the meta data first
            Json::Value metaDataMsg;
            metaDataMsg["filename"] = capture.image_name;
            metaDataMsg["number_of_chunks"] = number_of_chunks;
            metaDataMsg["number_of_captures"] = slidebook.number_captures;
            metaDataMsg["capture_index"] = capture_index;
            Json::FastWriter fast;
            std::string metaDataStr = fast.write(metaDataMsg);
            QByteArray byteArray = getMetaDataBuffer( QString::fromStdString(metaDataStr));
            int messageSize = byteArray.size();
            // create meta data message
            std::cout  << byteArray.toStdString() << std::endl;
            std::cout << "message size:" << messageSize
                      << "msg overhead:" << message_overhead
                      << " size of buffer:" << messageSize << std::endl;
            RdKafka::ErrorCode resp = producer->produce(topic, partition, RdKafka::Producer::RK_MSG_COPY,
                                                        (void*)byteArray.data(), messageSize, NULL, NULL);
            if (resp != RdKafka::ERR_NO_ERROR)
                std::cerr << "% Produce failed: " << RdKafka::err2str(resp) << std::endl;
            else
                std::cerr << "% Produced message" << std::endl;
            producer->poll(0);
            // then send the data
        }
    }
    catch (const III::Exception * e)
    {
        fmt::print("Failed with exception: {}\n", e->GetDescription());
        std::string inString;
        exit(1);
        delete e;
    }


    return a.exec();
}

