#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#include "aabcop.h"

// Constants
#define MAX_DATA_SIZE      128
#define MAX_COMMENT_SIZE   256

// Global variables
uint8_t Mailbox[MAILBOX_SIZE];
uint8_t MailboxIndex = 0;

// Application packet structure
typedef struct {
    uint8_t UID;
    uint8_t CID;
    uint8_t* Data;
    uint8_t L_SIZE;
    char Comment[MAX_COMMENT_SIZE];
} AppPacket;

// Function to write a byte to the mailbox and send when full
void WriteToMailbox(uint8_t byte, SOCKET sock) {
    Mailbox[MailboxIndex++] = byte;

    if (MailboxIndex == MAILBOX_SIZE) {
        send(sock, (const char*)Mailbox, MAILBOX_SIZE, 0);
        printf("Sent Mailbox: ");
        for (int i = 0; i < MAILBOX_SIZE; i++) {
            printf("0x%02X ", Mailbox[i]);
        }
        printf("\n");
        MailboxIndex = 0;
    }
}

// Function to create A2B frames from an application packet
void CreateA2BFrames(AppPacket* packet, SOCKET sock) {
    uint8_t L = packet->L_SIZE;

    // Print message details
    printf("\n=== Sending Message Details ===\n");
    printf("UID: 0x%02X\n", packet->UID);
    printf("CID: 0x%02X - ", packet->CID);
    printCIDDescription(packet->CID);
    printf("Data Length: %d\n", L);
    
    if (L > 0) {
        printf("Data Values: ");
        for (int i = 0; i < L; i++) {
            printf("0x%02X ", packet->Data[i]);
        }
        printf("\n");
    }
    
    if (strlen(packet->Comment) > 0) {
        printf("Comment: %s\n", packet->Comment);
    }
    printf("==========================\n\n");

    if (L <= 2) {
        // Single frame
        uint8_t header = 0x7F & L;  // FRAME_TYPE=0, DATA_SIZE=L
        WriteToMailbox(header, sock);
        WriteToMailbox(packet->CID, sock);
        WriteToMailbox(L > 0 ? packet->Data[0] : 0x00, sock);
        WriteToMailbox(L > 1 ? packet->Data[1] : 0x00, sock);
    } else {
        // Start frame
        uint8_t header = 0x7F & L;  // FRAME_TYPE=0, DATA_SIZE=L
        WriteToMailbox(header, sock);
        WriteToMailbox(packet->CID, sock);
        WriteToMailbox(packet->Data[0], sock);
        WriteToMailbox(packet->Data[1], sock);

        // Calculate number of multi-frames needed
        uint8_t multiCount = (L - 2 + 2) / 3;  // Ceiling division for (L-2)/3
        uint8_t dataIndex = 2;

        // Create multi-frames
        for (uint8_t counter = 1; counter <= multiCount; counter++) {
            uint8_t header = 0x80 | counter;  // FRAME_TYPE=1, COUNTER=counter
            WriteToMailbox(header, sock);

            // Add up to 3 data bytes per multi-frame
            for (int i = 0; i < 3; i++) {
                if (dataIndex < L) {
                    WriteToMailbox(packet->Data[dataIndex++], sock);
                } else {
                    WriteToMailbox(0x00, sock);  // Padding
                }
            }
        }
    }

    // Flush any remaining data
    if (MailboxIndex > 0) {
        // Pad remaining bytes with zeros
        while (MailboxIndex < MAILBOX_SIZE) {
            Mailbox[MailboxIndex++] = 0x00;
        }
        
        send(sock, (const char*)Mailbox, MAILBOX_SIZE, 0);
        printf("Sent Mailbox (final): ");
        for (int i = 0; i < MAILBOX_SIZE; i++) {
            printf("0x%02X ", Mailbox[i]);
        }
        printf("\n");
        MailboxIndex = 0;
    }
}

