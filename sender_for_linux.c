#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> // For bool, true, false

// Linux/POSIX Socket Headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // For close, usleep
#include <errno.h>  // For errno

#include "aabcop_new.h"

// Constants
#define MAX_DATA_SIZE     128
#define MAX_COMMENT_SIZE  256

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

// Callback function for writing bytes to socket
void socket_write_callback(uint8_t byte, void* ctx) {
    int sock = *((int*)ctx); // Changed SOCKET to int
    Mailbox[MailboxIndex++] = byte;

    if (MailboxIndex == MAILBOX_SIZE) {
        if (send(sock, (const char*)Mailbox, MAILBOX_SIZE, 0) < 0) {
            perror("Send Mailbox error");
        } else {
            printf("Sent Mailbox: ");
            for (int i = 0; i < MAILBOX_SIZE; i++) {
                printf("0x%02X ", Mailbox[i]);
            }
            printf("\n");
        }
        MailboxIndex = 0;
    }
}

// Function to create A2B frames from an application packet and send via socket
void CreateA2BFrames(AppPacket* packet, int sock) { // Changed SOCKET to int
    uint8_t L = packet->L_SIZE;

    // Print message details
    printf("\n=== Sending Message Details ===\n");
    printf("UID: 0x%02X\n", packet->UID);
    printf("CID: 0x%02X - ", packet->CID);
    aabcop_print_cid_description(packet->CID);
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

    // Use the library function to create and send frames
    aabcop_create_frames(packet->UID, packet->CID, packet->Data, L, socket_write_callback, &sock);

    // Flush any remaining data
    if (MailboxIndex > 0) {
        // Pad remaining bytes with zeros
        while (MailboxIndex < MAILBOX_SIZE) {
            Mailbox[MailboxIndex++] = 0x00;
        }

        if (send(sock, (const char*)Mailbox, MAILBOX_SIZE, 0) < 0) {
            perror("Send Mailbox (final) error");
        } else {
            printf("Sent Mailbox (final): ");
            for (int i = 0; i < MAILBOX_SIZE; i++) {
                printf("0x%02X ", Mailbox[i]);
            }
            printf("\n");
        }
        MailboxIndex = 0;
    }
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
            data[0] = 'T';
            data[1] = 'e';
            data[2] = 's';
            data[3] = 't';
            *length = 4;
            strcpy(comment, "Setting library name to 'Test'");
            break;
        case CID_ARTIST_NAME:
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
            data[0] = 'A';
            data[1] = 'l';
            data[2] = 'b';
            data[3] = 'u';
            data[4] = 'm';
            *length = 5;
            strcpy(comment, "Setting album name to 'Album'");
            break;
        case CID_STATION_NAME:
            data[0] = 'R';
            data[1] = 'a';
            data[2] = 'd';
            data[3] = 'i';
            data[4] = 'o';
            *length = 5;
            strcpy(comment, "Setting station name to 'Radio'");
            break;
        case CID_NODE_ENABLE:
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
            data[1] = 0x05;  // Medium compensation level
            *length = 2;
            strcpy(comment, "Setting speed compensation to level 5 for Zone 1");
            break;
        case CID_OVERHEAD_SWITCH:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Enable
            *length = 2;
            strcpy(comment, "Enabling overhead switch for Zone 1");
            break;
        case CID_ANC_ZONE_ENABLE:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Enable
            *length = 2;
            strcpy(comment, "Enabling ANC for Zone 1");
            break;
        case CID_BEEP:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Tone ID
            data[2] = 0x01;  // Volume level
            *length = 3;
            strcpy(comment, "Triggering beep tone 1 at volume level 1 for Zone 1");
            break;

        // Voice and Bluetooth related CIDs
        case CID_VOICE_OUTPUT:
            data[0] = 0x01;  // Zone 1
            data[1] = 0x01;  // Enable
            *length = 2;
            strcpy(comment, "Enabling voice output for Zone 1");
            break;
        case CID_BT_ADDR_AVAILABLE:
            data[0] = 0x03;  // 3 BT addresses available
            *length = 1;
            strcpy(comment, "Reporting 3 Bluetooth addresses available");
            break;
        case CID_BT_DEVICE_ADDR:
            data[0] = 0x01;  // Device index
            data[1] = 0x12;  // MAC address (simplified)
            data[2] = 0x34;
            data[3] = 0x56;
            data[4] = 0x78;
            data[5] = 0x9A;
            data[6] = 0xBC;
            *length = 7;
            strcpy(comment, "Setting Bluetooth device address for device 1");
            break;
        case CID_BT_DEVICE_STATUS:
            data[0] = 0x01;  // Device index
            data[1] = 0x02;  // Connected
            *length = 2;
            strcpy(comment, "Setting Bluetooth device 1 status to connected");
            break;
        case CID_BT_DEVICE_NAME:
            data[0] = 0x01;  // Device index
            data[1] = 'T';   // Device name "Test"
            data[2] = 'e';
            data[3] = 's';
            data[4] = 't';
            *length = 5;
            strcpy(comment, "Setting Bluetooth device 1 name to 'Test'");
            break;
        case CID_BT_PAIRING_STATUS:
            data[0] = 0x01;  // Start pairing
            *length = 1;
            strcpy(comment, "Starting Bluetooth pairing");
            break;
        case CID_BT_FORGET_DEVICE:
            data[0] = 0x01;  // Device index
            *length = 1;
            strcpy(comment, "Forgetting Bluetooth device 1");
            break;
        case CID_BT_DISCOVERING:
            data[0] = 0x01;  // Start discovering
            *length = 1;
            strcpy(comment, "Starting Bluetooth discovery");
            break;

        // Audio Configuration CIDs
        case CID_CHANNEL_SLOT:
            data[0] = 0x12;  // Channel 1, Slot 2
            *length = 1;
            strcpy(comment, "Assigning channel 1 to slot 2");
            break;
        case CID_PLL_LOCK_STATUS:
            data[0] = 0x01;  // Locked
            *length = 1;
            strcpy(comment, "Reporting PLL locked status");
            break;

        // Sensor CIDs
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
            data[0] = 0x01;  // Sensor ID
            data[1] = 0x42;  // Sensor value
            *length = 2;
            strcpy(comment, "Reporting generic sensor 1 value 0x42");
            break;

        // Module Configuration CIDs
        case CID_MODULE_ENABLE:
            data[0] = 0x01;  // Module ID
            data[1] = 0x01;  // Enable
            *length = 2;
            strcpy(comment, "Enabling module 1");
            break;
        case CID_POWER_SUPPLY_ENABLE:
            data[0] = 0x01;  // Enable
            *length = 1;
            strcpy(comment, "Enabling power supply");
            break;
        case CID_RCA_ENABLE:
            data[0] = 0x01;  // Enable
            *length = 1;
            strcpy(comment, "Enabling RCA");
            break;
        case CID_SOFT_START:
            data[0] = 0x01;  // Enable
            *length = 1;
            strcpy(comment, "Enabling soft start");
            break;
        case CID_UNDERVOLTAGE_THRESHOLD:
            data[0] = 0x0A;  // 10V
            *length = 1;
            strcpy(comment, "Setting undervoltage threshold to 10V");
            break;
        case CID_OVERVOLTAGE_THRESHOLD:
            data[0] = 0x10;  // 16V
            *length = 1;
            strcpy(comment, "Setting overvoltage threshold to 16V");
            break;
        case CID_BT_WIFI_RESERVED:
            data[0] = 0x01;  // Command ID
            data[1] = 0x01;  // Parameter
            *length = 2;
            strcpy(comment, "Sending BT/WiFi reserved command 1");
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
    printf("  3. Request Info (0x02)\n");
    printf("  4. Communication Error (0x03)\n");

    printf("\nAudio control commands (0x10-0x1F):\n");
    printf("  16. Audio/Video Source Name (0x10)\n");
    printf("  17. Audio Source Type (0x11)\n");
    printf("  18. Play Status (0x12)\n");
    printf("  19. Zone Volume (0x13)\n");
    printf("  20. Zone Volume Step (0x14)\n");
    printf("  21. Mute Zone (0x15)\n");
    printf("  22. Mute Channels (0x16)\n");
    printf("  23. Elapsed Track Time (0x17)\n");
    printf("  24. Track Time (0x18)\n");
    printf("  25. Repeat Support (0x19)\n");
    printf("  26. Shuffle Support (0x1A)\n");

    printf("\nMedia Library Data CIDs (0x20-0x29):\n");
    printf("  32. Library Data Type (0x20)\n");
    printf("  33. Library Data Name (0x21)\n");
    printf("  34. Artist Name (0x22)\n");
    printf("  35. Album Name (0x23)\n");
    printf("  36. Station Name (0x24)\n");

    printf("\nSystem and Zone CIDs (0x2A-0x2F):\n");
    printf("  42. Node Enable (0x2A)\n");
    printf("  43. Total Zones (0x2B)\n");
    printf("  44. Zone Name (0x2C)\n");

    printf("\nEqualization CIDs (0x30-0x3F):\n");
    printf("  48. Main/Sub Switching (0x30)\n");
    printf("  49. EQ Preset Name (0x31)\n");
    printf("  50. EQ Bass (0x32)\n");
    printf("  51. EQ Treble (0x33)\n");
    printf("  52. EQ Mid Range (0x34)\n");
    printf("  53. Balance (0x35)\n");
    printf("  54. Fade (0x36)\n");
    printf("  55. Sub Volume (0x37)\n");
    printf("  56. Subwoofer Switch (0x38)\n");
    printf("  57. Center Switch (0x39)\n");
    printf("  58. Tone Batch (0x3A)\n");
    printf("  59. Beep Volume (0x3B)\n");
    printf("  60. Speed Compensation (0x3C)\n");
    printf("  61. Overhead Switch (0x3D)\n");
    printf("  62. ANC Zone Enable (0x3E)\n");
    printf("  63. Beep (0x3F)\n");

    printf("\nVoice and Bluetooth related CIDs (0x40-0x56):\n");
    printf("  64. Voice Output (0x40)\n");
    printf("  70. BT Addresses Available (0x46)\n");
    printf("  71. BT Device Address (0x47)\n");
    printf("  72. BT Device Status (0x48)\n");
    printf("  73. BT Device Name (0x49)\n");
    printf("  80. BT Pairing Status (0x50)\n");
    printf("  81. BT Forget Device (0x51)\n");
    printf("  86. BT Discovering (0x56)\n");

    printf("\nAudio Configuration CIDs (0x60-0x6F):\n");
    printf("  96. Channel Slot Assignment (0x60)\n");
    printf("  97. PLL Lock Status (0x61)\n");

    printf("\nSensor CIDs (0x66-0x6F):\n");
    printf("  102. Input Voltage (0x66)\n");
    printf("  103. Input Current (0x67)\n");
    printf("  104. Temperature (0x68)\n");
    printf("  105. Sensor Generic (0x69)\n");

    printf("\nModule Configuration CIDs (0x70-0x7F):\n");
    printf("  112. Module Enable (0x70)\n");
    printf("  113. Power Supply Enable (0x71)\n");
    printf("  114. RCA Enable (0x72)\n");
    printf("  115. Soft Start (0x73)\n");
    printf("  116. Undervoltage Threshold (0x74)\n");
    printf("  117. Overvoltage Threshold (0x75)\n");
    printf("  118. BT WiFi Reserved (0x76)\n");

    printf("\nProtection and Diagnostics CIDs (0x80-0x8F):\n");
    printf("  128. Module Status (0x80)\n");
    printf("  129. Channel Clip Detection (0x81)\n");
    printf("  130. Channel Short Detection (0x82)\n");
    printf("  131. Channel Open Detection (0x83)\n");
    printf("  132. SHARC Status (0x84)\n");

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
    printf("  0x02: Request Info\n");
    printf("  0x03: Communication Error\n");
    printf("  0x04-0x0F: Reserved\n");

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

    printf("\nMedia Library Data CIDs (0x20-0x29):\n");
    printf("  0x20: Library Data Type\n");
    printf("  0x21: Library Data Name\n");
    printf("  0x22: Artist Name\n");
    printf("  0x23: Album Name\n");
    printf("  0x24: Station Name\n");
    printf("  0x25-0x29: Reserved\n");

    printf("\nSystem and Zone CIDs (0x2A-0x2F):\n");
    printf("  0x2A: Node Enable\n");
    printf("  0x2B: Total Zones Available\n");
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
    printf("  0x37: Sub Volume\n");
    printf("  0x38: Subwoofer Switch\n");
    printf("  0x39: Center Switch\n");
    printf("  0x3A: Tone Batch\n");
    printf("  0x3B: Beep Volume\n");
    printf("  0x3C: Speed Compensation\n");
    printf("  0x3D: Overhead Switch\n");
    printf("  0x3E: ANC Zone Enable\n");
    printf("  0x3F: Beep\n");

    printf("\nVoice and Bluetooth related CIDs (0x40-0x56):\n");
    printf("  0x40: Voice Output\n");
    printf("  0x41-0x45: Reserved\n");
    printf("  0x46: BT Addresses Available\n");
    printf("  0x47: BT Device Address\n");
    printf("  0x48: BT Device Status\n");
    printf("  0x49: BT Device Name\n");
    printf("  0x50: BT Pairing Status\n");
    printf("  0x51: BT Forget Device\n");
    printf("  0x52-0x55: Reserved\n");
    printf("  0x56: BT Discovering\n");
    printf("  0x57-0x5F: Reserved\n");

    printf("\nAudio Configuration CIDs (0x60-0x6F):\n");
    printf("  0x60: Channel Slot Assignment\n");
    printf("  0x61: PLL Lock Status\n");
    printf("  0x62-0x65: Reserved\n");

    printf("\nSensor CIDs (0x66-0x6F):\n");
    printf("  0x66: Input Voltage\n");
    printf("  0x67: Input Current\n");
    printf("  0x68: Temperature\n");
    printf("  0x69: Sensor Generic\n");
    printf("  0x6A-0x6F: Reserved\n");

    printf("\nModule Configuration CIDs (0x70-0x7F):\n");
    printf("  0x70: Module Enable\n");
    printf("  0x71: Power Supply Enable\n");
    printf("  0x72: RCA Enable\n");
    printf("  0x73: Soft Start\n");
    printf("  0x74: Undervoltage Threshold\n");
    printf("  0x75: Overvoltage Threshold\n");
    printf("  0x76: BT WiFi Reserved\n");
    printf("  0x77-0x7F: Reserved\n");

    printf("\nProtection and Diagnostics CIDs (0x80-0x8F):\n");
    printf("  0x80: Module Status\n");
    printf("  0x81: Channel Clip Detection\n");
    printf("  0x82: Channel Short Detection\n");
    printf("  0x83: Channel Open Detection\n");
    printf("  0x84: SHARC Status\n");
    printf("  0x85-0x8F: Reserved\n");

    printf("\nPress Enter to return to main menu...");
    getchar(); // Wait for Enter key (consume previous newline if any)
    getchar(); // Wait for actual Enter key
}

