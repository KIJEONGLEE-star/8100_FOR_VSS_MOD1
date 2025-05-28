#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       // for close()
#include <arpa/inet.h>    // for inet_ntoa(), htons()
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#include "aabcop_new.h"   // 사용자 정의 헤더

#define MAILBOX_SIZE 4
#define MAX_DATA_SIZE 128
#define MAX_COMMENT_SIZE 256

typedef struct {
    uint8_t frameType;
    uint8_t dataSize;
    uint8_t cid;
    uint8_t data[MAX_DATA_SIZE];
    uint8_t dataIndex;
    uint8_t expectedFrames;
    uint8_t receivedFrames;
    char comment[MAX_COMMENT_SIZE];
} MessageBuffer;

void socket_write_callback(uint8_t byte, void* ctx) {
    int* sock_ptr = (int*)ctx;
    int sock = *sock_ptr;
    static uint8_t mailbox[MAILBOX_SIZE];
    static uint8_t mailboxIndex = 0;

    mailbox[mailboxIndex++] = byte;

    if (mailboxIndex == MAILBOX_SIZE) {
        send(sock, (const char*)mailbox, MAILBOX_SIZE, 0);
        printf("Sent Mailbox: ");
        for (int i = 0; i < MAILBOX_SIZE; i++) {
            printf("0x%02X ", mailbox[i]);
        }
        printf("\n");
        mailboxIndex = 0;
    }
}

void processMailbox(uint8_t* mailbox, MessageBuffer* msgBuffer, int clientSocket) {
    printf("Received mailbox bytes: ");
    for (int i = 0; i < MAILBOX_SIZE; i++) {
        printf("0x%02X ", mailbox[i]);
    }
    printf("\n");

    uint8_t frameType = (mailbox[0] >> 7) & 0x01;

    if (frameType == 0) {
        msgBuffer->frameType = 0;
        msgBuffer->dataSize = mailbox[0] & 0x7F;
        msgBuffer->cid = mailbox[1];
        msgBuffer->dataIndex = 0;

        if (msgBuffer->dataSize > 0 && msgBuffer->dataIndex < MAX_DATA_SIZE)
            msgBuffer->data[msgBuffer->dataIndex++] = mailbox[2];
        if (msgBuffer->dataSize > 1 && msgBuffer->dataIndex < MAX_DATA_SIZE)
            msgBuffer->data[msgBuffer->dataIndex++] = mailbox[3];

        if (msgBuffer->dataSize <= 2)
            msgBuffer->expectedFrames = 1;
        else {
            uint8_t remainingData = msgBuffer->dataSize - 2;
            msgBuffer->expectedFrames = 1 + (remainingData + 2) / 3;
        }

        msgBuffer->receivedFrames = 1;

        printf("Start Frame: DataSize=%d, CID=0x%02X, ExpectedFrames=%d\n",
               msgBuffer->dataSize, msgBuffer->cid, msgBuffer->expectedFrames);
    } else {
        uint8_t counter = mailbox[0] & 0x7F;
        printf("Continuation Frame: Counter=%d\n", counter);

        for (int i = 1; i < MAILBOX_SIZE; i++) {
            if (msgBuffer->dataIndex < MAX_DATA_SIZE)
                msgBuffer->data[msgBuffer->dataIndex++] = mailbox[i];
        }

        msgBuffer->receivedFrames++;
    }

    if (msgBuffer->receivedFrames == msgBuffer->expectedFrames) {
        printf("\n*** Message Complete ***\n");
        printf("CID: 0x%02X, Data Length: %d\n", msgBuffer->cid, msgBuffer->dataIndex);

        printf("Data: ");
        for (int i = 0; i < msgBuffer->dataIndex; i++) {
            printf("0x%02X ", msgBuffer->data[i]);
        }
        printf("\n");

        aabcop_print_cid_info(msgBuffer->cid, msgBuffer->data, msgBuffer->dataIndex);

        uint8_t cidType = aabcop_get_cid_message_type(msgBuffer->cid);

        if (cidType & CID_TYPE_R) {
            printf("Generating automatic response for CID 0x%02X...\n", msgBuffer->cid);
            uint8_t responseData[MAX_DATA_SIZE];
            uint8_t responseLength = 0;

            responseData[0] = msgBuffer->cid;
            responseData[1] = RSP_STATUS_COMPLETION;
            responseLength = 2;

            switch (msgBuffer->cid) {
                case CID_NETWORK_STARTUP:
                    responseData[2] = UID_DANG8100;
                    responseLength = 3;
                    break;

                case CID_ZONE_VOLUME:
                    if (msgBuffer->dataIndex >= 2) {
                        responseData[2] = msgBuffer->data[0];
                        responseData[3] = msgBuffer->data[1];
                        responseLength = 4;
                    }
                    break;

                case CID_MODULE_STATUS:
                    responseData[2] = 0x81;
                    responseData[3] = 0x48;
                    responseLength = 4;
                    break;
            }

            printf("Sending response with CID 0x01 (Response Command), data length: %d\n", responseLength);

            aabcop_create_frames(UID_DANG8100, CID_RESPONSE, responseData, responseLength,
                                 socket_write_callback, &clientSocket);
        }

        printf("--------------------------------------------\n\n");
    }
}