// Function to print CID descriptions
void printCIDDescription(uint8_t cid) {
    printf("Selected CID: 0x%02X - ", cid);
    
    // Get CID Type
    uint8_t cidType = 0;
    
    switch (cid) {
        // Network generic CIDs
        case CID_NETWORK_STARTUP:
            printf("Network Startup/Validation");
            cidType = CID_TYPE_NETWORK_STARTUP;
            break;
        case CID_RESPONSE:
            printf("Response Command");
            cidType = CID_TYPE_RESPONSE;
            break;
        case CID_COMMUNICATION_ERROR:
            printf("Communication Error");
            cidType = CID_TYPE_COMMUNICATION_ERROR;
            break;

        // Audio control CIDs
        case CID_AUDIO_VIDEO_SOURCE_NAME:
            printf("Audio/Video Source Name");
            cidType = CID_TYPE_AUDIO_VIDEO_SOURCE_NAME;
            break;
        case CID_AUDIO_SOURCE_TYPE:
            printf("Audio Source Type and Capabilities");
            cidType = CID_TYPE_AUDIO_SOURCE_TYPE;
            break;
        case CID_PLAY_STATUS:
            printf("Play Status");
            cidType = CID_TYPE_PLAY_STATUS;
            break;
        case CID_ZONE_VOLUME:
            printf("Zone Volume Absolute");
            cidType = CID_TYPE_ZONE_VOLUME;
            break;
        case CID_ZONE_VOLUME_STEP:
            printf("Zone Volume Step");
            cidType = CID_TYPE_ZONE_VOLUME_STEP;
            break;
        case CID_MUTE_ZONE:
            printf("Mute Zone");
            cidType = CID_TYPE_MUTE_ZONE;
            break;
        case CID_MUTE_CHANNELS:
            printf("Mute Channels");
            cidType = CID_TYPE_MUTE_CHANNELS;
            break;
        case CID_ELAPSED_TRACK_TIME:
            printf("Elapsed Track/Chapter Time");
            cidType = CID_TYPE_ELAPSED_TRACK_TIME;
            break;
        case CID_TRACK_TIME:
            printf("Track/Chapter Time");
            cidType = CID_TYPE_TRACK_TIME;
            break;
        case CID_REPEAT_SUPPORT:
            printf("Repeat Support");
            cidType = CID_TYPE_REPEAT_SUPPORT;
            break;
        case CID_SHUFFLE_SUPPORT:
            printf("Shuffle Support");
            cidType = CID_TYPE_SHUFFLE_SUPPORT;
            break;

        // Audio inputs CIDs
        case CID_LIBRARY_DATA_TYPE:
            printf("Library Data Type");
            cidType = CID_TYPE_LIBRARY_DATA_TYPE;
            break;
        case CID_LIBRARY_DATA_NAME:
            printf("Library Data Name");
            cidType = CID_TYPE_LIBRARY_DATA_NAME;
            break;
        case CID_ARTIST_NAME:
            printf("Artist Name");
            cidType = CID_TYPE_ARTIST_NAME;
            break;
        case CID_ALBUM_NAME:
            printf("Album Name");
            cidType = CID_TYPE_ALBUM_NAME;
            break;
        case CID_STATION_NAME:
            printf("Station Name");
            cidType = CID_TYPE_STATION_NAME;
            break;
        case CID_POWER:
            printf("Power");
            cidType = CID_TYPE_POWER;
            break;
        case CID_TOTAL_ZONES:
            printf("Total Number of Zones Available");
            cidType = CID_TYPE_TOTAL_ZONES;
            break;
        case CID_ZONE_NAME:
            printf("Zone Name");
            cidType = CID_TYPE_ZONE_NAME;
            break;

        // Equalization CIDs
        case CID_MAIN_SUB_SWITCHING:
            printf("Main/Sub Switching");
            cidType = CID_TYPE_MAIN_SUB_SWITCHING;
            break;
        case CID_EQ_PRESET_NAME:
            printf("EQ Preset Name");
            cidType = CID_TYPE_EQ_PRESET_NAME;
            break;
        case CID_EQ_BASS:
            printf("Equalizer Bass");
            cidType = CID_TYPE_EQ_BASS;
            break;
        case CID_EQ_TREBLE:
            printf("Equalizer Treble");
            cidType = CID_TYPE_EQ_TREBLE;
            break;
        case CID_EQ_MID_RANGE:
            printf("Equalizer Mid Range");
            cidType = CID_TYPE_EQ_MID_RANGE;
            break;
        case CID_BALANCE:
            printf("Balance");
            cidType = CID_TYPE_BALANCE;
            break;
        case CID_FADE:
            printf("Fade");
            cidType = CID_TYPE_FADE;
            break;
        case CID_SUB_VOLUME:
            printf("Non-Fader, Sub Volume");
            cidType = CID_TYPE_SUB_VOLUME;
            break;
        case CID_SUBWOOFER_SWITCH:
            printf("Subwoofer Direct Switching");
            cidType = CID_TYPE_SUBWOOFER_SWITCH;
            break;
        case CID_CENTER_SWITCH:
            printf("Center Direct Switching");
            cidType = CID_TYPE_CENTER_SWITCH;
            break;
        case CID_TONE_BATCH:
            printf("Tone Batch Direct Switching");
            cidType = CID_TYPE_TONE_BATCH;
            break;
        case CID_BEEP_VOLUME:
            printf("Beep Volume Direct Switching");
            cidType = CID_TYPE_BEEP_VOLUME;
            break;
        case CID_SPEED_COMP:
            printf("Speed Compensation");
            cidType = CID_TYPE_SPEED_COMP;
            break;
        case CID_OVERHEAD_SWITCH:
            printf("Overhead Direct Switching");
            cidType = CID_TYPE_OVERHEAD_SWITCH;
            break;
        case CID_ANC_ZONE_ENABLE:
            printf("ANC Zone Enable");
            cidType = CID_TYPE_ANC_ZONE_ENABLE;
            break;
        case CID_BEEP:
            printf("Beep");
            cidType = CID_TYPE_BEEP;
            break;
        case CID_VOICE_OUTPUT:
            printf("Voice Output");
            cidType = CID_TYPE_VOICE_OUTPUT;
            break;

        // Bluetooth related CIDs
        case CID_BT_ADDR_AVAILABLE:
            printf("Number of Bluetooth Addresses Available");
            cidType = CID_TYPE_BT_ADDR_AVAILABLE;
            break;
        case CID_BT_DEVICE_ADDR:
            printf("Bluetooth Device Address");
            cidType = CID_TYPE_BT_DEVICE_ADDR;
            break;
        case CID_BT_DEVICE_STATUS:
            printf("Bluetooth Device Status");
            cidType = CID_TYPE_BT_DEVICE_STATUS;
            break;
        case CID_BT_DEVICE_NAME:
            printf("Bluetooth Device Name");
            cidType = CID_TYPE_BT_DEVICE_NAME;
            break;
        case CID_BT_PAIRING_STATUS:
            printf("Bluetooth Pairing Status");
            cidType = CID_TYPE_BT_PAIRING_STATUS;
            break;
        case CID_BT_FORGET_DEVICE:
            printf("Forget Bluetooth Device");
            cidType = CID_TYPE_BT_FORGET_DEVICE;
            break;
        case CID_BT_DISCOVERING:
            printf("Bluetooth Discovering");
            cidType = CID_TYPE_BT_DISCOVERING;
            break;

        // Audio configuration and PLL Lock CIDs
        case CID_CHANNEL_SLOT:
            printf("Channel Slot Assignment");
            cidType = CID_TYPE_CHANNEL_SLOT;
            break;
        case CID_PLL_LOCK_STATUS:
            printf("PLL Lock Status");
            cidType = CID_TYPE_PLL_LOCK_STATUS;
            break;
        case CID_INPUT_VOLTAGE:
            printf("Input Voltage");
            cidType = CID_TYPE_INPUT_VOLTAGE;
            break;
        case CID_INPUT_CURRENT:
            printf("Input Current");
            cidType = CID_TYPE_INPUT_CURRENT;
            break;
        case CID_TEMPERATURE:
            printf("Temperature Input Filter");
            cidType = CID_TYPE_TEMPERATURE;
            break;
        case CID_SENSOR_GENERIC:
            printf("Sensor Generic");
            cidType = CID_TYPE_SENSOR_GENERIC;
            break;

        // Module configuration CIDs
        case CID_MODULE_ENABLE:
            printf("Module Enable");
            cidType = CID_TYPE_MODULE_ENABLE;
            break;
        case CID_POWER_SUPPLY_ENABLE:
            printf("Power Supply Enable");
            cidType = CID_TYPE_POWER_SUPPLY_ENABLE;
            break;
        case CID_RCA_ENABLE:
            printf("RCA Enable");
            cidType = CID_TYPE_RCA_ENABLE;
            break;
        case CID_SOFT_START:
            printf("Soft Start");
            cidType = CID_TYPE_SOFT_START;
            break;
        case CID_UNDERVOLTAGE_THRESHOLD:
            printf("Undervoltage Threshold");
            cidType = CID_TYPE_UNDERVOLTAGE_THRESHOLD;
            break;
        case CID_OVERVOLTAGE_THRESHOLD:
            printf("Overvoltage Threshold");
            cidType = CID_TYPE_OVERVOLTAGE_THRESHOLD;
            break;
        case CID_BT_WIFI_RESERVED:
            printf("Bluetooth/WiFi Reserved Command");
            cidType = CID_TYPE_BT_WIFI_RESERVED;
            break;

        // Protection and diagnostics CIDs
        case CID_MODULE_STATUS:
            printf("Module Status");
            cidType = CID_TYPE_MODULE_STATUS;
            break;
        case CID_CHANNEL_CLIP:
            printf("Channel Clip Detection");
            cidType = CID_TYPE_CHANNEL_CLIP;
            break;
        case CID_CHANNEL_SHORT:
            printf("Channel Short Detection");
            cidType = CID_TYPE_CHANNEL_SHORT;
            break;
        case CID_CHANNEL_OPEN:
            printf("Channel Open Detection");
            cidType = CID_TYPE_CHANNEL_OPEN;
            break;
        case CID_SHARC_STATUS:
            printf("SHARC Status");
            cidType = CID_TYPE_SHARC_STATUS;
            break;

        default:
            printf("Unknown/Custom Command");
            cidType = 0;
            break;
    }
    
    // Print message type
    printf(" (Type: ");
    if (cidType == CID_TYPE_S) {
        printf("S - Sender Only");
    } else if (cidType == CID_TYPE_R) {
        printf("R - Receiver Only");
    } else if (cidType == CID_TYPE_RS) {
        printf("R/S - Both Sender and Receiver");
    } else {
        printf("Unknown");
    }
    printf(")\n");
}