// Function to receive and process response from receiver
void receiveAndProcessResponse(int sock) { // Changed SOCKET to int
    uint8_t response[MAILBOX_SIZE];
    uint8_t responseData[256];  // Buffer for reconstructed response data
    uint8_t *responseDataPtr = responseData; // Pointer to the response data buffer
    uint8_t responseCid;
    uint8_t responseDataIndex = 0;
    uint8_t responseDataSize = 0;
    ssize_t bytesReceived; // Use ssize_t for recv return type

    bytesReceived = recv(sock, (char*)response, MAILBOX_SIZE, 0);

    if (bytesReceived == MAILBOX_SIZE) {
        printf("\n=== Received Message from Receiver ===\n");
        printf("Received Mailbox: ");
        for (int i = 0; i < MAILBOX_SIZE; i++) {
            printf("0x%02X ", response[i]);
        }
        printf("\n");

        // Parse the received frame
        bool isComplete = aabcop_parse_frame(response, &responseCid, &responseDataPtr, &responseDataIndex, &responseDataSize);

        // Print information about the received CID
        aabcop_print_cid_info(responseCid, responseData, responseDataIndex);

        // If message is not complete, try to receive more frames
        if (!isComplete) {
            printf("Message incomplete, waiting for more frames...\n");

            fd_set readSet;
            struct timeval timeout;

            FD_ZERO(&readSet);
            FD_SET(sock, &readSet);

            timeout.tv_sec = 1;  // 1 second timeout
            timeout.tv_usec = 0;

            while (!isComplete) {
                int ready = select(sock + 1, &readSet, NULL, NULL, &timeout); // Note: first arg of select is nfds

                if (ready > 0) {
                    if (FD_ISSET(sock, &readSet)) { // Check if our socket is ready
                        bytesReceived = recv(sock, (char*)response, MAILBOX_SIZE, 0);

                        if (bytesReceived == MAILBOX_SIZE) {
                            printf("Received continuation frame: ");
                            for (int i = 0; i < MAILBOX_SIZE; i++) {
                                printf("0x%02X ", response[i]);
                            }
                            printf("\n");

                            isComplete = aabcop_parse_frame(response, &responseCid, &responseDataPtr, &responseDataIndex, &responseDataSize);

                            if (isComplete) {
                                printf("Message complete after continuation frame.\n");
                                aabcop_print_cid_info(responseCid, responseData, responseDataIndex);
                            }
                        } else if (bytesReceived < 0) {
                            perror("Recv continuation error");
                            break;
                        } else { // bytesReceived == 0
                             printf("Connection closed by peer during continuation.\n");
                            break;
                        }
                    }
                } else if (ready == 0) {
                    printf("Timeout waiting for continuation frames.\n");
                    break;
                } else { // ready < 0
                    perror("Select error during continuation");
                    break;
                }
                // Re-arm select if needed for next iteration if not complete
                if (!isComplete) {
                    FD_ZERO(&readSet);
                    FD_SET(sock, &readSet);
                    timeout.tv_sec = 1; // Reset timeout
                    timeout.tv_usec = 0;
                }
            }
        }

        printf("===================================\n\n");
    } else if (bytesReceived > 0) {
        printf("Received partial response (%zd bytes)\n", bytesReceived);
    } else if (bytesReceived == 0) {
        printf("Connection closed by the receiver\n");
    } else { // bytesReceived < 0
        perror("Error receiving response");
    }
}

