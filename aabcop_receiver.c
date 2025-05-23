#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#include "aabcop.h"

// Buffer for reassembling multi-frame messages
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

// Function to print message details based on CID
void printCIDInfo(uint8_t cid, uint8_t* data, uint8_t dataLen) {
    printf("Command ID (CID): 0x%02X - ", cid);
    
    switch (cid) {
        // Network generic CIDs
        case CID_NETWORK_STARTUP:
            printf("Network Startup/Validation\n");
            printf("Action: Primary node validates the A2B network configuration\n");
            break;
            
        case CID_RESPONSE:
            printf("Response Command\n");
            if (dataLen >= 2) {
                printf("Echo CID: 0x%02X, Status: %d", data[0], data[1]);
                if (data[1] == RSP_STATUS_COMPLETION) printf(" (Completion)");
                else if (data[1] == RSP_STATUS_NOT_SUPPORTED) printf(" (CID Not Supported)");
                else if (data[1] == RSP_STATUS_PARAM_ERROR) printf(" (Parameter Error)");
                else if (data[1] == RSP_STATUS_BUSY) printf(" (Busy)");
                else if (data[1] == RSP_STATUS_EXEC_FAILURE) printf(" (Execution Failure)");
                printf("\n");
                
                if (dataLen > 2) {
                    printf("Additional data: ");
                    for (int i = 2; i < dataLen; i++) {
                        printf("0x%02X ", data[i]);
                    }
                    printf("\n");
                }
            }
            break;
            
        case CID_COMMUNICATION_ERROR:
            printf("Communication Error\n");
            if (dataLen >= 1) {
                printf("Error type: %d", data[0]);
                if (data[0] == 0) printf(" (None)");
                else if (data[0] == 1) printf(" (Error at Sender)");
                else if (data[0] == 2) printf(" (Error at Receiver)");
                printf("\n");
            }
            break;
            
        // Audio control CIDs
        case CID_AUDIO_VIDEO_SOURCE_NAME:
            printf("Audio/Video Source Name\n");
            if (dataLen > 0) {
                printf("Source name: ");
                for (int i = 0; i < dataLen; i++) {
                    printf("%c", data[i]);
                }
                printf("\n");
            }
            break;
            
        case CID_AUDIO_SOURCE_TYPE:
            printf("Audio Source Type and Capabilities\n");
            if (dataLen >= 1) {
                printf("Type: %d", data[0]);
                switch(data[0]) {
                    case 1: printf(" (AM)"); break;
                    case 2: printf(" (FM)"); break;
                    case 6: printf(" (USB)"); break;
                    case 11: printf(" (Bluetooth)"); break;
                    case 23: printf(" (HDMI)"); break;
                    case 28: printf(" (Microphone A2B)"); break;
                    default: break;
                }
                printf("\n");
                
                if (dataLen >= 5) {
                    uint32_t capabilities = 
                        ((uint32_t)data[1] << 24) | 
                        ((uint32_t)data[2] << 16) | 
                        ((uint32_t)data[3] << 8) | 
                        data[4];
                    printf("Capabilities: 0x%08X\n", capabilities);
                    
                    // Print individual capabilities
                    if (capabilities & 0x00000001) printf("  - Play\n");
                    if (capabilities & 0x00000002) printf("  - Pause\n");
                    if (capabilities & 0x00000004) printf("  - Stop\n");
                    // Add more as needed
                }
            }
            break;
            
        case CID_PLAY_STATUS:
            printf("Play Status\n");
            if (dataLen >= 1) {
                printf("Status: %d", data[0]);
                switch(data[0]) {
                    case 0: printf(" (Play)"); break;
                    case 1: printf(" (Pause)"); break;
                    case 2: printf(" (Stop)"); break;
                    case 3: printf(" (FF 1x)"); break;
                    case 4: printf(" (FF 2x)"); break;
                    case 5: printf(" (FF 3x)"); break;
                    case 6: printf(" (FF 4x)"); break;
                    case 7: printf(" (RW 1x)"); break;
                    case 8: printf(" (RW 2x)"); break;
                    case 9: printf(" (RW 3x)"); break;
                    case 10: printf(" (RW 4x)"); break;
                    default: break;
                }
                printf("\n");
            }
            break;
            
        case CID_ZONE_VOLUME:
            printf("Zone Volume Absolute\n");
            if (dataLen >= 2) {
                printf("Zone: %d", data[0]);
                if (data[0] == 0) printf(" (All Zones)");
                else printf(" (Zone %d)", data[0]);
                
                printf(", Volume: %d%%\n", data[1]);
            }
            break;
            
        case CID_ZONE_VOLUME_STEP:
            printf("Zone Volume Step\n");
            if (dataLen >= 2) {
                printf("Zone: %d", data[0]);
                if (data[0] == 0) printf(" (All Zones)");
                else printf(" (Zone %d)", data[0]);
                
                uint8_t stepDir = (data[1] >> 7) & 0x01;
                uint8_t stepSize = (data[1] >> 3) & 0x0F;
                
                printf(", Direction: %s, Step Size: %d\n", 
                       stepDir ? "Down" : "Up", stepSize);
            }
            break;
            
        case CID_MUTE_ZONE:
            printf("Mute Zone\n");
            if (dataLen >= 2) {
                printf("Zone: %d", data[0]);
                if (data[0] == 0) printf(" (All Zones)");
                else printf(" (Zone %d)", data[0]);
                
                printf(", Command: %d", data[1]);
                if (data[1] == 0) printf(" (Unmute)");
                else if (data[1] == 1) printf(" (Mute)");
                printf("\n");
            }
            break;
            
        case CID_MUTE_CHANNELS:
            printf("Mute Channels\n");
            if (dataLen >= 2) {
                uint16_t bitMap = (data[0] << 8) | data[1];
                printf("Mute Bitmap: 0x%04X (", bitMap);
                for (int i = 0; i < 16; i++) {
                    if (bitMap & (1 << (15 - i)))
                        printf("1");
                    else
                        printf("0");
                    
                    if (i % 4 == 3 && i < 15)
                        printf(" ");
                }
                printf(")\n");
                
                // Print which channels are muted
                printf("Muted channels: ");
                int hasMutedChannels = 0;
                for (int i = 0; i < 16; i++) {
                    if (bitMap & (1 << i)) {
                        printf("%d ", i + 1);
                        hasMutedChannels = 1;
                    }
                }
                if (!hasMutedChannels) {
                    printf("None");
                }
                printf("\n");
            }
            break;
            
        case CID_ELAPSED_TRACK_TIME:
            printf("Elapsed Track/Chapter Time\n");
            if (dataLen >= 4) {
                uint32_t seconds = 
                    ((uint32_t)data[0] << 24) | 
                    ((uint32_t)data[1] << 16) | 
                    ((uint32_t)data[2] << 8) | 
                    data[3];
                
                int hours = seconds / 3600;
                int minutes = (seconds % 3600) / 60;
                int secs = seconds % 60;
                
                printf("Time: %02d:%02d:%02d (%u seconds)\n", 
                       hours, minutes, secs, seconds);
            }
            break;
            
        case CID_TRACK_TIME:
            printf("Track/Chapter Time\n");
            if (dataLen >= 4) {
                uint32_t seconds = 
                    ((uint32_t)data[0] << 24) | 
                    ((uint32_t)data[1] << 16) | 
                    ((uint32_t)data[2] << 8) | 
                    data[3];
                
                int hours = seconds / 3600;
                int minutes = (seconds % 3600) / 60;
                int secs = seconds % 60;
                
                printf("Total time: %02d:%02d:%02d (%u seconds)\n", 
                       hours, minutes, secs, seconds);
            }
            break;
            
        case CID_REPEAT_SUPPORT:
            printf("Repeat Support\n");
            if (dataLen >= 1) {
                uint8_t supported = (data[0] >> 4) & 0x0F;
                uint8_t status = data[0] & 0x0F;
                
                printf("Supported modes: ");
                if (supported & 0x1) printf("Song ");
                if (supported & 0x2) printf("Play Queue ");
                printf("\n");
                
                printf("Current status: ");
                switch(status) {
                    case 0: printf("Off"); break;
                    case 1: printf("One (Current File)"); break;
                    case 2: printf("All (Play Queue)"); break;
                    case 15: printf("Data Not Available / Do Not Change"); break;
                    default: printf("Reserved (%d)", status); break;
                }
                printf("\n");
            }
            break;
            
        case CID_SHUFFLE_SUPPORT:
            printf("Shuffle Support\n");
            if (dataLen >= 1) {
                uint8_t supported = (data[0] >> 4) & 0x0F;
                uint8_t status = data[0] & 0x0F;
                
                printf("Supported modes: ");
                if (supported & 0x1) printf("Play Queue ");
                if (supported & 0x2) printf("All ");
                printf("\n");
                
                printf("Current status: ");
                switch(status) {
                    case 0: printf("Off"); break;
                    case 1: printf("Play Queue"); break;
                    case 2: printf("All"); break;
                    case 15: printf("Data Not Available / Do Not Change"); break;
                    default: printf("Reserved (%d)", status); break;
                }
                printf("\n");
            }
            break;
            
        // Audio inputs CIDs
        case CID_LIBRARY_DATA_TYPE:
            printf("Library Data Type\n");
            if (dataLen >= 1) {
                printf("Type: %d", data[0]);
                switch(data[0]) {
                    case 0: printf(" (File)"); break;
                    case 1: printf(" (Playlist Name)"); break;
                    case 2: printf(" (Genre Name / Category Name)"); break;
                    case 3: printf(" (Album Name)"); break;
                    case 4: printf(" (Artist Name)"); break;
                    case 5: printf(" (Track Name / Song Name)"); break;
                    case 6: printf(" (Station Name / Channel name)"); break;
                    case 7: printf(" (Station Number / Channel Number)"); break;
                    case 8: printf(" (Favorite Number)"); break;
                    case 9: printf(" (Play Queue)"); break;
                    case 10: printf(" (Content Info)"); break;
                    default: break;
                }
                printf("\n");
            }
            break;
            
        case CID_LIBRARY_DATA_NAME:
        case CID_ARTIST_NAME:
        case CID_ALBUM_NAME:
        case CID_STATION_NAME:
        case CID_ZONE_NAME:
        case CID_EQ_PRESET_NAME:
            // All string data types are handled the same way
            {
                const char* dataType = "";
                switch (cid) {
                    case CID_LIBRARY_DATA_NAME: dataType = "Library Data"; break;
                    case CID_ARTIST_NAME: dataType = "Artist"; break;
                    case CID_ALBUM_NAME: dataType = "Album"; break;
                    case CID_STATION_NAME: dataType = "Station"; break;
                    case CID_ZONE_NAME: dataType = "Zone"; break;
                    case CID_EQ_PRESET_NAME: dataType = "EQ Preset"; break;
                    default: dataType = "Unknown"; break;
                }
                
                printf("%s Name\n", dataType);
                if (dataLen > 0) {
                    printf("Name: \"");
                    for (int i = 0; i < dataLen; i++) {
                        printf("%c", data[i]);
                    }
                    printf("\"\n");
                }
            }
            break;
            
        case CID_POWER:
            printf("Power\n");
            if (dataLen >= 1) {
                printf("Status: %d", data[0]);
                switch(data[0]) {
                    case 0: printf(" (Off)"); break;
                    case 1: printf(" (On)"); break;
                    case 2: printf(" (Error)"); break;
                    case 3: printf(" (Unavailable/Unknown)"); break;
                    default: break;
                }
                printf("\n");
            }
            break;
            
        case CID_TOTAL_ZONES:
            printf("Total Number of Zones Available\n");
            if (dataLen >= 1) {
                printf("Zones: %d\n", data[0]);
            }
            break;
            
        // Equalization CIDs
        case CID_MAIN_SUB_SWITCHING:
            printf("Main/Sub Switching\n");
            if (dataLen >= 2) {
                printf("Zone: %d", data[0]);
                if (data[0] == 0) printf(" (All Zones)");
                else printf(" (Zone %d)", data[0]);
                
                printf(", Value: %d", data[1]);
                if (data[1] == 0) printf(" (Main)");
                else if (data[1] == 1) printf(" (Sub)");
                printf("\n");
            }
            break;
            
        case CID_EQ_BASS:
        case CID_EQ_TREBLE:
        case CID_EQ_MID_RANGE:
        case CID_BALANCE:
        case CID_FADE:
        case CID_SUB_VOLUME:
            // Handle all percentage-based settings similarly
            {
                const char* settingType = "";
                switch (cid) {
                    case CID_EQ_BASS: settingType = "Bass"; break;
                    case CID_EQ_TREBLE: settingType = "Treble"; break;
                    case CID_EQ_MID_RANGE: settingType = "Mid Range"; break;
                    case CID_BALANCE: settingType = "Balance"; break;
                    case CID_FADE: settingType = "Fade"; break;
                    case CID_SUB_VOLUME: settingType = "Sub Volume"; break;
                    default: settingType = "Unknown"; break;
                }
                
                printf("Equalizer %s\n", settingType);
                if (dataLen >= 2) {
                    printf("Zone: %d", data[0]);
                    if (data[0] == 0) printf(" (All Zones)");
                    else printf(" (Zone %d)", data[0]);
                    
                    // Handle signed percentage values
                    int8_t value = (int8_t)data[1];
                    printf(", Level: %d%%", value);
                    
                    // Add directional info based on setting type
                    if (cid == CID_BALANCE && value != 0) {
                        printf(" (%s)", value > 0 ? "Right" : "Left");
                    } else if (cid == CID_FADE && value != 0) {
                        printf(" (%s)", value > 0 ? "Front" : "Rear");
                    }
                    
                    printf("\n");
                }
            }
            break;
            
        case CID_SUBWOOFER_SWITCH:
        case CID_CENTER_SWITCH:
        case CID_TONE_BATCH:
        case CID_OVERHEAD_SWITCH:
        case CID_ANC_ZONE_ENABLE:
        case CID_BEEP:
            // Handle all on/off zone-based settings similarly
            {
                const char* settingType = "";
                switch (cid) {
                    case CID_SUBWOOFER_SWITCH: settingType = "Subwoofer Direct"; break;
                    case CID_CENTER_SWITCH: settingType = "Center Direct"; break;
                    case CID_TONE_BATCH: settingType = "Tone Batch Direct"; break;
                    case CID_OVERHEAD_SWITCH: settingType = "Overhead Direct"; break;
                    case CID_ANC_ZONE_ENABLE: settingType = "ANC Zone"; break;
                    case CID_BEEP: settingType = "Beep"; break;
                    default: settingType = "Unknown"; break;
                }
                
                printf("%s Switching\n", settingType);
                if (dataLen >= 2) {
                    printf("Zone: %d", data[0]);
                    if (data[0] == 0) printf(" (All Zones)");
                    else printf(" (Zone %d)", data[0]);
                    
                    printf(", State: %d", data[1]);
                    if (data[1] == 0) printf(" (Disabled)");
                    else if (data[1] == 1) printf(" (Enabled)");
                    printf("\n");
                }
            }
            break;
            
        case CID_BEEP_VOLUME:
            printf("Beep Volume Direct Switching\n");
            if (dataLen >= 2) {
                printf("Zone: %d", data[0]);
                if (data[0] == 0) printf(" (All Zones)");
                else printf(" (Zone %d)", data[0]);
                
                printf(", Volume: %d%%\n", data[1]);
            }
            break;
            
        case CID_SPEED_COMP:
            printf("Speed Compensation\n");
            if (dataLen >= 2) {
                printf("Zone: %d", data[0]);
                if (data[0] == 0) printf(" (All Zones)");
                else printf(" (Zone %d)", data[0]);
                
                uint8_t enable = (data[1] >> 7) & 0x01;
                uint8_t speed = data[1] & 0x7F;
                
                printf(", Enabled: %s, Speed: %d\n", 
                       enable ? "Yes" : "No", speed);
            }
            break;
            
        case CID_VOICE_OUTPUT:
            printf("Voice Output\n");
            if (dataLen >= 1) {
                printf("State: %d", data[0]);
                if (data[0] == 0) printf(" (Disabled)");
                else if (data[0] == 1) printf(" (Enabled)");
                printf("\n");
            }
            break;
            
        // Bluetooth CIDs
        case CID_BT_ADDR_AVAILABLE:
            printf("Number of Bluetooth Addresses Available\n");
            if (dataLen >= 1) {
                printf("Count: %d\n", data[0]);
            }
            break;
            
        case CID_BT_DEVICE_ADDR:
            printf("Bluetooth Device Address\n");
            if (dataLen >= 7) {
                printf("Index: %d\n", data[0]);
                printf("Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                       data[1], data[2], data[3], data[4], data[5], data[6]);
            }
            break;
            
        case CID_BT_DEVICE_STATUS:
            printf("Bluetooth Device Status\n");
            if (dataLen >= 2) {
                printf("Index: %d\n", data[0]);
                printf("Status: %d", data[1]);
                switch(data[1]) {
                    case 0: printf(" (Connected)"); break;
                    case 1: printf(" (Not Connected, In Memory)"); break;
                    case 2: printf(" (Not Paired, Available)"); break;
                    case 254: printf(" (Error)"); break;
                    case 255: printf(" (Data Not Available)"); break;
                    default: break;
                }
                printf("\n");
            }
            break;
            
        case CID_BT_DEVICE_NAME:
            printf("Bluetooth Device Name\n");
            if (dataLen >= 1) {
                printf("Index: %d\n", data[0]);
                if (dataLen > 1) {
                    printf("Name: \"");
                    for (int i = 1; i < dataLen; i++) {
                        printf("%c", data[i]);
                    }
                    printf("\"\n");
                }
            }
            break;
            
        case CID_BT_PAIRING_STATUS:
            printf("Bluetooth Pairing Status\n");
            if (dataLen >= 2) {
                printf("Index: %d\n", data[0]);
                printf("Status: %d", data[1]);
                switch(data[1]) {
                    case 0: printf(" (Reserved)"); break;
                    case 1: printf(" (Connect)"); break;
                    case 2: printf(" (Connecting)"); break;
                    case 3: printf(" (Not Connected / Disconnected)"); break;
                    case 13: printf(" (Unknown)"); break;
                    case 14: printf(" (Error)"); break;
                    case 15: printf(" (Data Not Available)"); break;
                    default: break;
                }
                printf("\n");
            }
            break;
            
        case CID_BT_FORGET_DEVICE:
            printf("Forget Bluetooth Device\n");
            if (dataLen >= 2) {
                printf("Index: %d\n", data[0]);
                printf("Command: %d", data[1]);
                switch(data[1]) {
                    case 0: printf(" (No)"); break;
                    case 1: printf(" (Yes)"); break;
                    case 2: printf(" (Error)"); break;
                    case 3: printf(" (Unavailable/Unknown)"); break;
                    default: break;
                }
                printf("\n");
            }
            break;
            
        case CID_BT_DISCOVERING:
            printf("Bluetooth Discovering\n");
            if (dataLen >= 1) {
                printf("State: %d", data[0]);
                switch(data[0]) {
                    case 0: printf(" (No/Off)"); break;
                    case 1: printf(" (Yes/On)"); break;
                    case 2: printf(" (Error)"); break;
                    case 3: printf(" (Unavailable/Unknown)"); break;
                    default: break;
                }
                printf("\n");
            }
            break;
            
        // Audio configuration and PLL Lock CIDs
        case CID_CHANNEL_SLOT:
            printf("Channel Slot Assignment\n");
            if (dataLen >= 1) {
                uint8_t channel = (data[0] >> 4) & 0x0F;
                uint8_t slot = data[0] & 0x0F;
                printf("Channel: %d, Slot: %d\n", channel, slot);
            }
            break;
            
        case CID_PLL_LOCK_STATUS:
            printf("PLL Lock Status\n");
            if (dataLen >= 1) {
                printf("Status: 0x%02X", data[0]);
                if (data[0] == 0xFF) printf(" (All PLLs Locked)");
                else printf(" (Some PLLs Unlocked)");
                printf("\n");
                
                // Detailed bit by bit analysis
                if (data[0] != 0xFF) {
                    printf("Unlocked PLLs: ");
                    for (int i = 0; i < 8; i++) {
                        if (!(data[0] & (1 << i))) {
                            printf("PLL%d ", i);
                        }
                    }
                    printf("\n");
                }
            }
            break;
            
        case CID_INPUT_VOLTAGE:
            printf("Input Voltage\n");
            if (dataLen >= 2) {
                uint16_t millivolts = (data[0] << 8) | data[1];
                printf("Voltage: %u.%03u V\n", millivolts / 1000, millivolts % 1000);
            }
            break;
            
        case CID_INPUT_CURRENT:
            printf("Input Current\n");
            if (dataLen >= 2) {
                uint16_t milliamps = (data[0] << 8) | data[1];
                printf("Current: %u.%03u A\n", milliamps / 1000, milliamps % 1000);
            }
            break;
            
        case CID_TEMPERATURE:
            printf("Temperature Input Filter\n");
            if (dataLen >= 1) {
                int8_t temp = (int8_t)data[0];
                printf("Temperature: %d°C\n", temp);
            }
            break;
            
        case CID_SENSOR_GENERIC:
            printf("Sensor Generic\n");
            if (dataLen >= 1) {
                uint8_t sensorId = (data[0] >> 4) & 0x0F;
                printf("Sensor ID: %d\n", sensorId);
                
                printf("Raw data: ");
                for (int i = 0; i < dataLen; i++) {
                    printf("0x%02X ", data[i]);
                }
                printf("\n");
            }
            break;
            
        // Module configuration CIDs
        case CID_MODULE_ENABLE:
        case CID_POWER_SUPPLY_ENABLE:
        case CID_RCA_ENABLE:
            // Handle all simple enable commands
            {
                const char* moduleType = "";
                switch (cid) {
                    case CID_MODULE_ENABLE: moduleType = "Module"; break;
                    case CID_POWER_SUPPLY_ENABLE: moduleType = "Power Supply"; break;
                    case CID_RCA_ENABLE: moduleType = "RCA"; break;
                    default: moduleType = "Unknown"; break;
                }
                
                printf("%s Enable\n", moduleType);
                if (dataLen >= 1) {
                    printf("State: %d", data[0]);
                    if (data[0] == 0) printf(" (Disabled)");
                    else if (data[0] == 1) printf(" (Enabled)");
                    printf("\n");
                }
            }
            break;
            
        case CID_SOFT_START:
            printf("Soft Start\n");
            if (dataLen >= 1) {
                printf("Slew Rate: %d\n", data[0]);
            }
            break;
            
        case CID_UNDERVOLTAGE_THRESHOLD:
            printf("Undervoltage Threshold\n");
            if (dataLen >= 1) {
                printf("Threshold: 0x%02X\n", data[0]);
            }
            break;
            
        case CID_OVERVOLTAGE_THRESHOLD:
            printf("Overvoltage Threshold\n");
            if (dataLen >= 1) {
                printf("Threshold: 0x%02X\n", data[0]);
            }
            break;
            
        case CID_BT_WIFI_RESERVED:
            printf("Reserved commands for Bluetooth and Wi-Fi\n");
            if (dataLen > 0) {
                printf("Data: ");
                for (int i = 0; i < dataLen; i++) {
                    printf("0x%02X ", data[i]);
                }
                printf("\n");
            }
            break;
            
        // Protection and diagnostics CIDs
        case CID_MODULE_STATUS:
            printf("Module Status\n");
            if (dataLen >= 2) {
                uint8_t enable = (data[0] >> 7) & 0x01;
                uint8_t voltageLevel = (data[0] >> 2) & 0x1F;
                uint8_t ovp = (data[0] >> 1) & 0x01;
                uint8_t uvp = data[0] & 0x01;
                
                uint8_t ocp = (data[1] >> 7) & 0x01;
                uint8_t temperature = (data[1] >> 2) & 0x1F;
                uint8_t thermalFb = (data[1] >> 1) & 0x01;
                uint8_t thermalSd = data[1] & 0x01;
                
                printf("Enable: %d (%s)\n", enable, enable ? "Enabled" : "Disabled");
                printf("Voltage Level: %d (%.1f V)\n", voltageLevel, 6.0 + (voltageLevel * 0.5));
                printf("OVP: %d (%s)\n", ovp, ovp ? "Active" : "Inactive");
                printf("UVP: %d (%s)\n", uvp, uvp ? "Active" : "Inactive");
                printf("OCP: %d (%s)\n", ocp, ocp ? "Active" : "Inactive");
                
                // Calculate actual temperature range from -40°C to 175°C
                int actualTemp = -40 + (temperature * 7);
                printf("Temperature: %d (~%d°C)\n", temperature, actualTemp);
                
                printf("Thermal Foldback: %d (%s)\n", thermalFb, thermalFb ? "Active" : "Inactive");
                printf("Thermal Shutdown: %d (%s)\n", thermalSd, thermalSd ? "Active" : "Inactive");
            }
            break;
            
        case CID_CHANNEL_STATUS:
            printf("Channel Status\n");
            if (dataLen >= 1) {
                uint8_t channel = (data[0] >> 4) & 0x0F;
                uint8_t clipDet = (data[0] >> 3) & 0x01;
                uint8_t shortCircuit = (data[0] >> 2) & 0x01;
                uint8_t openCircuit = (data[0] >> 1) & 0x01;
                uint8_t mute = data[0] & 0x01;
                
                printf("Channel: %d\n", channel);
                printf("Clip Detected: %d (%s)\n", clipDet, clipDet ? "Yes" : "No");
                printf("Short Circuit: %d (%s)\n", shortCircuit, shortCircuit ? "Yes" : "No");
                printf("Open Circuit: %d (%s)\n", openCircuit, openCircuit ? "Yes" : "No");
                printf("Mute: %d (%s)\n", mute, mute ? "Muted" : "Unmuted");
            }
            break;
            
        default:
            printf("Unknown Command (0x%02X)\n", cid);
            if (dataLen > 0) {
                printf("Data: ");
                for (int i = 0; i < dataLen; i++) {
                    printf("0x%02X ", data[i]);
                }
                printf("\n");
            }
            break;
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
            strcpy(comment, "Responding to network startup procedure");
            break;
        case CID_RESPONSE:
            data[0] = 0x32;  // Echo CID (e.g., EQ_BASS)
            data[1] = 0x00;  // Status: Success
            *length = 2;
            strcpy(comment, "Response to command");
            break;
        case CID_COMMUNICATION_ERROR:
            data[0] = 0x02;  // Error type: Error at Receiver
            *length = 1;
            strcpy(comment, "Reporting communication error at receiver");
            break;
            
        // Audio control CIDs
        case CID_AUDIO_VIDEO_SOURCE_NAME:
            // String data example (e.g., "USB")
            data[0] = 'U';
            data[1] = 'S';
            data[2] = 'B';
            *length = 3;
            strcpy(comment, "Current audio source name is 'USB'");
            break;
        case CID_AUDIO_SOURCE_TYPE:
            data[0] = 0x06;  // USB
            data[1] = 0x01;  // Capabilities: Play
            data[2] = 0x00;
            data[3] = 0x00;
            data[4] = 0x00;
            *length = 5;
            strcpy(comment, "Current audio source type is USB");
            break;
        case CID_PLAY_STATUS:
            data[0] = 0x00;  // Play
            *length = 1;
            strcpy(comment, "Current play status is PLAY");
            break;
        case CID_REPEAT_SUPPORT:
            data[0] = 0x31;  // Supported: Song+Queue (3), Status: One (1)
            *length = 1;
            strcpy(comment, "Current repeat mode: One track");
            break;
        case CID_SHUFFLE_SUPPORT:
            data[0] = 0x31;  // Supported: Queue+All (3), Status: Queue (1)
            *length = 1;
            strcpy(comment, "Current shuffle mode: Play Queue");
            break;
            
        // Audio inputs CIDs
        case CID_POWER:
            data[0] = 0x01;  // On
            *length = 1;
            strcpy(comment, "Current power state is ON");
            break;
        case CID_BT_PAIRING_STATUS:
            data[0] = 0x00;  // Index 0
            data[1] = 0x03;  // Status: Not Connected / Disconnected
            *length = 2;
            strcpy(comment, "Bluetooth device 0 is not connected");
            break;
            
        // Default response for any CID
        default:
            data[0] = 0x00;
            *length = 1;
            strcpy(comment, "Default response");
            break;
    }
}

// Function to create A2B frames for sending
void createAndSendA2BFrames(SOCKET clientSocket, uint8_t cid, uint8_t* data, uint8_t dataLength, const char* comment) {
    uint8_t mailbox[MAILBOX_SIZE] = {0};
    uint8_t mailboxIndex = 0;
    
    // Print message details
    printf("\n=== Sending Message to Sender ===\n");
    printf("CID: 0x%02X - ", cid);
    printCIDInfo(cid, data, dataLength);
    
    if (dataLength > 0) {
        printf("Data Values: ");
        for (int i = 0; i < dataLength; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    }
    
    if (comment && strlen(comment) > 0) {
        printf("Comment: %s\n", comment);
    }
    
    // Function to write a byte to the mailbox and send when full
    auto writeToMailbox = [&](uint8_t byte) {
        mailbox[mailboxIndex++] = byte;
        
        if (mailboxIndex == MAILBOX_SIZE) {
            send(clientSocket, (const char*)mailbox, MAILBOX_SIZE, 0);
            printf("Sent Mailbox: ");
            for (int i = 0; i < MAILBOX_SIZE; i++) {
                printf("0x%02X ", mailbox[i]);
            }
            printf("\n");
            mailboxIndex = 0;
        }
    };
    
    if (dataLength <= 2) {
        // Single frame
        uint8_t header = 0x7F & dataLength;  // FRAME_TYPE=0, DATA_SIZE=L
        writeToMailbox(header);
        writeToMailbox(cid);
        writeToMailbox(dataLength > 0 ? data[0] : 0x00);
        writeToMailbox(dataLength > 1 ? data[1] : 0x00);
    } else {
        // Start frame
        uint8_t header = 0x7F & dataLength;  // FRAME_TYPE=0, DATA_SIZE=L
        writeToMailbox(header);
        writeToMailbox(cid);
        writeToMailbox(data[0]);
        writeToMailbox(data[1]);
        
        // Calculate number of multi-frames needed
        uint8_t multiCount = (dataLength - 2 + 2) / 3;  // Ceiling division for (L-2)/3
        uint8_t dataIndex = 2;
        
        // Create multi-frames
        for (uint8_t counter = 1; counter <= multiCount; counter++) {
            uint8_t header = 0x80 | counter;  // FRAME_TYPE=1, COUNTER=counter
            writeToMailbox(header);
            
            // Add up to 3 data bytes per multi-frame
            for (int i = 0; i < 3; i++) {
                if (dataIndex < dataLength) {
                    writeToMailbox(data[dataIndex++]);
                } else {
                    writeToMailbox(0x00);  // Padding
                }
            }
        }
    }
    
    // Flush any remaining data
    if (mailboxIndex > 0) {
        // Pad remaining bytes with zeros
        while (mailboxIndex < MAILBOX_SIZE) {
            mailbox[mailboxIndex++] = 0x00;
        }
        
        send(clientSocket, (const char*)mailbox, MAILBOX_SIZE, 0);
        printf("Sent Mailbox (final): ");
        for (int i = 0; i < MAILBOX_SIZE; i++) {
            printf("0x%02X ", mailbox[i]);
        }
        printf("\n");
    }
    
    printf("===================================\n\n");
}

// Function to process a single mailbox (4 bytes)
void processMailbox(uint8_t* mailbox, MessageBuffer* msgBuffer) {
    // Print raw mailbox bytes
    printf("Received mailbox bytes: ");
    for (int i = 0; i < MAILBOX_SIZE; i++) {
        printf("0x%02X ", mailbox[i]);
    }
    printf("\n");
    
    // Check if this is a start frame or continuation frame
    uint8_t frameType = (mailbox[0] >> 7) & 0x01;
    
    if (frameType == 0) {  // Start frame
        msgBuffer->frameType = 0;
        msgBuffer->dataSize = mailbox[0] & 0x7F;
        msgBuffer->cid = mailbox[1];
        msgBuffer->dataIndex = 0;
        
        // Copy data from start frame
        if (msgBuffer->dataSize > 0 && msgBuffer->dataIndex < MAX_DATA_SIZE) {
            msgBuffer->data[msgBuffer->dataIndex++] = mailbox[2];
        }
        if (msgBuffer->dataSize > 1 && msgBuffer->dataIndex < MAX_DATA_SIZE) {
            msgBuffer->data[msgBuffer->dataIndex++] = mailbox[3];
        }
        
        // Calculate expected frames
        if (msgBuffer->dataSize <= 2) {
            msgBuffer->expectedFrames = 1;  // Only start frame
        } else {
            uint8_t remainingData = msgBuffer->dataSize - 2;  // Data after start frame
            msgBuffer->expectedFrames = 1 + (remainingData + 2) / 3;  // Start frame + multi frames
        }
        
        msgBuffer->receivedFrames = 1;
        
        printf("Start Frame: DataSize=%d, CID=0x%02X, ExpectedFrames=%d\n", 
               msgBuffer->dataSize, msgBuffer->cid, msgBuffer->expectedFrames);
    } else {  // Continuation frame
        uint8_t counter = mailbox[0] & 0x7F;
        printf("Continuation Frame: Counter=%d\n", counter);
        
        // Copy data from continuation frame
        for (int i = 1; i < MAILBOX_SIZE; i++) {
            if (msgBuffer->dataIndex < MAX_DATA_SIZE) {
                msgBuffer->data[msgBuffer->dataIndex++] = mailbox[i];
            }
        }
        
        msgBuffer->receivedFrames++;
    }
    
    // Check if message is complete
    if (msgBuffer->receivedFrames == msgBuffer->expectedFrames) {
        printf("\n*** Message Complete ***\n");
        printf("CID: 0x%02X, Data Length: %d\n", msgBuffer->cid, msgBuffer->dataIndex);
        
        // Print data bytes
        printf("Data: ");
        for (int i = 0; i < msgBuffer->dataIndex; i++) {
            printf("0x%02X ", msgBuffer->data[i]);
        }
        printf("\n");
        
        // Print CID-specific information
        printCIDInfo(msgBuffer->cid, msgBuffer->data, msgBuffer->dataIndex);
        printf("--------------------------------------------\n\n");
    }
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
        case CID_CHANNEL_STATUS: cidType = CID_TYPE_CHANNEL_STATUS; break;
        default: cidType = 0; break;
    }
    
    return cidType;
}

// Function to check if the CID can be sent by receiver (R or R/S)
int isReceiverCID(uint8_t cid) {
    uint8_t cidType = getCIDType(cid);
    
    // Check if CID is receiver type (R or R/S)
    return (cidType == CID_TYPE_R || cidType == CID_TYPE_RS);
}

// Function to send a response message (for interactive mode)
void sendResponseMessage(SOCKET clientSocket, uint8_t cid, uint8_t status) {
    uint8_t mailbox[MAILBOX_SIZE];
    
    // Create a single-frame response message
    mailbox[0] = 0x03;  // FRAME_TYPE=0, DATA_SIZE=3
    mailbox[1] = CID_RESPONSE;
    mailbox[2] = cid;   // Echo the original CID
    mailbox[3] = status;  // Status (0 = Success)
    
    // Send the response
    send(clientSocket, (const char*)mailbox, MAILBOX_SIZE, 0);
    
    printf("Sent response: CID=0x%02X, Status=%d\n", cid, status);
}

// Function to print CID descriptions
void printCIDDescription(uint8_t cid) {
    printf("Selected CID: 0x%02X - ", cid);
    
    // Get CID Type
    uint8_t cidType = getCIDType(cid);
    
    switch (cid) {
        // Network generic CIDs
        case CID_NETWORK_STARTUP:
            printf("Network Startup/Validation");
            break;
        case CID_RESPONSE:
            printf("Response Command");
            break;
        case CID_COMMUNICATION_ERROR:
            printf("Communication Error");
            break;

        // Audio control CIDs
        case CID_AUDIO_VIDEO_SOURCE_NAME:
            printf("Audio/Video Source Name");
            break;
        case CID_AUDIO_SOURCE_TYPE:
            printf("Audio Source Type and Capabilities");
            break;
        case CID_PLAY_STATUS:
            printf("Play Status");
            break;
        case CID_ZONE_VOLUME:
            printf("Zone Volume Absolute");
            break;
        case CID_ZONE_VOLUME_STEP:
            printf("Zone Volume Step");
            break;
        case CID_MUTE_ZONE:
            printf("Mute Zone");
            break;
        case CID_MUTE_CHANNELS:
            printf("Mute Channels");
            break;
        case CID_ELAPSED_TRACK_TIME:
            printf("Elapsed Track/Chapter Time");
            break;
        case CID_TRACK_TIME:
            printf("Track/Chapter Time");
            break;
        case CID_REPEAT_SUPPORT:
            printf("Repeat Support");
            break;
        case CID_SHUFFLE_SUPPORT:
            printf("Shuffle Support");
            break;

        // Audio inputs CIDs
        case CID_LIBRARY_DATA_TYPE:
            printf("Library Data Type");
            break;
        case CID_LIBRARY_DATA_NAME:
            printf("Library Data Name");
            break;
        case CID_ARTIST_NAME:
            printf("Artist Name");
            break;
        case CID_ALBUM_NAME:
            printf("Album Name");
            break;
        case CID_STATION_NAME:
            printf("Station Name");
            break;
        case CID_POWER:
            printf("Power");
            break;
        case CID_TOTAL_ZONES:
            printf("Total Number of Zones Available");
            break;
        case CID_ZONE_NAME:
            printf("Zone Name");
            break;

        // More cases can be added as needed

        default:
            printf("Unknown/Custom Command");
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

int main() {
    WSADATA wsa;
    SOCKET listenSocket, clientSocket;
    struct sockaddr_in server, client;
    int clientLen = sizeof(struct sockaddr_in);
    uint8_t mailbox[MAILBOX_SIZE];
    MessageBuffer msgBuffer = {0};
    BOOL interactiveMode = FALSE;
    BOOL enableRSResponses = FALSE;
    
    // Initialize Winsock
    printf("Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    
    // Create socket
    if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket. Error: %d\n", WSAGetLastError());
        return 1;
    }
    
    // Prepare sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(12345);
    
    // Bind socket
    if (bind(listenSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed. Error: %d\n", WSAGetLastError());
        return 1;
    }
    
    // Listen for incoming connections
    listen(listenSocket, 3);

    // Ask user for interactive mode
    char choice;
    printf("Enable interactive mode (respond to commands)? (y/n): ");
    scanf(" %c", &choice);
    interactiveMode = (choice == 'y' || choice == 'Y');
    getchar();  // Consume newline
    
    // Ask user for handling R/S CIDs
    printf("Enable automatic responses for R/S type CIDs? (y/n): ");
    scanf(" %c", &choice);
    enableRSResponses = (choice == 'y' || choice == 'Y');
    getchar();  // Consume newline
    
    printf("AABCOP Protocol Receiver %s, R/S Responses %s\n", 
           interactiveMode ? "(Interactive Mode)" : "(Monitor Mode)",
           enableRSResponses ? "Enabled" : "Disabled");
    printf("Waiting for connections on port 12345...\n");
    
    // Accept an incoming connection
    if ((clientSocket = accept(listenSocket, (struct sockaddr*)&client, &clientLen)) == INVALID_SOCKET) {
        printf("Accept failed. Error: %d\n", WSAGetLastError());
        return 1;
    }
    
    printf("Connection accepted from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    
    // Receive data
    int recvSize;
    memset(&msgBuffer, 0, sizeof(MessageBuffer));
    
    while ((recvSize = recv(clientSocket, (char*)mailbox, MAILBOX_SIZE, 0)) > 0) {
        if (recvSize == MAILBOX_SIZE) {
            processMailbox(mailbox, &msgBuffer);
            
            // Check if message is complete
            if (msgBuffer.receivedFrames == msgBuffer.expectedFrames) {
                uint8_t cid = msgBuffer.cid;
                uint8_t cidType = getCIDType(cid);
                
                // In interactive mode, send responses
                if (interactiveMode) {
                    // Send basic response for all commands
                    sendResponseMessage(clientSocket, cid, RSP_STATUS_COMPLETION);
                    
                    // For R/S type CIDs, we may also send a specific response back
                    if (enableRSResponses && (cidType == CID_TYPE_RS || cidType == CID_TYPE_R)) {
                        printf("\n*** Sending response for R/S type CID 0x%02X ***\n", cid);
                        
                        // Generate default response data
                        uint8_t responseData[MAX_DATA_SIZE];
                        uint8_t responseLength;
                        char responseComment[MAX_COMMENT_SIZE];
                        
                        generateDefaultData(cid, responseData, &responseLength, responseComment);
                        
                        // Determine if we want to use default data or create custom data
                        if (enableRSResponses) {
                            printf("Use default response data? (y/n): ");
                            char useDefault;
                            char dataLine[512];
                            
                            scanf(" %c", &useDefault);
                            getchar();  // Consume newline
                            
                            if (useDefault != 'y' && useDefault != 'Y') {
                                printf("Enter response data (hex, space-separated, e.g., 01 A2 FF):\n");
                                fgets(dataLine, sizeof(dataLine), stdin);
                                
                                char* token = strtok(dataLine, " \t\n");
                                responseLength = 0;
                                
                                while (token && responseLength < MAX_DATA_SIZE) {
                                    if (sscanf(token, "%hhx", &responseData[responseLength]) == 1) {
                                        responseLength++;
                                    }
                                    token = strtok(NULL, " \t\n");
                                }
                                
                                if (responseLength == 0) {
                                    printf("No valid data entered. Using empty data.\n");
                                }
                                
                                printf("Enter comment for this response:\n");
                                fgets(responseComment, sizeof(responseComment), stdin);
                                responseComment[strcspn(responseComment, "\n")] = 0;  // Remove trailing newline
                            } else {
                                printf("Using default response data:\n");
                                for (int i = 0; i < responseLength; i++) {
                                    printf("0x%02X ", responseData[i]);
                                }
                                printf("\n");
                                printf("Default comment: %s\n", responseComment);
                            }
                            
                            // Send the R/S specific response
                            createAndSendA2BFrames(clientSocket, cid, responseData, responseLength, responseComment);
                        }
                    }
                }
            }
        } else {
            printf("Received incomplete mailbox (%d bytes)\n", recvSize);
        }
    }
    
    if (recvSize == 0) {
        printf("Connection closed by client\n");
    } else {
        printf("recv failed. Error: %d\n", WSAGetLastError());
    }
    
    // Clean up
    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();
    
    return 0;
}