// Function to generate default data based on CID
void generateDefaultData(uint8_t cid, uint8_t* data, uint8_t* length, char* comment) {
    // Clear comment by default
    comment[0] = '\0';
    
    switch (cid) {
        // Network generic CIDs
        case CID_NETWORK_STARTUP:
            *length = 0;
            strcpy(comment, "Initializing network startup procedure");
            break;
        case CID_RESPONSE:
            data[0] = 0x32;  // Echo CID (e.g., EQ_BASS)
            data[1] = 0x00;  // Status: Success
            *length = 2;
            strcpy(comment, "Response to EQ_BASS command");
            break;
        case CID_COMMUNICATION_ERROR:
            data[0] = 0x01;  // Error type: Error at Sender
            *length = 1;
            strcpy(comment, "Reporting communication error at sender");
            break;

        // Audio control CIDs
        case CID_AUDIO_VIDEO_SOURCE_NAME:
            // String data example (e.g., "USB")
            data[0] = 'U';
            data[1] = 'S';
            data[2] = 'B';
            *length = 3;
            strcpy(comment, "Setting audio source name to 'USB'");
            break;
        case CID_AUDIO_SOURCE_TYPE:
            data[0] = 0x06;  // USB
            data[1] = 0x01;  // Capabilities: Play
            data[2] = 0x00;
            data[3] = 0x00;
            data[4] = 0x00;
            *length = 5;
            strcpy(comment, "Setting audio source type to USB with play capability");
            break;
        case CID_PLAY_STATUS:
            data[0] = 0x00;  // Play
            *length = 1;
            strcpy(comment, "Setting play status to PLAY");
            break;
        case CID_ZONE_VOLUME:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x50;  // 80% volume
            *length = 2;
            strcpy(comment, "Setting Zone 1 volume to 80%");
            break;
        case CID_ZONE_VOLUME_STEP:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Step: Up=0, Down=1
            *length = 2;
            strcpy(comment, "Decreasing volume for Zone 1");
            break;
        case CID_MUTE_ZONE:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Mute on
            *length = 2;
            strcpy(comment, "Muting Zone 1");
            break;
        case CID_MUTE_CHANNELS:
            data[0] = 0x55;  // Mute pattern (binary: 01010101 00000000)
            data[1] = 0x00;
            *length = 2;
            strcpy(comment, "Muting channels 1, 3, 5, 7");
            break;
        case CID_ELAPSED_TRACK_TIME:
            data[0] = 0x00;
            data[1] = 0x00;
            data[2] = 0x01;
            data[3] = 0x2C;  // 300 seconds
            *length = 4;
            strcpy(comment, "Reporting elapsed track time: 5 minutes");
            break;
        case CID_TRACK_TIME:
            data[0] = 0x00;
            data[1] = 0x00;
            data[2] = 0x03;
            data[3] = 0xE8;  // 1000 seconds
            *length = 4;
            strcpy(comment, "Reporting total track time: 16 minutes 40 seconds");
            break;
        case CID_REPEAT_SUPPORT:
            data[0] = 0x31;  // Supported: Song+Queue (3), Status: One (1)
            *length = 1;
            strcpy(comment, "Setting repeat mode: One track");
            break;
        case CID_SHUFFLE_SUPPORT:
            data[0] = 0x31;  // Supported: Queue+All (3), Status: Queue (1)
            *length = 1;
            strcpy(comment, "Setting shuffle mode: Play Queue");
            break;

        // Audio inputs CIDs
        case CID_LIBRARY_DATA_TYPE:
            data[0] = 0x04;  // Artist Name
            *length = 1;
            strcpy(comment, "Setting library data type to Artist Name");
            break;
        case CID_LIBRARY_DATA_NAME:
            // String data example
            data[0] = 'T';
            data[1] = 'e';
            data[2] = 's';
            data[3] = 't';
            *length = 4;
            strcpy(comment, "Setting library name to 'Test'");
            break;
        case CID_ARTIST_NAME:
            // String data example
            data[0] = 'A';
            data[1] = 'r';
            data[2] = 't';
            data[3] = 'i';
            data[4] = 's';
            data[5] = 't';
            *length = 6;
            strcpy(comment, "Setting artist name to 'Artist'");
            break;
        case CID_ALBUM_NAME:
            // String data example
            data[0] = 'A';
            data[1] = 'l';
            data[2] = 'b';
            data[3] = 'u';
            data[4] = 'm';
            *length = 5;
            strcpy(comment, "Setting album name to 'Album'");
            break;
        case CID_STATION_NAME:
            // String data example
            data[0] = 'R';
            data[1] = 'a';
            data[2] = 'd';
            data[3] = 'i';
            data[4] = 'o';
            *length = 5;
            strcpy(comment, "Setting station name to 'Radio'");
            break;
        case CID_POWER:
            data[0] = 0x01;  // On
            *length = 1;
            strcpy(comment, "Setting power state to ON");
            break;
        case CID_TOTAL_ZONES:
            data[0] = 0x04;  // 4 zones
            *length = 1;
            strcpy(comment, "Reporting total of 4 zones");
            break;
        case CID_ZONE_NAME:
            // String data example
            data[0] = 'F';
            data[1] = 'r';
            data[2] = 'o';
            data[3] = 'n';
            data[4] = 't';
            *length = 5;
            strcpy(comment, "Setting zone name to 'Front'");
            break;

        // Equalization CIDs
        case CID_MAIN_SUB_SWITCHING:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Enable
            *length = 2;
            strcpy(comment, "Enabling Main/Sub for Zone 1");
            break;
        case CID_EQ_PRESET_NAME:
            // String data example
            data[0] = 'R';
            data[1] = 'o';
            data[2] = 'c';
            data[3] = 'k';
            *length = 4;
            strcpy(comment, "Setting EQ preset to 'Rock'");
            break;
        case CID_EQ_BASS:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x14;  // +20% bass
            *length = 2;
            strcpy(comment, "Setting bass to +20% for Zone 1");
            break;
        case CID_EQ_TREBLE:
            data[0] = 0x01;  // Zone 1
            data[1] = 0xEC;  // -20% treble
            *length = 2;
            strcpy(comment, "Setting treble to -20% for Zone 1");
            break;
        case CID_EQ_MID_RANGE:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x00;  // 0% (flat)
            *length = 2;
            strcpy(comment, "Setting mid-range to flat (0%) for Zone 1");
            break;
        case CID_BALANCE:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x0A;  // +10% (right)
            *length = 2;
            strcpy(comment, "Setting balance to +10% (right) for Zone 1");
            break;
        case CID_FADE:
            data[0] = 0x01;  // Zone 1
            data[1] = 0xF6;  // -10% (rear)
            *length = 2;
            strcpy(comment, "Setting fade to -10% (rear) for Zone 1");
            break;
        case CID_SUB_VOLUME:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x50;  // 80% sub volume
            *length = 2;
            strcpy(comment, "Setting subwoofer volume to 80% for Zone 1");
            break;
        case CID_SUBWOOFER_SWITCH:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Enable
            *length = 2;
            strcpy(comment, "Enabling subwoofer for Zone 1");
            break;
        case CID_CENTER_SWITCH:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Enable
            *length = 2;
            strcpy(comment, "Enabling center channel for Zone 1");
            break;
        case CID_TONE_BATCH:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Enable
            *length = 2;
            strcpy(comment, "Enabling tone batch for Zone 1");
            break;
        case CID_BEEP_VOLUME:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x32;  // 50% volume
            *length = 2;
            strcpy(comment, "Setting beep volume to 50% for Zone 1");
            break;
        case CID_SPEED_COMP:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x81;  // Enable with speed 1
            *length = 2;
            strcpy(comment, "Enabling speed compensation (level 1) for Zone 1");
            break;
        case CID_OVERHEAD_SWITCH:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Enable
            *length = 2;
            strcpy(comment, "Enabling overhead speakers for Zone 1");
            break;
        case CID_ANC_ZONE_ENABLE:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Enable
            *length = 2;
            strcpy(comment, "Enabling ANC for Zone 1");
            break;
        case CID_BEEP:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Beep on
            *length = 2;
            strcpy(comment, "Enabling beep for Zone 1");
            break;
        case CID_VOICE_OUTPUT:
            data[0] = 0x01;  // Enable
            *length = 1;
            strcpy(comment, "Enabling voice output");
            break;

        // Bluetooth related CIDs
        case CID_BT_ADDR_AVAILABLE:
            data[0] = 0x02;  // 2 addresses available
            *length = 1;
            strcpy(comment, "Reporting 2 Bluetooth addresses available");
            break;
        case CID_BT_DEVICE_ADDR:
            data[0] = 0x00;  // Index 0
            data[1] = 0x11;
            data[2] = 0x22;
            data[3] = 0x33;
            data[4] = 0x44;
            data[5] = 0x55;
            data[6] = 0x66;
            *length = 7;
            strcpy(comment, "Setting Bluetooth device address for device 0");
            break;
        case CID_BT_DEVICE_STATUS:
            data[0] = 0x00;  // Index 0
            data[1] = 0x00;  // Connected
            *length = 2;
            strcpy(comment, "Reporting device 0 is connected");
            break;
        case CID_BT_DEVICE_NAME:
            data[0] = 0x00;  // Index 0
            data[1] = 'M';
            data[2] = 'y';
            data[3] = ' ';
            data[4] = 'P';
            data[5] = 'h';
            data[6] = 'o';
            data[7] = 'n';
            data[8] = 'e';
            *length = 9;
            strcpy(comment, "Setting Bluetooth device name to 'My Phone'");
            break;
        case CID_BT_PAIRING_STATUS:
            data[0] = 0x00;  // Index 0
            data[1] = 0x01;  // Connect
            *length = 2;
            strcpy(comment, "Setting pairing status to Connect for device 0");
            break;
        case CID_BT_FORGET_DEVICE:
            data[0] = 0x00;  // Index 0
            data[1] = 0x01;  // Yes/Enabled
            *length = 2;
            strcpy(comment, "Forgetting Bluetooth device 0");
            break;
        case CID_BT_DISCOVERING:
            data[0] = 0x01;  // Yes/Enabled
            *length = 1;
            strcpy(comment, "Enabling Bluetooth discovery mode");
            break;

        // Audio configuration and PLL Lock CIDs
        case CID_CHANNEL_SLOT:
            data[0] = 0x23;  // Channel 2, Slot 3
            *length = 1;
            strcpy(comment, "Assigning Channel 2 to Slot 3");
            break;
        case CID_PLL_LOCK_STATUS:
            data[0] = 0xFF;  // All PLLs locked
            *length = 1;
            strcpy(comment, "Reporting all PLLs locked");
            break;
        case CID_INPUT_VOLTAGE:
            data[0] = 0x12;  // 1.2V (example)
            data[1] = 0x34;
            *length = 2;
            strcpy(comment, "Reporting input voltage of 1.2V");
            break;
        case CID_INPUT_CURRENT:
            data[0] = 0x05;  // 0.5A (example)
            data[1] = 0x00;
            *length = 2;
            strcpy(comment, "Reporting input current of 0.5A");
            break;
        case CID_TEMPERATURE:
            data[0] = 0x28;  // 40°C
            *length = 1;
            strcpy(comment, "Reporting temperature of 40°C");
            break;
        case CID_SENSOR_GENERIC:
            data[0] = 0x12;  // Sensor ID 1, value 2
            data[1] = 0x34;  // More data
            *length = 2;
            strcpy(comment, "Reporting generic sensor data");
            break;

        // Module configuration CIDs
        case CID_MODULE_ENABLE:
            data[0] = 0x01;  // Enable
            *length = 1;
            strcpy(comment, "Enabling module");
            break;
        case CID_POWER_SUPPLY_ENABLE:
            data[0] = 0x01;  // Enable
            *length = 1;
            strcpy(comment, "Enabling power supply");
            break;
        case CID_RCA_ENABLE:
            data[0] = 0x01;  // Enable
            *length = 1;
            strcpy(comment, "Enabling RCA input");
            break;
        case CID_SOFT_START:
            data[0] = 0x32;  // Slew rate value
            *length = 1;
            strcpy(comment, "Setting soft start slew rate to 50");
            break;
        case CID_UNDERVOLTAGE_THRESHOLD:
            data[0] = 0x80;  // Threshold value
            *length = 1;
            strcpy(comment, "Setting undervoltage threshold to 128");
            break;
        case CID_OVERVOLTAGE_THRESHOLD:
            data[0] = 0xC0;  // Threshold value
            *length = 1;
            strcpy(comment, "Setting overvoltage threshold to 192");
            break;
        case CID_BT_WIFI_RESERVED:
            data[0] = 0x01;  // Reserved data
            *length = 1;
            strcpy(comment, "Sending reserved Bluetooth/WiFi command");
            break;

        // Protection and diagnostics CIDs
        case CID_MODULE_STATUS:
            data[0] = 0x81;  // Enable=1, Voltage Level=0, OVP=0, UVP=1
            data[1] = 0x48;  // OCP=0, Temperature=9, Thermal FB=0, Thermal SD=0
            *length = 2;
            strcpy(comment, "Reporting module status: Enabled, UVP active, Temperature level 9");
            break;
        case CID_CHANNEL_CLIP:
            data[0] = 0x00;  // High byte of bitmap
            data[1] = 0x05;  // Low byte: channels 1 and 3 clipping
            *length = 2;
            strcpy(comment, "Reporting channel clip: Channels 1 and 3 clipping");
            break;
        case CID_CHANNEL_SHORT:
            data[0] = 0x00;  // High byte of bitmap
            data[1] = 0x10;  // Low byte: channel 5 short circuit
            *length = 2;
            strcpy(comment, "Reporting channel short: Channel 5 short circuit");
            break;
        case CID_CHANNEL_OPEN:
            data[0] = 0x00;  // High byte of bitmap
            data[1] = 0x20;  // Low byte: channel 6 open circuit
            *length = 2;
            strcpy(comment, "Reporting channel open: Channel 6 open circuit");
            break;
        case CID_SHARC_STATUS:
            data[0] = 0x01;  // Status OK
            *length = 1;
            strcpy(comment, "Reporting SHARC status: All systems OK");
            break;

        default:
            data[0] = 0x00;
            *length = 1;
            strcpy(comment, "Unknown command");
            break;
    }
}

