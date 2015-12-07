#include "project2.h"
#define TEST 0

// Function to print the error messages
void error(const char msg[])
{
    if(errorDebug){
        fprintf(stderr,"Error: %s",msg);
    }
}

// Function to print the usage of the application
void printUsage(){
    printf("Usage: project2 -p port -h hostfile -c count\n");
    exit(1);
}

// Function to validate the port number passed
bool verifyPortNumber(long portNumber){
    error("Entered function verifyPortNumber\n");

    if(portNumber < 1024 || portNumber >65535){
        return false;
    }
    return true;
}

// Function to validate the hostfile passed
bool verifyHostFileName(string fileName){
    error("Entered function verifyHostFileName\n");

    FILE *fp;
    fp = fopen(fileName.c_str(), "rb");
    if(!fp){
        return false;
    }

    int numLine = 0;
    char ch;

    do{
        ch = fgetc(fp);
        if(ch == '\n'){
            numLine++;
        }
    }while(ch != EOF);

    fclose(fp);

    mySystemInfo.totalNumProcess = numLine;

    return true;
}

// Function to verify the value of count input parameter
bool verifyCount(int count){
    error("Entered function verifyCount\n");

    if(count < 0){
        return false;
    }

    return true;
}

/*
// Function to calculate the time difference between start and end time
double getTimeDiff(struct timeval *start, struct timeval* end, bool print){

    if (print){
        fprintf(stderr, "Enetered function getTimeDiff\n");
    }
    double diffCalc = (end->tv_sec - start->tv_sec)*pow(10,3) + 
                                (end->tv_usec - start->tv_usec) / pow(10,3);
    return diffCalc;
}

*/
// Function to send the acknowledgement
void sendAckMessage(uint32_t msgID, string receiverIP, generalSystemInfo& systemInfo, int portNum, 
                        senderInfo &mySenderSide){
    error("Entered the function sendAckMessage\n");

    // Create ACK message
    AckMessage* ackMessage = (AckMessage*) malloc(sizeof(AckMessage));

    if(!ackMessage){
        printf("Unable to allocate memory in sendAckMessage\n");
        return;
    }

    ackMessage->type = ACKMSG_TYPE;
    ackMessage->sender = systemInfo.selfID;
    ackMessage->msg_id = msgID;
    ackMessage->proposed_seq = ++(systemInfo.lastPropSeqNum);
    ackMessage->receiver = systemInfo.ipToID[receiverIP];


    struct sockaddr_in receiverAddr;

    if(mySenderSide.mySocket == -1){
        // Need to open a socket
        if((mySenderSide.mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            error("Error while opening socket to send the ack message\n");
            exit(1);
        }
    }

    memset((char*)&receiverAddr, 0, sizeof(receiverAddr));
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(portNum);

    if(inet_aton(receiverIP.c_str(), &receiverAddr.sin_addr) == 0){
        error("INET_ATON Failed\n");
    }

    // Converting the ACK to network order before sending
    convertAckByteOrder(ackMessage, true);

    if(sendto(mySenderSide.mySocket, ackMessage, sizeof(AckMessage), 0, 
                (struct sockaddr*) &receiverAddr, sizeof(receiverAddr)) == -1){
        if(paramDebug){
            fprintf(stderr, "%d: Failed to send the ACK message for msg %d to %s", 
                            systemInfo.selfID, msgID, receiverIP.c_str());
        }
    }
    else{
        if(paramDebug){
            fprintf(stderr, "%d: ACK successfully sent for msg %d to %s\n", 
            systemInfo.selfID, msgID, receiverIP.c_str());
        }
    }

}

// Function to print the data inside a message
void printMessage(void* msgToPrint, int type){

    if(paramDebug){

        if(type == 1){

            fprintf(stderr, "DATA MESSAGE:\n");
            DataMessage* msg = (DataMessage*) msgToPrint;

            fprintf(stderr, "Type: %d\n", msg->type);
            fprintf(stderr, "Sender: %d\n", msg->sender);
            fprintf(stderr, "MSG ID: %d\n", msg->msg_id);
            fprintf(stderr, "DATA: %d\n", msg->data);
        }
        else if(type == 2){
    
            fprintf(stderr, "ACK MESSAGE:\n");
            AckMessage* msg = (AckMessage*) msgToPrint;

            fprintf(stderr, "Type: %d\n", msg->type);
            fprintf(stderr, "Sender: %d\n", msg->sender);
            fprintf(stderr, "MSG ID: %d\n", msg->msg_id);
            fprintf(stderr, "PROPOSED SEQ: %d\n", msg->proposed_seq);
            fprintf(stderr, "Receiver: %d\n", msg->receiver);
        }
        else if(type == 3){

            fprintf(stderr, "SEQ MESSAGE:\n");
            SeqMessage* msg = (SeqMessage*) msgToPrint;

            fprintf(stderr, "Type: %d\n", msg->type);
            fprintf(stderr, "Sender: %d\n", msg->sender);
            fprintf(stderr, "MSG ID: %d\n", msg->msg_id);
            fprintf(stderr, "FINAL SEQ: %d\n", msg->final_seq);
        }
    }
}

// Function to update the sequence number in the queue and update the proposed 
// sequence number too
void updateFinalSeqNum(uint32_t senderId, uint32_t msgID, uint32_t finalSeqNum, 
                    generalSystemInfo& systemInfo, receiverInfo& myReceiverSide){

    error("Enetered function updateFinalSeqNum\n");

    msgInfo* localMsgPtr = NULL;

    // Look for the message in the ready queue
    for(myReceiverSide.iter1 = myReceiverSide.priorityToMsgList.begin();
        myReceiverSide.iter1 != myReceiverSide.priorityToMsgList.end();
        myReceiverSide.iter1++){
        
        localMsgPtr = myReceiverSide.iter1->second;

        if(!localMsgPtr){
            if(errorDebug){
                error("**************** MESSAGE NOT FOUND IN QUEUE *************\n");
            }
        }
        else{
            error("//////////////////// MESSAGE FOUND IN THE QUEUE //////////////\n");
        }

        if((localMsgPtr->myMsg->sender == senderId) && (localMsgPtr->myMsg->msg_id == msgID)){

            if(errorDebug){
                error("Message matched the senderID and msgID\n");
            }
            
            // Message found in the queue. Update its sequence number
            localMsgPtr->myseqNum = finalSeqNum;

            // Mark the message as deliverable
            localMsgPtr->isDeliverable = true;

            break;
        }
    }

    // Change the seq number for this message in the map. We need to erase the older
    // record and create a new one
    if(myReceiverSide.iter1 != myReceiverSide.priorityToMsgList.end()){
        if(errorDebug){
            error("Message found in the ready queue\n");
        }
        myReceiverSide.priorityToMsgList.erase(myReceiverSide.iter1);
    }

    // Create the new record
    myReceiverSide.priorityToMsgList[make_pair(finalSeqNum, senderId)] = localMsgPtr;

    // Update the proposed seq number if necessary
    if(systemInfo.lastPropSeqNum < finalSeqNum){
        systemInfo.lastPropSeqNum = finalSeqNum;
    }

    if(errorDebug){
        error("Exiting function updateFinalSeqNum\n");
    }
}

// Function to send the final sequence number message
void sendFinalSequenceNumberMsg(generalSystemInfo& systemInfo, senderInfo& mySenderSide,
                             receiverInfo& myReceiverSide){

    error("Entered function sendFinalSequenceNumberMsg\n");

    // Decide the final seq number
    uint32_t finalSeqNum = *(myReceiverSide.ackRecvdSeqNum.rbegin());

    // Create the final sequence number message
    SeqMessage* sendSeqMessage = (SeqMessage*) malloc(sizeof(SeqMessage));

    sendSeqMessage->type = SEQMSG_TYPE;
    sendSeqMessage->sender = systemInfo.selfID;
    sendSeqMessage->msg_id = mySenderSide.msgNumberSent;
    sendSeqMessage->final_seq = finalSeqNum;

    //Update the Message seq number with the final sequence number and also
    //update the proposed seq number if necessary
    updateFinalSeqNum(systemInfo.selfID, mySenderSide.msgNumberSent, finalSeqNum,
                        systemInfo, myReceiverSide);

    // Convert the sequence message from host to network order
    convertSeqByteOrder(sendSeqMessage, true);

    // Send the sequence message with the final sequence number to all the process
    for(int i = 1; i<=systemInfo.totalNumProcess; i++){
       
       // Dont send the message to self
        if(i != systemInfo.selfID){

            struct sockaddr_in receiverAddr;
            string receiverIP = systemInfo.hostIPAddr[i-1];

            if(mySenderSide.mySocket == -1){
                // Need to open a socket
                if((mySenderSide.mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
                    error("Error while opening socket to send the final sequence message\n");
                    exit(1);
                }
            }

            memset((char*)&receiverAddr, 0, sizeof(receiverAddr));
            receiverAddr.sin_family = AF_INET;
            receiverAddr.sin_port = htons(myReceiverSide.portNum);

            if(inet_aton(receiverIP.c_str(), &receiverAddr.sin_addr) == 0){
                error("INET_ATON Failed\n");
            }

            if(sendto(mySenderSide.mySocket, sendSeqMessage, sizeof(SeqMessage), 0,
                        (struct sockaddr*) &receiverAddr, sizeof(receiverAddr)) == -1){
                if(paramDebug){
                    fprintf(stderr, "Proc %d failed to send the Final Seq message to %s",
                            systemInfo.selfID, receiverIP.c_str());
                }
            }
            else{
                if(paramDebug){
                    fprintf(stderr, "Final Seq Msg successfully sent to %s by %s\n",
                            receiverIP.c_str(), systemInfo.selfHostName.c_str());
                }
            }

        }
    }
}

// Check if the messages in the ready queue can be now delivered or not.
// If yes then deliver those messages and print the output
void checkAndDeliverMsg(generalSystemInfo& systemInfo, senderInfo& mySenderSide,
                             receiverInfo& myReceiverSide){
    error("Entered function checkAndDeliverMsg\n");

    msgInfo* checkMsg = NULL;

    // Check which all message can be delivered
    for(myReceiverSide.iter1 = myReceiverSide.priorityToMsgList.begin();
        myReceiverSide.iter1 != myReceiverSide.priorityToMsgList.end();
        myReceiverSide.iter1++){

        checkMsg = myReceiverSide.iter1->second;

        if(!checkMsg){
            printf("***************************** NO MESSAGE IN QUEUE FOUND ******************** \n");
            exit(1);
        }

        // Update the delivered list of messages
        if(checkMsg->isDeliverable){
            myReceiverSide.procToDelvMsgList[checkMsg->myMsg->sender].insert(checkMsg->myMsg->msg_id);


            // Print the delivered message
            printf("%d: Processed message %d from sender %d with seq %d\n", systemInfo.selfID, checkMsg->myMsg->msg_id,
                    checkMsg->myMsg->sender, checkMsg->myseqNum);
            fflush(stdout);

            // Remove this message from the ready queue
            myReceiverSide.priorityToMsgList.erase(myReceiverSide.iter1);

        }
        else{
            return;
        }

    }

}

// To convert the byte order of the message
void convertByteOrder(DataMessage* dataMessage, bool hton){

    if(hton){

        error("Converting from host to network order\n");
        dataMessage->type = htonl(dataMessage->type);
        dataMessage->sender = htonl(dataMessage->sender);
        dataMessage->msg_id = htonl(dataMessage->msg_id);
        dataMessage->data = htonl(dataMessage->data);
    }
    else{
        
        error("Converting from network order to host\n");
        dataMessage->type = ntohl(dataMessage->type);
        dataMessage->sender = ntohl(dataMessage->sender);
        dataMessage->msg_id = ntohl(dataMessage->msg_id);
        dataMessage->data = ntohl(dataMessage->data);
    }
}

// To convert the byte order of the sequence message
void convertSeqByteOrder(SeqMessage* seqMessage, bool hton){

    if(hton){

        error("Converting from host to network order\n");
        seqMessage->type = htonl(seqMessage->type);
        seqMessage->sender = htonl(seqMessage->sender);
        seqMessage->msg_id = htonl(seqMessage->msg_id);
        seqMessage->final_seq = htonl(seqMessage->final_seq);
    }
    else{

        error("Converting from network order to host order\n");
        seqMessage->type = ntohl(seqMessage->type);
        seqMessage->sender = ntohl(seqMessage->sender);
        seqMessage->msg_id = ntohl(seqMessage->msg_id);
        seqMessage->final_seq = ntohl(seqMessage->final_seq);
    }
}


// To convert byte order of ACK message
void convertAckByteOrder(AckMessage* ackMessage, bool hton){

    if(hton){
        error("Converting Ack from host to network order\n");
        ackMessage->type = htonl(ackMessage->type);
        ackMessage->sender = htonl(ackMessage->sender);
        ackMessage->msg_id = htonl(ackMessage->msg_id);
        ackMessage->proposed_seq = htonl(ackMessage->proposed_seq);
        ackMessage->receiver = htonl(ackMessage->receiver);
    }
    else{
        error("Converting Ack from network to host order\n");
        ackMessage->type = ntohl(ackMessage->type);
        ackMessage->sender = ntohl(ackMessage->sender);
        ackMessage->msg_id = ntohl(ackMessage->msg_id);
        ackMessage->proposed_seq = ntohl(ackMessage->proposed_seq);
        ackMessage->receiver = ntohl(ackMessage->receiver);
    }
}

// Function to check if the received data message has already been delivered 
// or not
bool checkIfDelivered(DataMessage* message, receiverInfo myReceiverSide){

    error("Entered function checkIfDelivered\n");
    if(myReceiverSide.procToDelvMsgList[message->sender].find(message->msg_id) 
            != myReceiverSide.procToDelvMsgList[message->sender].end()){
        return true;
    }

    return false;

}

// Function to check if the received data message is already in the ready queue 
// or not.
bool checkIfQueued(DataMessage* message, receiverInfo myReceiverSide){

    error("Entered function checkIfQueued\n");

    // Look for the message in the ready queue
    for(myReceiverSide.iter1 = myReceiverSide.priorityToMsgList.begin();
            myReceiverSide.iter1 != myReceiverSide.priorityToMsgList.end();
            myReceiverSide.iter1++){

        msgInfo* localMsgPtr = myReceiverSide.iter1->second;

        if((localMsgPtr->myMsg->sender == message->sender) && (localMsgPtr->myMsg->msg_id == message->msg_id)){

            return true;
        }
    }

    return false;
}

// Function to store the message into the ready queue
void storeMessage(DataMessage* message, receiverInfo& myReceiverSide, generalSystemInfo systemInfo){

    error("Entered function storeMessage\n");

    msgInfo* storeMessage = (msgInfo*) malloc(sizeof(msgInfo));

    storeMessage->isDeliverable = false;
    storeMessage->myMsg = message;
    storeMessage->myseqNum = systemInfo.lastPropSeqNum;

    myReceiverSide.priorityToMsgList[make_pair(systemInfo.lastPropSeqNum, message->sender)] 
                = storeMessage;

}

// Function to verify the ACK message is valid or not.
bool verifyAckMessage(AckMessage* message, generalSystemInfo systemInfo, senderInfo mySenderSide){

    error("Entered verifyAckMessage\n");

    if(message->receiver != systemInfo.selfID){
        return false;
    }

    if(message->msg_id != mySenderSide.msgNumberSent){
        return false;
    }

    return true;
}

// Function to re-transmit the Data Message to the processes from which ACK 
// has not been received
void handleTimerExpiry(int sig){

    error("Entered function handleTimerExpiry\n");

    // Resend Data Message to all the process from which ACK is not received
    // Build the data message
    DataMessage* resendMessage = (DataMessage*) malloc(sizeof(DataMessage));
    resendMessage->type = DATAMSG_TYPE;
    resendMessage->sender = mySystemInfo.selfID;
    resendMessage->msg_id = sender.msgNumberSent;
    resendMessage->data = DUMMY;

    // Convert  the message from host to network before retransmission
    convertByteOrder(resendMessage, true);

    struct sockaddr_in lieuAddr;
    memset((char*)&lieuAddr, 0, sizeof(lieuAddr));
    lieuAddr.sin_family = AF_INET;
    lieuAddr.sin_port = htons(receiver.portNum);

    for(sender.ackIter = sender.expectAckFrom.begin();
            sender.ackIter != sender.expectAckFrom.end();
            sender.ackIter++){

        if(inet_aton(mySystemInfo.hostIPAddr[(*sender.ackIter - 1)].c_str(),
                    &lieuAddr.sin_addr) == 0){
            error("INET_ATON failed\n");
        }


        // Check the sender socket
        if(sender.mySocket == -1){
            // Need to open a socket
            if((sender.mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
                error("Error while opening socket to retransmit the data message\n");
                exit(1);
            }
        }

        if(sendto(sender.mySocket, resendMessage, sizeof(DataMessage), 0,
                    (struct sockaddr*) &lieuAddr, sizeof(lieuAddr)) == -1){
            if(paramDebug){
                fprintf(stderr, "Failed to send the mesage \
                        to %s\n",
                        mySystemInfo.hostIPAddr[(*sender.ackIter - 1)].c_str());
            }
        }else{
            if(paramDebug){
                fprintf(stderr, "Successfully sent the message \
                        to %s\n", mySystemInfo.hostIPAddr[(*sender.ackIter - 1)].c_str());
            }
        }
    }

    // Re-start the time
    signal(SIGALRM, handleTimerExpiry);
    alarm(ALARMTIME);
}


// **** MAIN FUNCTION **** //
int main(int argc, char* argv[])
{
    int argCount = 1;
    bool isCommanderProc = false;

    //generalSystemInfo mySystemInfo;
    mySystemInfo.totalNumProcess = 0;
    mySystemInfo.lastPropSeqNum = 0;
    mySystemInfo.lastFinalSeqNum = 0;
    mySystemInfo.numOfMsgToSend = 0;

    // Sender Information structure
    //senderInfo sender;
    sender.msgNumberSent = 0;

    // Receiver Information structure
    //receiverInfo receiver;

    // Implement the command line option parser
    if(argc < 7)
    {
        // Insufficient number of options entered
        error("Invalid command entered\n");
        printUsage();
    }

    do{
        // Get the port number
        if(strcmp(argv[argCount], "-p") == 0){
            long port = atoi(argv[++argCount]);
            if(verifyPortNumber(port)){
                receiver.portNum =  port;
            }
            else{
                error("Invalid Port Number entered\n");
                exit(1);
            }
            argCount++;
        }
        // Get the host file name
        if(strcmp(argv[argCount], "-h") == 0){
            string fileName(argv[++argCount]);
            if(verifyHostFileName(fileName)){
                mySystemInfo.hostFileName = fileName;
            }
            else{
                error("Invalid hostfile name entered\n");
                exit(1);
            }
            argCount++;
        }

        // Get the faulty process count
        if(strcmp(argv[argCount], "-c") == 0){
            int numMulMsg = atoi(argv[++argCount]);

            if(verifyCount(numMulMsg)){
                mySystemInfo.numOfMsgToSend = numMulMsg;
            }
            else{
                error("Invalid count entered \n");
                exit(1);
            }
            argCount++;
        }
    }while(argCount < argc); 

    if(paramDebug){
        fprintf(stderr, "Inside the print if\n");
        fprintf(stderr, "Enetered port number is: %ld\n", receiver.portNum);
        fprintf(stderr, "Entered host file name is: %s\n", 
                mySystemInfo.hostFileName.c_str());
        fprintf(stderr, "Count of message is: %d\n", mySystemInfo.numOfMsgToSend);
    }

    struct in_addr **ipAddr;
    struct hostent* he;
    FILE* fp = fopen(mySystemInfo.hostFileName.c_str(), "r");

    if(paramDebug){
        fprintf(stderr, "total process: %d\n", mySystemInfo.totalNumProcess);
    }
    // Get self hostname
    char myName[100];
    gethostname(myName, 100);
    mySystemInfo.selfHostName = myName;

    struct sockaddr_in lieuAddr;
    struct sockaddr_in senderProcAddr;
    struct sockaddr_in myInfo;

    for(int i = 0; i<mySystemInfo.totalNumProcess; i++){

        if(fp == NULL){
            error("Unable to open the hostfile\n");
            exit(1);
        }

        // Get the ipaddress of all the hosts.
        char currHost[100];

        if(fgets(currHost, 100, fp) != NULL){
            mySystemInfo.hostNames.push_back(currHost);
            if(mySystemInfo.hostNames[i].rfind('\n') != string::npos){
                mySystemInfo.hostNames[i].erase(--(mySystemInfo.hostNames[i].end()));
            }

        }

        if((he = gethostbyname(mySystemInfo.hostNames[i].c_str())) == NULL){
            fprintf(stderr, "Unable to get the ip address of the host: %s\n",
                    currHost);
            exit(1);
        }

        //Store the ip address
        ipAddr = (struct in_addr**)he->h_addr_list;

        if(mySystemInfo.selfHostName.compare(mySystemInfo.hostNames[i]) == 0){
            string currentIP = inet_ntoa(*ipAddr[XINU]);

            if(currentIP.find("127") == 0){
                mySystemInfo.hostIPAddr.push_back(inet_ntoa(*ipAddr[VM]));
            }
            else{
                mySystemInfo.hostIPAddr.push_back(inet_ntoa(*ipAddr[XINU]));
            }
        }
        else{
            mySystemInfo.hostIPAddr.push_back(inet_ntoa(*ipAddr[XINU]));
        }

        // update the map
        mySystemInfo.ipToID.insert(pair<string, int>(mySystemInfo.hostIPAddr[i], i+1));
        mySystemInfo.hostNameToIP.insert(
                pair<string, string>(mySystemInfo.hostNames[i], 
                    mySystemInfo.hostIPAddr[i]));

        if (paramDebug)
            fprintf(stderr, "Host name: %s, Ip address: %s\n",
                    mySystemInfo.hostNames[i].c_str(), 
                    mySystemInfo.hostIPAddr[i].c_str());
    }
    fclose(fp);

    mySystemInfo.selfID = 
        mySystemInfo.ipToID[mySystemInfo.hostNameToIP[mySystemInfo.selfHostName]];

    if(paramDebug){
        fprintf(stderr, "Self hostname is %s\n", mySystemInfo.selfHostName.c_str());
        fprintf(stderr, "Self IP Address is %s\n", 
                mySystemInfo.hostNameToIP[mySystemInfo.selfHostName].c_str());
        fprintf(stderr, "Self ID is %d\n", mySystemInfo.selfID);
    }


    // To store the address of sender of ACK
    memset((char*)&senderProcAddr, 0, sizeof(senderProcAddr));
    socklen_t senderLen = sizeof(senderProcAddr);

    // store the info for sending the message
    memset((char*)&lieuAddr, 0, sizeof(lieuAddr));
    lieuAddr.sin_family = AF_INET;
    lieuAddr.sin_port = htons(receiver.portNum);

    // Store the info to bind receiving port with the socket.
    memset((char*)&myInfo, 0, sizeof(myInfo));
    myInfo.sin_family = AF_INET;
    myInfo.sin_port = htons(receiver.portNum);
    myInfo.sin_addr.s_addr = htonl(INADDR_ANY);

    // Open a sender socket to send the messages
    if((sender.mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        error("Error when trying to open the socket to send message\n");
        exit(1);
    }

    // Open a receiver socket to receive the message
    if((receiver.mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        error("Error when trying to open the receiving socket \n");

        exit(1);
    }

    // bind the receiver socket
    if(bind(receiver.mySocket, (struct sockaddr*) &myInfo, sizeof(myInfo)) == -1){
        error("Bind failed for receiving socket \n");
        exit(1);
    }


    if(paramDebug){
        fprintf(stderr, "Self Name: %s and self ID: %d\n",
                mySystemInfo.selfHostName.c_str(), mySystemInfo.selfID);
    }

    do{
        if(sender.msgNumberSent < mySystemInfo.numOfMsgToSend){

            for(int i = 1; i <= mySystemInfo.totalNumProcess; i++){
                sender.expectAckFrom.insert(i);
            }

            // Remove self ID from the ack
            sender.expectAckFrom.erase(mySystemInfo.selfID);

            // send the data message to all the process from whom ack is not received
            // DONT FREE THIS MEMORY
            DataMessage *myMessage = (DataMessage*)malloc(sizeof(DataMessage));

            if(!myMessage){
                printf("Unable to malloc for sending message with id: %d\n", sender.msgNumberSent);
                printf("Exiting the application\n");
                exit(1);    
            }

            myMessage->type = DATAMSG_TYPE;
            myMessage->sender = mySystemInfo.selfID;
            myMessage->msg_id = ++(sender.msgNumberSent);
            myMessage->data = DUMMY;


            // convert the message to network order before sending
            convertByteOrder(myMessage, true);

            // send the data message to all the process from whom ack is not received
            for(sender.ackIter = sender.expectAckFrom.begin(); 
                    sender.ackIter != sender.expectAckFrom.end(); 
                    sender.ackIter++){

                if(inet_aton(mySystemInfo.hostIPAddr[(*sender.ackIter - 1)].c_str(), 
                            &lieuAddr.sin_addr) == 0){
                    error("INET_ATON failed\n");
                    //exit(1);
                }


                if(sendto(sender.mySocket, myMessage, sizeof(DataMessage), 0, 
                            (struct sockaddr*) &lieuAddr, sizeof(lieuAddr)) == -1){
                    if(paramDebug){
                        fprintf(stderr, "Failed to send the mesage \
                                to %s\n", 
                                mySystemInfo.hostIPAddr[(*sender.ackIter - 1)].c_str());
                    }
                }else{
                    if(paramDebug){
                        fprintf(stderr, "Successfully sent the message \
                                to %s\n", mySystemInfo.hostIPAddr[(*sender.ackIter - 1)].c_str());
                    }
                }
            }

            // Place this message in my own queue.
            // DONT FREE THIS MEMORY
            //Convert the message from network to host order before storing
            convertByteOrder(myMessage, false);

            msgInfo* selfMessage = (msgInfo*) malloc(sizeof(msgInfo));
            selfMessage->myMsg = myMessage;
            selfMessage->myseqNum = ++(mySystemInfo.lastPropSeqNum);
            selfMessage->isDeliverable = false;

            receiver.priorityToMsgList[make_pair(selfMessage->myseqNum, selfMessage->myMsg->sender)] = selfMessage;
            sender.seqNumberOfSelfMsg = selfMessage->myseqNum;
            receiver.ackRecvdSeqNum.insert(sender.seqNumberOfSelfMsg);
            
            // Set the Alarm as the process has send messages to all the processes.
            signal(SIGALRM, handleTimerExpiry);
            alarm(ALARMTIME);
        }

        int status = -1;
        int ackIndex = -1;


        do{
            // Allocating memory for ACK msg type as it is the largest.
            // Need to free this memory after the receive loop.
            void* recvdMessage = (void*)malloc(sizeof(AckMessage));

            status = recvfrom(receiver.mySocket, recvdMessage, sizeof(AckMessage), 
                    0, (struct sockaddr*) &senderProcAddr, &senderLen);


            if(status > 0){

                // Check the message type
                uint32_t* msgType = (uint32_t*) malloc(sizeof(uint32_t));

                // Exit the application if failed to malloc the memory
                if(!msgType){
                    printf("Unable to allocate the memory for msgType\n");
                    printf("Exiting the application\n");
                    exit(1);
                }

                memcpy(msgType, recvdMessage, sizeof(uint32_t));

                uint32_t type = *msgType;
                type = htonl(type);
                string senderIP = inet_ntoa(senderProcAddr.sin_addr);

                // If message is ACK type
                if(type == ACKMSG_TYPE){

                    // Get the ID of the process who has send the ACK
                    ackIndex = mySystemInfo.ipToID[senderIP];

                    // Allocate memory for the ACK message
                    AckMessage* ackMessage = (AckMessage*)recvdMessage;

                    // Convert the ACK MEssage from network to host byte order
                    convertAckByteOrder(ackMessage, false);

                    // verify the ACK message
                    if(!verifyAckMessage(ackMessage, mySystemInfo, sender)){
                       
                        error("Invalid ACK MESSAGE received\n");
                        // Invalid ACK message received so free the buffer and again go for
                        // receiving message
                        if(ackMessage){
                            free(ackMessage);
                            ackMessage = NULL;
                        }
                        continue;
                    }

                    // Remove the id from the sender expected ack list
                    sender.ackIter = sender.expectAckFrom.find(ackIndex);

                    if(sender.ackIter != sender.expectAckFrom.end()){
                        sender.expectAckFrom.erase(sender.ackIter);
                        if(paramDebug){
                            fprintf(stderr, "Valid ACK received by \
                                    the process %d from %s\n", 
                                    mySystemInfo.selfID, senderIP.c_str()); 
                        }
                    }
                    else{
                        if(paramDebug){
                            fprintf(stderr, "UNEXPECTED ACK received \
                                    by the process %d from %s\n", 
                                    mySystemInfo.selfID, senderIP.c_str());
                        }
                    }

                    // Store the received sequence number in the list for this message
                    // We will use this list to compute the final sequence number for our
                    // data message.
                    receiver.ackRecvdSeqNum.insert(ackMessage->proposed_seq);

                    // Check if we have received all the ACK's
                    if(receiver.ackRecvdSeqNum.size() == mySystemInfo.totalNumProcess){
                        // We have received all the acks from the processes. So reset the alarm
                        // and send the final sequence message to all the process
                        alarm(0);

                        // Send the final Sequence number to all. NEED TO IMPLEMENT THIS FUNCTION
                        sendFinalSequenceNumberMsg(mySystemInfo, sender, receiver);

                        // Check if the messages in the queue can be delivered or not
                        checkAndDeliverMsg(mySystemInfo, sender, receiver);

                        // Clear the multiset which contains the sequence number of all the received ACKs
                        receiver.ackRecvdSeqNum.clear();

                        // Received all the ACKs so break from the loop
                        break;
                    }
                }
                // If message is data message type
                else if(type == DATAMSG_TYPE){
                    if(paramDebug){
                        fprintf(stderr, "Received Data message by process %d \
                                from %s\n", mySystemInfo.selfID, senderIP.c_str());
                    }

                    DataMessage* dataMessage = (DataMessage*)recvdMessage;
                    
                    // Convert message from the network to host order
                    convertByteOrder(dataMessage, false);

                    // Check if the sender is a valid process or not
                    if(dataMessage->sender < 1 || dataMessage->sender > mySystemInfo.totalNumProcess){
                        
                        error("Invalid DATA Message received\n");
                        // Invalid data message received
                        // Free the message buffer
                        if(dataMessage){
                            free(dataMessage);
                            dataMessage = NULL;
                        }
                        continue;
                    }

                    bool msgAlreadyDelivered = false;
                    bool msgAlreadyInQueue = false;

                    // Check if the received data message has already been delivered or not
                    msgAlreadyDelivered = checkIfDelivered(dataMessage, receiver);

                    // Check if the received data message is already in the queue or not
                    // If its there then don't send the ACK. Coz we are assuming the ACK
                    // will not be dropped and we must have send the ACK while storing it
                    // in the queue.
                    msgAlreadyInQueue = checkIfQueued(dataMessage, receiver);

                    if(!msgAlreadyInQueue && !msgAlreadyDelivered){

                        // Send the ACK for the message and store the message in the queue.
                        // So we need to increase the proposed Seq Number by 1 before sending the
                        // ACK. While storing just use the proposed sequence number
                        sendAckMessage(dataMessage->msg_id, senderIP, mySystemInfo, receiver.portNum, sender);

                        // Store the message in the ready Queue 
                        storeMessage(dataMessage, receiver, mySystemInfo);

                    }
                    else{
                        if(paramDebug){
                            fprintf(stderr, "Process %d received data message from %s, which is already \
                                    processes\n", mySystemInfo.selfID, senderIP.c_str());
                        }
                    }

                    //checkAndDeliverMsg(mySystemInfo, sender, receiver);

                }
                else if(type == SEQMSG_TYPE){

                    if(paramDebug){
                        fprintf(stderr, "Received Seq message by process %d \
                                                    from %s\n", mySystemInfo.selfID, senderIP.c_str());
                    }

                    // Final sequence received
                    SeqMessage* seqMessage = (SeqMessage*)recvdMessage;

                    // Convert from network to host order
                    convertSeqByteOrder(seqMessage, false);

                    // Verify that the sequence message is received from valid sender
                    if(seqMessage->sender < 1 || seqMessage->sender > mySystemInfo.totalNumProcess){
                    
                        error("Invalid Sequence Message received\n");
                        // Invalid Final Sequence Message received
                        if(seqMessage){
                            free(seqMessage);
                            seqMessage = NULL;
                        }
                        continue;
                    }

                    //Update the sequence number of the message in the queue
                    //and the proposed sequence number of the system if needed
                    updateFinalSeqNum(seqMessage->sender, seqMessage->msg_id, seqMessage->final_seq, 
                                        mySystemInfo, receiver);

                    // Check and Deliver the message
                    checkAndDeliverMsg(mySystemInfo, sender, receiver);
                }
                else{
                    if(errorDebug){
                        error("Unexpected type of message received\n");
                    }
                }
            }
            else if(status < 0){
                if((errno != EWOULDBLOCK) && (errno != EAGAIN)){
                    if(paramDebug){
                        fprintf(stderr, "Error while receive %d\n", errno);
                    }
                }

                if(recvdMessage){
                    //free(recvdMessage);
                    //recvdMessage = NULL;

                }
            }

        }while(1); // Receive Loop. Will break when we have received all the ACK's

    }while(1); // Infinite Loop to send

    // Close the sockets
    close(receiver.mySocket);
    close(sender.mySocket);
}