void sendResponseMessage(int clientSocket, uint8_t cid, uint8_t status) {
    uint8_t mailbox[MAILBOX_SIZE];

    mailbox[0] = 0x03;
    mailbox[1] = CID_RESPONSE;
    mailbox[2] = cid;
    mailbox[3] = status;

    send(clientSocket, (const char*)mailbox, MAILBOX_SIZE, 0);

    printf("Sent response: CID=0x%02X, Status=%d\n", cid, status);
}

int main() {
    int listenSocket, clientSocket;
    struct sockaddr_in server, client;
    socklen_t clientLen = sizeof(client);
    uint8_t mailbox[MAILBOX_SIZE];
    MessageBuffer msgBuffer = {0};
    int interactiveMode = 0;
    int enableRSResponses = 0;

    char choice;
    printf("Enable interactive mode (respond to commands)? (y/n): ");
    scanf(" %c", &choice);
    interactiveMode = (choice == 'y' || choice == 'Y');

    printf("Enable automatic responses for R/S type CIDs? (y/n): ");
    scanf(" %c", &choice);
    enableRSResponses = (choice == 'y' || choice == 'Y');

    printf("AABCOP Protocol Receiver %s, R/S Responses %s\n",
           interactiveMode ? "(Interactive Mode)" : "(Monitor Mode)",
           enableRSResponses ? "Enabled" : "Disabled");

    // Create socket
    if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Could not create socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(12345);

    if (bind(listenSocket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(listenSocket, 3);

    printf("Waiting for connections on port 12345...\n");

    if ((clientSocket = accept(listenSocket, (struct sockaddr*)&client, &clientLen)) < 0) {
        perror("Accept failed");
        return 1;
    }

    printf("Connection accepted from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    aabcop_init();

    int recvSize;
    memset(&msgBuffer, 0, sizeof(MessageBuffer));

    while ((recvSize = recv(clientSocket, (char*)mailbox, MAILBOX_SIZE, 0)) > 0) {
        if (recvSize == MAILBOX_SIZE) {
            processMailbox(mailbox, &msgBuffer, clientSocket);

            if (msgBuffer.receivedFrames == msgBuffer.expectedFrames) {
                uint8_t cid = msgBuffer.cid;
                uint8_t cidType = aabcop_get_cid_message_type(cid);

                if (interactiveMode && !enableRSResponses) {
                    sendResponseMessage(clientSocket, cid, RSP_STATUS_COMPLETION);
                }
            }
        } else {
            printf("Received incomplete mailbox (%d bytes)\n", recvSize);
        }
    }

    if (recvSize == 0) {
        printf("Connection closed by client\n");
    } else {
        perror("recv failed");
    }

    close(clientSocket);
    close(listenSocket);

    return 0;
}