// Helper function to display the main menu (categorized)
void displayMenu() {
    printf("\n====== AABCOP Protocol Sender ======\n");
    printf("Network generic commands (0x00-0x0F):\n");
    printf("  1. Network Startup/Validation (0x00)\n");
    printf("  2. Response Command (0x01)\n");
    printf("  3. Communication Error (0x02)\n");
    
    printf("\nAudio control commands (0x10-0x1F):\n");
    printf("  10. Audio/Video Source Name (0x10)\n");
    printf("  11. Audio Source Type (0x11)\n");
    printf("  12. Play Status (0x12)\n");
    printf("  13. Zone Volume (0x13)\n");
    printf("  14. Zone Volume Step (0x14)\n");
    printf("  15. Mute Zone (0x15)\n");
    printf("  16. Mute Channels (0x16)\n");
    printf("  17. Elapsed Track Time (0x17)\n");
    printf("  18. Track Time (0x18)\n");
    printf("  19. Repeat Support (0x19)\n");
    
    printf("\nEQ commands (0x30-0x3F):\n");
    printf("  30. Main/Sub Switching (0x30)\n");
    printf("  31. EQ Preset Name (0x31)\n");
    printf("  32. EQ Bass (0x32)\n");
    printf("  33. EQ Treble (0x33)\n");
    printf("  34. EQ Mid Range (0x34)\n");
    
    printf("\nConfiguration commands (0x60-0x6F):\n");
    printf("  60. Channel Slot Assignment (0x60)\n");
    printf("  61. PLL Lock Status (0x61)\n");
    
    printf("\nModule commands (0x70-0x8F):\n");
    printf("  70. Module Enable (0x70)\n");
    printf("  80. Module Status (0x80)\n");
    printf("  81. Channel Clip Detection (0x81)\n");
    printf("  82. Channel Short Detection (0x82)\n");
    printf("  83. Channel Open Detection (0x83)\n");
    printf("  84. SHARC Status (0x84)\n");
    
    printf("\nUtilities:\n");
    printf("  90. Show All Available CIDs\n");
    printf("  91. Send Custom Command\n");
    printf("   0. Exit\n");
    
    printf("Enter choice: ");
}