int main() {
    int s; // Changed SOCKET to int
    struct sockaddr_in server;
    uint8_t data[MAX_DATA_SIZE];
    uint8_t UID = UID_DANG8100;  // Default UID for DANG8100
    char comment[MAX_COMMENT_SIZE] = {0};

    // No WSAStartup needed for Linux
    printf("Initializing socket...\n");

    // Create socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // Changed INVALID_SOCKET to -1
        perror("Could not create socket");
        return 1;
    }
    printf("Socket created.\n");

    // Set up server address
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(12345);

    // Connect to server
    printf("Attempting to connect to the receiver...\n");
    if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Connection failed. Is the receiver running?");
        close(s); // Changed closesocket
        return 1;
    }

    printf("Connected to receiver.\n");

    int choice;
    uint8_t cid, dataLength;
    char dataLine[512];

    do {
        displayMenu();
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            // Clear input buffer
            while (getchar() != '\n' && getchar() != EOF);
            continue;
        }
        // Consume trailing newline from scanf
        getchar();


        if (choice == 0) break;

        if (choice == 90) {
            // getchar(); // Consume newline already handled
            showAllCIDs();
            continue;
        }

        // Map menu choice to CID
        if (choice == 91) {
            printf("Enter custom CID (hex, e.g. A1): ");
            if (scanf("%hhx", &cid) != 1) {
                printf("Invalid hex input for CID.\n");
                 while (getchar() != '\n' && getchar() != EOF);
                continue;
            }
            getchar(); // Consume newline
        } else {
            // Convert menu number to actual CID
            switch (choice) {
                // Network generic CIDs
                case 1: cid = CID_NETWORK_STARTUP; break;
                case 2: cid = CID_RESPONSE; break;
                case 3: cid = CID_REQUEST_INFO; break;
                case 4: cid = CID_COMMUNICATION_ERROR; break;

                // Audio control CIDs
                case 16: cid = CID_AUDIO_VIDEO_SOURCE_NAME; break;
                case 17: cid = CID_AUDIO_SOURCE_TYPE; break;
                case 18: cid = CID_PLAY_STATUS; break;
                case 19: cid = CID_ZONE_VOLUME; break;
                case 20: cid = CID_ZONE_VOLUME_STEP; break;
                case 21: cid = CID_MUTE_ZONE; break;
                case 22: cid = CID_MUTE_CHANNELS; break;
                case 23: cid = CID_ELAPSED_TRACK_TIME; break;
                case 24: cid = CID_TRACK_TIME; break;
                case 25: cid = CID_REPEAT_SUPPORT; break;
                case 26: cid = CID_SHUFFLE_SUPPORT; break;

                // Media Library Data CIDs
                case 32: cid = CID_LIBRARY_DATA_TYPE; break;
                case 33: cid = CID_LIBRARY_DATA_NAME; break;
                case 34: cid = CID_ARTIST_NAME; break;
                case 35: cid = CID_ALBUM_NAME; break;
                case 36: cid = CID_STATION_NAME; break;

                // System and Zone CIDs
                case 42: cid = CID_NODE_ENABLE; break;
                case 43: cid = CID_TOTAL_ZONES; break;
                case 44: cid = CID_ZONE_NAME; break;

                // Equalization CIDs
                case 48: cid = CID_MAIN_SUB_SWITCHING; break;
                case 49: cid = CID_EQ_PRESET_NAME; break;
                case 50: cid = CID_EQ_BASS; break;
                case 51: cid = CID_EQ_TREBLE; break;
                case 52: cid = CID_EQ_MID_RANGE; break;
                case 53: cid = CID_BALANCE; break;
                case 54: cid = CID_FADE; break;
                case 55: cid = CID_SUB_VOLUME; break;
                case 56: cid = CID_SUBWOOFER_SWITCH; break;
                case 57: cid = CID_CENTER_SWITCH; break;

                // Sensor CIDs (as per original mixed order)
                case 102: cid = CID_INPUT_VOLTAGE; break;
                case 103: cid = CID_INPUT_CURRENT; break;
                case 104: cid = CID_TEMPERATURE; break;

                // Equalization CIDs (additional)
                case 58: cid = CID_TONE_BATCH; break;
                case 59: cid = CID_BEEP_VOLUME; break;
                case 60: cid = CID_SPEED_COMP; break;
                case 61: cid = CID_OVERHEAD_SWITCH; break;
                case 62: cid = CID_ANC_ZONE_ENABLE; break;
                case 63: cid = CID_BEEP; break;

                // Voice and Bluetooth related CIDs
                case 64: cid = CID_VOICE_OUTPUT; break;
                case 70: cid = CID_BT_ADDR_AVAILABLE; break;
                case 71: cid = CID_BT_DEVICE_ADDR; break;
                case 72: cid = CID_BT_DEVICE_STATUS; break;
                case 73: cid = CID_BT_DEVICE_NAME; break;
                case 80: cid = CID_BT_PAIRING_STATUS; break;
                case 81: cid = CID_BT_FORGET_DEVICE; break;
                case 86: cid = CID_BT_DISCOVERING; break;

                // Audio Configuration CIDs
                case 96: cid = CID_CHANNEL_SLOT; break;
                case 97: cid = CID_PLL_LOCK_STATUS; break;

                // Sensor CIDs (additional)
                case 105: cid = CID_SENSOR_GENERIC; break;

                // Module Configuration CIDs
                case 112: cid = CID_MODULE_ENABLE; break;
                case 113: cid = CID_POWER_SUPPLY_ENABLE; break;
                case 114: cid = CID_RCA_ENABLE; break;
                case 115: cid = CID_SOFT_START; break;
                case 116: cid = CID_UNDERVOLTAGE_THRESHOLD; break;
                case 117: cid = CID_OVERVOLTAGE_THRESHOLD; break;
                case 118: cid = CID_BT_WIFI_RESERVED; break;

                // Protection and Diagnostics CIDs
                case 128: cid = CID_MODULE_STATUS; break;
                case 129: cid = CID_CHANNEL_CLIP; break;
                case 130: cid = CID_CHANNEL_SHORT; break;
                case 131: cid = CID_CHANNEL_OPEN; break;
                case 132: cid = CID_SHARC_STATUS; break;

                // Original menu numbers for backwards compatibility (some might be redundant now)
                // case 10: cid = CID_AUDIO_VIDEO_SOURCE_NAME; break; // Redundant with 16
                // case 11: cid = CID_AUDIO_SOURCE_TYPE; break; // Redundant with 17
                // ... and so on for other redundant cases. For brevity, I'll omit them.
                // Ensure your CID constants are correctly defined in aabcop_new.h

                default:
                    printf("Invalid choice. Please try again.\n");
                    continue;
            }
        }

        aabcop_print_cid_description(cid); // Assuming this function exists and is portable

        // Check CID type
        uint8_t cidType = aabcop_get_cid_message_type(cid); // Assuming this function exists
        if (cidType == 0) { // Assuming 0 means unknown
            printf("Warning: Unknown CID type.\n");
            printf("Do you still want to send it? (y/n): ");
            char confirm;
            scanf(" %c", &confirm);
            getchar(); // Consume newline
            if (confirm != 'y' && confirm != 'Y') {
                continue;
            }
        }
        // Assuming CID_TYPE_R is defined in aabcop_new.h
        else if (cidType == CID_TYPE_R) {
            printf("Warning: This CID is a receiver type (R). It's typically sent from receiver to sender.\n");
            printf("Do you still want to send it? (y/n): ");
            char confirm;
            scanf(" %c", &confirm);
            getchar(); // Consume newline
            if (confirm != 'y' && confirm != 'Y') {
                continue;
            }
        }

        // Generate default data based on CID or get custom data
        printf("Use default data for this CID? (y/n): ");
        char useDefault;
        scanf(" %c", &useDefault);
        getchar(); // Consume newline

        if (useDefault == 'y' || useDefault == 'Y') {
            generateDefaultData(cid, data, &dataLength, comment); // comment is populated here

            printf("Using default data: ");
            for (int i = 0; i < dataLength; i++) {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
            printf("Default comment: %s\n", comment);
        } else {
            printf("Enter data (hex, space-separated, e.g., 01 A2 FF):\n");
            // getchar(); // consume newline already handled
            if (fgets(dataLine, sizeof(dataLine), stdin) == NULL) {
                printf("Error reading data line.\n");
                continue;
            }

            char* token = strtok(dataLine, " \t\n");
            dataLength = 0;

            while (token && dataLength < MAX_DATA_SIZE) {
                if (sscanf(token, "%hhx", &data[dataLength]) == 1) {
                    dataLength++;
                } else {
                    printf("Warning: Could not parse '%s' as hex. Skipping.\n", token);
                }
                token = strtok(NULL, " \t\n");
            }

            if (dataLength == 0 && strlen(dataLine) > 1) { // Check if anything was typed but not parsed
                 printf("No valid hex data entered. Using empty data if you proceed.\n");
            }


            printf("Enter comment for this message:\n");
            if (fgets(comment, sizeof(comment), stdin) == NULL) {
                 printf("Error reading comment.\n");
                 comment[0] = '\0'; // Ensure comment is empty
            }
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
        packet.Comment[MAX_COMMENT_SIZE - 1] = '\0'; // Ensure null-termination

        printf("Sending packet with CID: 0x%02X, Data length: %d\n", cid, dataLength);
        CreateA2BFrames(&packet, s);
        printf("Packet sent successfully!\n");

        // Check if we should expect a response (for R/S type CIDs)
        // Assuming CID_TYPE_R also implies we might get a response in this context
        // or there's another type like CID_TYPE_SR for this.
        if (cidType & CID_TYPE_R) { // Using SR instead of RS as per original comment - adjust if needed
            printf("Waiting for response from receiver (CID type implies response)...\n");

            // Add a small delay to allow the receiver to process and respond
            usleep(500 * 1000); // 500 milliseconds

            // Non-blocking check for response
            fd_set readSet;
            struct timeval timeout;

            FD_ZERO(&readSet);
            FD_SET(s, &readSet);

            timeout.tv_sec = 2;  // 2 seconds timeout
            timeout.tv_usec = 0;

            // The first argument to select() should be the highest-numbered file descriptor plus 1.
            int ready = select(s + 1, &readSet, NULL, NULL, &timeout);

            if (ready > 0) {
                if (FD_ISSET(s, &readSet)) { // Check if our socket is the one that's ready
                    receiveAndProcessResponse(s);
                }
            } else if (ready == 0) {
                printf("No response received within timeout period\n");
            } else { // ready < 0
                perror("Error in select()");
            }
        }

    } while (choice != 0);

    // Clean up
    close(s); // Changed closesocket
    // No WSACleanup() needed for Linux

    printf("Program terminated.\n");
    return 0;
}
