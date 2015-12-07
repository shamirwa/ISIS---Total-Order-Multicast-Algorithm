#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <string.h>
#include <netdb.h>
#include <map>
#include <vector>
#include <set>
#include <unistd.h>
#include <math.h>
#include "message.h"
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <sys/time.h>
#include <signal.h>

using namespace std;

#define errorDebug 0
#define paramDebug 0
#define ACK_TIMER 100
#define DATAMSG_TYPE 1
#define ACKMSG_TYPE 2
#define SEQMSG_TYPE 3
#define XINU 0
#define VM 1
#define DUMMY 1
#define ALARMTIME 5


typedef struct{
    DataMessage* myMsg;
    uint32_t myseqNum;
    bool isDeliverable;
}msgInfo;


// Structure to keep the common information between sender and receiver side.
typedef struct{
    vector<string> hostNames;
    vector<string> hostIPAddr;
    map<string, string> hostNameToIP;
    map<string, uint32_t> ipToID;

    // config parameters
    string hostFileName;
    int totalNumProcess;
    int numOfMsgToSend;

    // Self information
    string selfHostName;
    uint32_t selfID;
    uint32_t lastPropSeqNum;
    uint32_t lastFinalSeqNum;
}generalSystemInfo;


// Sender Information Struct
typedef struct{
    set<int> expectAckFrom;
    set<int>::iterator ackIter;
    int mySocket;
    int msgNumberSent;
    uint32_t seqNumberOfSelfMsg;

}senderInfo;

// Receiver Information struct
typedef struct{
    long int portNum;
    int mySocket;
    map< pair<uint32_t, uint32_t>, msgInfo* > priorityToMsgList;
    map< pair<uint32_t, uint32_t>, msgInfo* >::iterator iter1;
    map<uint32_t, set<uint32_t> > procToDelvMsgList;
    map<uint32_t, set<uint32_t> >::iterator iter2; 
    multiset<uint32_t> ackRecvdSeqNum;
}receiverInfo;

// GLOBAL VARIABLES
generalSystemInfo mySystemInfo;
senderInfo sender;
receiverInfo receiver;

// Function to use all the error message
void error(const char msg[]);

// Function to print the help menu
void printUsage();

// Function to validate the port number passed
bool verifyPortNumber(long portNumber);

// Function to validate the hostfile passed
bool verifyHostFileName(string fileName);

// Function to validate the number of faulty process
bool verifyCount(int count);

// Function to send the Final Sequence Message
void sendFinalSequenceNumberMsg(generalSystemInfo& systemInfo, senderInfo& mySenderSide,
                             receiverInfo& myReceiverSide);

// Function to verify and deliver the messages in the queue
void checkAndDeliverMsg(generalSystemInfo& systemInfo, senderInfo& mySenderSide,
                             receiverInfo& myReceiverSide);

void convertByteOrder(DataMessage* dataMessage, bool hton);

void convertAckByteOrder(AckMessage* ackMessage, bool hton);

void convertSeqByteOrder(SeqMessage* seqMessage, bool hton);

bool checkIfDelivered(DataMessage* message, receiverInfo myReceiverSide);

bool checkIfQueued(DataMessage* message, receiverInfo myReceiverSide);

void storeMessage(DataMessage* message, receiverInfo& myReceiverSide, generalSystemInfo systemInfo);

// Function to send the ACK message to the sender
void sendAckMessage(uint32_t msgID, string receiverIP, generalSystemInfo& systemInfo, int portNum, 
                    senderInfo &mySenderSide);

void updateFinalSeqNum(uint32_t senderId, uint32_t msgID, uint32_t finalSeqNum, generalSystemInfo& systemInfo, receiverInfo& myReceiverSide);

bool verifyAckMessage(AckMessage* message, generalSystemInfo systemInfo, senderInfo mySenderSide);

void handleTimerExpiry(int sig);

void printMessage(void* msgToPrint, int type);
/*
// Return the index of the process from which ack is received
double getTimeDiff(struct timeval *start, struct timeval* end, bool print=false);
*/