// Helper function to show all available CIDs
void showAllCIDs() {
    printf("\n===== All Available CIDs =====\n");
    
    printf("Network generic CIDs (0x00-0x0F):\n");
    printf("  0x00: Network Startup/Validation\n");
    printf("  0x01: Response Command\n");
    printf("  0x02: Communication Error\n");
    printf("  0x03-0x0F: Reserved\n");
    
    printf("\nAudio control CIDs (0x10-0x1F):\n");
    printf("  0x10: Audio/Video Source Name\n");
    printf("  0x11: Audio Source Type\n");
    printf("  0x12: Play Status\n");
    printf("  0x13: Zone Volume\n");
    printf("  0x14: Zone Volume Step\n");
    printf("  0x15: Mute Zone\n");
    printf("  0x16: Mute Channels\n");
    printf("  0x17: Elapsed Track Time\n");
    printf("  0x18: Track Time\n");
    printf("  0x19: Repeat Support\n");
    printf("  0x1A: Shuffle Support\n");
    printf("  0x1B-0x1F: Reserved\n");
    
    printf("\nAudio inputs CIDs (0x20-0x2F):\n");
    printf("  0x20: Library Data Type\n");
    printf("  0x21: Library Data Name\n");
    printf("  0x22: Artist Name\n");
    printf("  0x23: Album Name\n");
    printf("  0x24: Station Name\n");
    printf("  0x25-0x29: Reserved\n");
    printf("  0x2A: Power\n");
    printf("  0x2B: Total Number of Zones Available\n");
    printf("  0x2C: Zone Name\n");
    printf("  0x2D-0x2F: Reserved\n");
    
    printf("\nEqualization CIDs (0x30-0x3F):\n");
    printf("  0x30: Main/Sub Switching\n");
    printf("  0x31: EQ Preset Name\n");
    printf("  0x32: EQ Bass\n");
    printf("  0x33: EQ Treble\n");
    printf("  0x34: EQ Mid Range\n");
    printf("  0x35: Balance\n");
    printf("  0x36: Fade\n");
    printf("  0x37: Non-Fader, Sub Volume\n");
    printf("  0x38: Subwoofer Direct Switching\n");
    printf("  0x39: Center Direct Switching\n");
    printf("  0x3A: Tone Batch Direct Switching\n");
    printf("  0x3B: Beep Volume Direct Switching\n");
    printf("  0x3C: Speed Compensation\n");
    printf("  0x3D: Overhead Direct Switching\n");
    printf("  0x3E: ANC Zone Enable\n");
    printf("  0x3F: Beep\n");
    
    printf("\nVoice & Bluetooth CIDs (0x40-0x56):\n");
    printf("  0x40: Voice Output\n");
    printf("  0x41-0x45: Reserved\n");
    printf("  0x46: Number of Bluetooth Addresses Available\n");
    printf("  0x47: Bluetooth Device Address\n");
    printf("  0x48: Bluetooth Device Status\n");
    printf("  0x49: Bluetooth Device Name\n");
    printf("  0x50: Bluetooth Pairing Status\n");
    printf("  0x51: Forget Bluetooth Device\n");
    printf("  0x52-0x55: Reserved\n");
    printf("  0x56: Discovering\n");
    printf("  0x57-0x5F: Reserved\n");
    
    printf("\nAudio configuration and PLL Lock CIDs (0x60-0x6F):\n");
    printf("  0x60: Channel Slot Assignment\n");
    printf("  0x61: PLL Lock Status\n");
    printf("  0x62-0x65: Reserved\n");
    printf("  0x66: Input Voltage\n");
    printf("  0x67: Input Current\n");
    printf("  0x68: Temperature Input Filter\n");
    printf("  0x69: Sensor Generic\n");
    printf("  0x6A-0x6F: Reserved\n");
    
    printf("\nModule configuration CIDs (0x70-0x7F):\n");
    printf("  0x70: Module Enable\n");
    printf("  0x71: Power Supply Enable\n");
    printf("  0x72: RCA Enable\n");
    printf("  0x73: Soft Start\n");
    printf("  0x74: Undervoltage Threshold\n");
    printf("  0x75: Overvoltage Threshold\n");
    printf("  0x76: Reserved for Bluetooth and Wi-Fi\n");
    printf("  0x77-0x7F: Reserved\n");
    
    printf("\nProtection and diagnostics CIDs (0x80-0x8F):\n");
    printf("  0x80: Module Status\n");
    printf("  0x81: Channel Clip Detection\n");
    printf("  0x82: Channel Short Detection\n");
    printf("  0x83: Channel Open Detection\n");
    printf("  0x84: SHARC Status\n");
    printf("  0x85-0x8F: Reserved\n");
    
    printf("\nPress Enter to return to main menu...");
    getchar();  // Wait for Enter key
}

// Function to get CID type
uint8_t getCIDType(uint8_t cid) {
    uint8_t cidType = 0;
    
    // Get CID type based on CID value
    switch (cid) {
        case CID_NETWORK_STARTUP: cidType = CID_TYPE_NETWORK_STARTUP; break;
        case CID_RESPONSE: cidType = CID_TYPE_RESPONSE; break;
        case CID_COMMUNICATION_ERROR: cidType = CID_TYPE_COMMUNICATION_ERROR; break;
        case CID_AUDIO_VIDEO_SOURCE_NAME: cidType = CID_TYPE_AUDIO_VIDEO_SOURCE_NAME; break;
        case CID_AUDIO_SOURCE_TYPE: cidType = CID_TYPE_AUDIO_SOURCE_TYPE; break;
        case CID_PLAY_STATUS: cidType = CID_TYPE_PLAY_STATUS; break;
        case CID_ZONE_VOLUME: cidType = CID_TYPE_ZONE_VOLUME; break;
        case CID_ZONE_VOLUME_STEP: cidType = CID_TYPE_ZONE_VOLUME_STEP; break;
        case CID_MUTE_ZONE: cidType = CID_TYPE_MUTE_ZONE; break;
        case CID_MUTE_CHANNELS: cidType = CID_TYPE_MUTE_CHANNELS; break;
        case CID_ELAPSED_TRACK_TIME: cidType = CID_TYPE_ELAPSED_TRACK_TIME; break;
        case CID_TRACK_TIME: cidType = CID_TYPE_TRACK_TIME; break;
        case CID_REPEAT_SUPPORT: cidType = CID_TYPE_REPEAT_SUPPORT; break;
        case CID_SHUFFLE_SUPPORT: cidType = CID_TYPE_SHUFFLE_SUPPORT; break;
        case CID_LIBRARY_DATA_TYPE: cidType = CID_TYPE_LIBRARY_DATA_TYPE; break;
        case CID_LIBRARY_DATA_NAME: cidType = CID_TYPE_LIBRARY_DATA_NAME; break;
        case CID_ARTIST_NAME: cidType = CID_TYPE_ARTIST_NAME; break;
        case CID_ALBUM_NAME: cidType = CID_TYPE_ALBUM_NAME; break;
        case CID_STATION_NAME: cidType = CID_TYPE_STATION_NAME; break;
        case CID_POWER: cidType = CID_TYPE_POWER; break;
        case CID_TOTAL_ZONES: cidType = CID_TYPE_TOTAL_ZONES; break;
        case CID_ZONE_NAME: cidType = CID_TYPE_ZONE_NAME; break;
        case CID_MAIN_SUB_SWITCHING: cidType = CID_TYPE_MAIN_SUB_SWITCHING; break;
        case CID_EQ_PRESET_NAME: cidType = CID_TYPE_EQ_PRESET_NAME; break;
        case CID_EQ_BASS: cidType = CID_TYPE_EQ_BASS; break;
        case CID_EQ_TREBLE: cidType = CID_TYPE_EQ_TREBLE; break;
        case CID_EQ_MID_RANGE: cidType = CID_TYPE_EQ_MID_RANGE; break;
        case CID_BALANCE: cidType = CID_TYPE_BALANCE; break;
        case CID_FADE: cidType = CID_TYPE_FADE; break;
        case CID_SUB_VOLUME: cidType = CID_TYPE_SUB_VOLUME; break;
        case CID_SUBWOOFER_SWITCH: cidType = CID_TYPE_SUBWOOFER_SWITCH; break;
        case CID_CENTER_SWITCH: cidType = CID_TYPE_CENTER_SWITCH; break;
        case CID_TONE_BATCH: cidType = CID_TYPE_TONE_BATCH; break;
        case CID_BEEP_VOLUME: cidType = CID_TYPE_BEEP_VOLUME; break;
        case CID_SPEED_COMP: cidType = CID_TYPE_SPEED_COMP; break;
        case CID_OVERHEAD_SWITCH: cidType = CID_TYPE_OVERHEAD_SWITCH; break;
        case CID_ANC_ZONE_ENABLE: cidType = CID_TYPE_ANC_ZONE_ENABLE; break;
        case CID_BEEP: cidType = CID_TYPE_BEEP; break;
        case CID_VOICE_OUTPUT: cidType = CID_TYPE_VOICE_OUTPUT; break;
        case CID_BT_ADDR_AVAILABLE: cidType = CID_TYPE_BT_ADDR_AVAILABLE; break;
        case CID_BT_DEVICE_ADDR: cidType = CID_TYPE_BT_DEVICE_ADDR; break;
        case CID_BT_DEVICE_STATUS: cidType = CID_TYPE_BT_DEVICE_STATUS; break;
        case CID_BT_DEVICE_NAME: cidType = CID_TYPE_BT_DEVICE_NAME; break;
        case CID_BT_PAIRING_STATUS: cidType = CID_TYPE_BT_PAIRING_STATUS; break;
        case CID_BT_FORGET_DEVICE: cidType = CID_TYPE_BT_FORGET_DEVICE; break;
        case CID_BT_DISCOVERING: cidType = CID_TYPE_BT_DISCOVERING; break;
        case CID_CHANNEL_SLOT: cidType = CID_TYPE_CHANNEL_SLOT; break;
        case CID_PLL_LOCK_STATUS: cidType = CID_TYPE_PLL_LOCK_STATUS; break;
        case CID_INPUT_VOLTAGE: cidType = CID_TYPE_INPUT_VOLTAGE; break;
        case CID_INPUT_CURRENT: cidType = CID_TYPE_INPUT_CURRENT; break;
        case CID_TEMPERATURE: cidType = CID_TYPE_TEMPERATURE; break;
        case CID_SENSOR_GENERIC: cidType = CID_TYPE_SENSOR_GENERIC; break;
        case CID_MODULE_ENABLE: cidType = CID_TYPE_MODULE_ENABLE; break;
        case CID_POWER_SUPPLY_ENABLE: cidType = CID_TYPE_POWER_SUPPLY_ENABLE; break;
        case CID_RCA_ENABLE: cidType = CID_TYPE_RCA_ENABLE; break;
        case CID_SOFT_START: cidType = CID_TYPE_SOFT_START; break;
        case CID_UNDERVOLTAGE_THRESHOLD: cidType = CID_TYPE_UNDERVOLTAGE_THRESHOLD; break;
        case CID_OVERVOLTAGE_THRESHOLD: cidType = CID_TYPE_OVERVOLTAGE_THRESHOLD; break;
        case CID_BT_WIFI_RESERVED: cidType = CID_TYPE_BT_WIFI_RESERVED; break;
        case CID_MODULE_STATUS: cidType = CID_TYPE_MODULE_STATUS; break;
        case CID_CHANNEL_CLIP: cidType = CID_TYPE_CHANNEL_CLIP; break;
        case CID_CHANNEL_SHORT: cidType = CID_TYPE_CHANNEL_SHORT; break;
        case CID_CHANNEL_OPEN: cidType = CID_TYPE_CHANNEL_OPEN; break;
        case CID_SHARC_STATUS: cidType = CID_TYPE_SHARC_STATUS; break;
        default: cidType = 0; break;
    }
    
    return cidType;
}

// Function to check if the CID is a sender type (S or R/S)
int isSenderCID(uint8_t cid) {
    uint8_t cidType = getCIDType(cid);
    
    // Check if CID is sender type (S or R/S)
    return (cidType == CID_TYPE_S || cidType == CID_TYPE_RS);
}

// Function to receive and process response from receiver
void receiveAndProcessResponse(SOCKET sock) {
    uint8_t response[MAILBOX_SIZE];
    int bytesReceived = recv(sock, (char*)response, MAILBOX_SIZE, 0);
    
    if (bytesReceived == MAILBOX_SIZE) {
        printf("\n=== Received Message from Receiver ===\n");
        printf("Received Mailbox: ");
        for (int i = 0; i < MAILBOX_SIZE; i++) {
            printf("0x%02X ", response[i]);
        }
        printf("\n");
        
        // Process the received message
        uint8_t frameType = (response[0] >> 7) & 0x01;
        
        if (frameType == 0) {  // Start frame
            uint8_t dataSize = response[0] & 0x7F;
            uint8_t cid = response[1];
            
            printf("Command ID (CID): 0x%02X - ", cid);
            printCIDDescription(cid);
            
            if (dataSize > 0) {
                printf("Data: ");
                for (int i = 2; i < MAILBOX_SIZE && i < dataSize + 2; i++) {
                    printf("0x%02X ", response[i]);
                }
                printf("\n");
            }
        }
        printf("===================================\n\n");
    } else if (bytesReceived > 0) {
        printf("Received partial response (%d bytes)\n", bytesReceived);
    } else if (bytesReceived == 0) {
        printf("Connection closed by the receiver\n");
    } else {
        printf("Error receiving response: %d\n", WSAGetLastError());
    }
}

int main() {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in server;
    uint8_t data[MAX_DATA_SIZE];
    uint8_t UID = UID_DANG8100;  // Default UID for DANG8100
    char comment[MAX_COMMENT_SIZE] = {0};

    // Initialize Winsock
    printf("Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket. Error: %d\n", WSAGetLastError());
        return 1;
    }

    // Set up server address
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(12345);

    // Connect to server
    printf("Attempting to connect to the receiver...\n");
    if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Connection failed. Is the receiver running?\n");
        WSACleanup();
        return 1;
    }

    printf("Connected to receiver.\n");
    
    int choice;
    uint8_t cid, dataLength;
    char dataLine[512];
    
    do {
        displayMenu();
        scanf("%d", &choice);
        
        if (choice == 0) break;
        
        if (choice == 90) {
            getchar();  // Consume newline
            showAllCIDs();
            continue;
        }
        
        // Map menu choice to CID
        if (choice == 91) {
            printf("Enter custom CID (hex, e.g. A1): ");
            scanf("%hhx", &cid);
        } else {
            // Convert menu number to actual CID
            switch (choice) {
                case 1: cid = CID_NETWORK_STARTUP; break;
                case 2: cid = CID_RESPONSE; break;
                case 3: cid = CID_COMMUNICATION_ERROR; break;
                case 10: cid = CID_AUDIO_VIDEO_SOURCE_NAME; break;
                case 11: cid = CID_AUDIO_SOURCE_TYPE; break;
                case 12: cid = CID_PLAY_STATUS; break;
                case 13: cid = CID_ZONE_VOLUME; break;
                case 14: cid = CID_ZONE_VOLUME_STEP; break;
                case 15: cid = CID_MUTE_ZONE; break;
                case 16: cid = CID_MUTE_CHANNELS; break;
                case 17: cid = CID_ELAPSED_TRACK_TIME; break;
                case 18: cid = CID_TRACK_TIME; break;
                case 19: cid = CID_REPEAT_SUPPORT; break;
                case 30: cid = CID_MAIN_SUB_SWITCHING; break;
                case 31: cid = CID_EQ_PRESET_NAME; break;
                case 32: cid = CID_EQ_BASS; break;
                case 33: cid = CID_EQ_TREBLE; break;
                case 34: cid = CID_EQ_MID_RANGE; break;
                case 60: cid = CID_CHANNEL_SLOT; break;
                case 61: cid = CID_PLL_LOCK_STATUS; break;
                case 70: cid = CID_MODULE_ENABLE; break;
                case 80: cid = CID_MODULE_STATUS; break;
                case 81: cid = CID_CHANNEL_CLIP; break;
                case 82: cid = CID_CHANNEL_SHORT; break;
                case 83: cid = CID_CHANNEL_OPEN; break;
                case 84: cid = CID_SHARC_STATUS; break;
                default:
                    printf("Invalid choice. Please try again.\n");
                    continue;
            }
        }
        
        printCIDDescription(cid);
        
        // Check CID type
        uint8_t cidType = getCIDType(cid);
        if (cidType == 0) {
            printf("Warning: Unknown CID type.\n");
            printf("Do you still want to send it? (y/n): ");
            char confirm;
            scanf(" %c", &confirm);
            if (confirm != 'y' && confirm != 'Y') {
                continue;
            }
        }
        else if (cidType == CID_TYPE_R) {
            printf("Warning: This CID is a receiver type (R). It's typically sent from receiver to sender.\n");
            printf("Do you still want to send it? (y/n): ");
            char confirm;
            scanf(" %c", &confirm);
            if (confirm != 'y' && confirm != 'Y') {
                continue;
            }
        }
        
        // Generate default data based on CID or get custom data
        printf("Use default data for this CID? (y/n): ");
        char useDefault;
        scanf(" %c", &useDefault);
        
        if (useDefault == 'y' || useDefault == 'Y') {
            generateDefaultData(cid, data, &dataLength, comment);
            
            printf("Using default data: ");
            for (int i = 0; i < dataLength; i++) {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
            printf("Default comment: %s\n", comment);
        } else {
            printf("Enter data (hex, space-separated, e.g., 01 A2 FF):\n");
            getchar();  // consume newline
            fgets(dataLine, sizeof(dataLine), stdin);
            
            char* token = strtok(dataLine, " \t\n");
            dataLength = 0;
            
            while (token && dataLength < MAX_DATA_SIZE) {
                if (sscanf(token, "%hhx", &data[dataLength]) == 1) {
                    dataLength++;
                }
                token = strtok(NULL, " \t\n");
            }
            
            if (dataLength == 0) {
                printf("No valid data entered. Using empty data.\n");
            }
            
            printf("Enter comment for this message:\n");
            fgets(comment, sizeof(comment), stdin);
            comment[strcspn(comment, "\n")] = 0;  // Remove trailing newline
        }
        
        // Create and send the packet
        AppPacket packet = {
            .UID = UID,
            .CID = cid,
            .Data = data,
            .L_SIZE = dataLength
        };
        strncpy(packet.Comment, comment, MAX_COMMENT_SIZE - 1);
        
        printf("Sending packet with CID: 0x%02X, Data length: %d\n", cid, dataLength);
        CreateA2BFrames(&packet, s);
        printf("Packet sent successfully!\n");
        
        // Check if we should expect a response (for R/S type CIDs)
        if (cidType == CID_TYPE_RS) {
            printf("Waiting for response from receiver (CID type is R/S)...\n");
            
            // Add a small delay to allow the receiver to process and respond
            Sleep(500);
            
            // Non-blocking check for response
            fd_set readSet;
            struct timeval timeout;
            
            FD_ZERO(&readSet);
            FD_SET(s, &readSet);
            
            timeout.tv_sec = 2;  // 2 seconds timeout
            timeout.tv_usec = 0;
            
            int ready = select(0, &readSet, NULL, NULL, &timeout);
            
            if (ready > 0) {
                receiveAndProcessResponse(s);
            } else if (ready == 0) {
                printf("No response received within timeout period\n");
            } else {
                printf("Error in select(): %d\n", WSAGetLastError());
            }
        }
        
    } while (choice != 0);
    
    // Clean up
    closesocket(s);
    WSACleanup();
    
    printf("Program terminated.\n");
    return 0;
}