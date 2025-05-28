#ifndef AABCOP_H
#define AABCOP_H

#include <stdint.h>
#include <stdbool.h>

// Constants for AABCOP protocol
#define UID_DANG8100           0x60  // Unit Identifier for DANG8100
#define MAILBOX_SIZE           4     // A2B Mailbox size in bytes

// Frame type definitions
#define FRAME_TYPE_START       0     // Start frame
#define FRAME_TYPE_CONTINUATION 1    // Continuation frame

// Device Types
#define DEVICE_TYPE_ALL        0x01  // All device types
#define DEVICE_TYPE_DSP        0x02  // DSP device type (Primary node)
#define DEVICE_TYPE_HU         0x04  // Head Unit device type
#define DEVICE_TYPE_AMP        0x08  // Amplifier device type (Secondary node)

// CID Message Type
#define CID_TYPE_S             0x01  // Sender messages
#define CID_TYPE_R             0x02  // Receiver messages
#define CID_TYPE_SR            0x03  // Both Sender and Receiver messages
#define CID_TYPE_RS            0x03  // Alias for CID_TYPE_SR (used in aabcop_sender.c)

// Protocol states
typedef enum {
    STATE_INIT,        // Initial state
    STATE_DISCOVERY,   // Discovering nodes on A2B network
    STATE_RUNNING,     // Normal running state
    STATE_ERROR        // Error state
} protocol_state_t;

// Network generic CIDs (0x00-0x0F)
#define CID_NETWORK_STARTUP    0x00  // Network startup/validation
#define CID_DEVICE_NETWORK_STARTUP DEVICE_TYPE_DSP
#define CID_TYPE_NETWORK_STARTUP CID_TYPE_S | CID_TYPE_R

#define CID_RESPONSE           0x01  // Response command
#define CID_DEVICE_RESPONSE    (DEVICE_TYPE_DSP | DEVICE_TYPE_AMP)
#define CID_TYPE_RESPONSE      CID_TYPE_R

#define CID_REQUEST_INFO       0x02  // Request info
#define CID_DEVICE_REQUEST_INFO DEVICE_TYPE_ALL
#define CID_TYPE_REQUEST_INFO  CID_TYPE_S

#define CID_COMMUNICATION_ERROR 0x03  // Communication error
#define CID_DEVICE_COMMUNICATION_ERROR (DEVICE_TYPE_DSP | DEVICE_TYPE_AMP)
#define CID_TYPE_COMMUNICATION_ERROR CID_TYPE_R
// 0x04-0x0F are reserved

// Audio control CIDs (0x10-0x1F)
#define CID_AUDIO_VIDEO_SOURCE_NAME 0x10  // Audio/Video source name
#define CID_DEVICE_AUDIO_VIDEO_SOURCE_NAME DEVICE_TYPE_DSP
#define CID_TYPE_AUDIO_VIDEO_SOURCE_NAME CID_TYPE_R

#define CID_AUDIO_SOURCE_TYPE 0x11  // Audio source type and capabilities
#define CID_DEVICE_AUDIO_SOURCE_TYPE DEVICE_TYPE_DSP
#define CID_TYPE_AUDIO_SOURCE_TYPE CID_TYPE_R

#define CID_PLAY_STATUS      0x12  // Play status
#define CID_DEVICE_PLAY_STATUS DEVICE_TYPE_DSP
#define CID_TYPE_PLAY_STATUS (CID_TYPE_S | CID_TYPE_R)

#define CID_ZONE_VOLUME      0x13  // Zone volume absolute
#define CID_DEVICE_ZONE_VOLUME DEVICE_TYPE_DSP
#define CID_TYPE_ZONE_VOLUME (CID_TYPE_S | CID_TYPE_R)

#define CID_ZONE_VOLUME_STEP 0x14  // Zone volume step
#define CID_DEVICE_ZONE_VOLUME_STEP DEVICE_TYPE_DSP
#define CID_TYPE_ZONE_VOLUME_STEP (CID_TYPE_S | CID_TYPE_R)

#define CID_MUTE_ZONE        0x15  // Mute zone
#define CID_DEVICE_MUTE_ZONE DEVICE_TYPE_DSP
#define CID_TYPE_MUTE_ZONE   (CID_TYPE_S | CID_TYPE_R)

#define CID_MUTE_CHANNELS    0x16  // Mute channels
#define CID_DEVICE_MUTE_CHANNELS (DEVICE_TYPE_DSP | DEVICE_TYPE_AMP)
#define CID_TYPE_MUTE_CHANNELS (CID_TYPE_S | CID_TYPE_R)

#define CID_ELAPSED_TRACK_TIME 0x17  // Elapsed track/chapter time
#define CID_DEVICE_ELAPSED_TRACK_TIME DEVICE_TYPE_HU
#define CID_TYPE_ELAPSED_TRACK_TIME (CID_TYPE_S | CID_TYPE_R)

#define CID_TRACK_TIME       0x18  // Track/chapter time
#define CID_DEVICE_TRACK_TIME DEVICE_TYPE_HU
#define CID_TYPE_TRACK_TIME  (CID_TYPE_S | CID_TYPE_R)

#define CID_REPEAT_SUPPORT   0x19  // Repeat support
#define CID_DEVICE_REPEAT_SUPPORT DEVICE_TYPE_HU
#define CID_TYPE_REPEAT_SUPPORT (CID_TYPE_S | CID_TYPE_R)

#define CID_SHUFFLE_SUPPORT  0x1A  // Shuffle support
#define CID_DEVICE_SHUFFLE_SUPPORT DEVICE_TYPE_HU
#define CID_TYPE_SHUFFLE_SUPPORT (CID_TYPE_S | CID_TYPE_R)
// 0x1B-0x1F are reserved

// Media Library Data CIDs (0x20-0x29)
#define CID_LIBRARY_DATA_TYPE 0x20  // Library data type
#define CID_DEVICE_LIBRARY_DATA_TYPE DEVICE_TYPE_HU
#define CID_TYPE_LIBRARY_DATA_TYPE (CID_TYPE_S | CID_TYPE_R)

#define CID_LIBRARY_DATA_NAME 0x21  // Library data name
#define CID_DEVICE_LIBRARY_DATA_NAME DEVICE_TYPE_HU
#define CID_TYPE_LIBRARY_DATA_NAME (CID_TYPE_S | CID_TYPE_R)

#define CID_ARTIST_NAME      0x22  // Artist name
#define CID_DEVICE_ARTIST_NAME DEVICE_TYPE_HU
#define CID_TYPE_ARTIST_NAME (CID_TYPE_S | CID_TYPE_R)

#define CID_ALBUM_NAME       0x23  // Album name
#define CID_DEVICE_ALBUM_NAME DEVICE_TYPE_HU
#define CID_TYPE_ALBUM_NAME  (CID_TYPE_S | CID_TYPE_R)

#define CID_STATION_NAME     0x24  // Station name
#define CID_DEVICE_STATION_NAME DEVICE_TYPE_HU
#define CID_TYPE_STATION_NAME (CID_TYPE_S | CID_TYPE_R)
// 0x25-0x29 are reserved

// System and Zone CIDs (0x2A-0x2F)
#define CID_NODE_ENABLE      0x2A  // Node enable
#define CID_DEVICE_NODE_ENABLE (DEVICE_TYPE_DSP | DEVICE_TYPE_AMP)
#define CID_TYPE_NODE_ENABLE_DSP (CID_TYPE_S | CID_TYPE_R)
#define CID_TYPE_NODE_ENABLE_AMP CID_TYPE_R
#define CID_TYPE_NODE_ENABLE CID_TYPE_NODE_ENABLE_DSP

#define CID_TOTAL_ZONES      0x2B  // Total number of zones available
#define CID_DEVICE_TOTAL_ZONES (DEVICE_TYPE_DSP | DEVICE_TYPE_HU)
#define CID_TYPE_TOTAL_ZONES (CID_TYPE_S | CID_TYPE_R)

#define CID_ZONE_NAME        0x2C  // Zone name
#define CID_DEVICE_ZONE_NAME DEVICE_TYPE_HU
#define CID_TYPE_ZONE_NAME   (CID_TYPE_S | CID_TYPE_R)
// 0x2D-0x2F are reserved

// Equalization CIDs (0x30-0x3F)
#define CID_MAIN_SUB_SWITCHING 0x30  // Main/Sub switching
#define CID_DEVICE_MAIN_SUB_SWITCHING DEVICE_TYPE_ALL
#define CID_TYPE_MAIN_SUB_SWITCHING CID_TYPE_S

#define CID_EQ_PRESET_NAME   0x31  // EQ preset name
#define CID_DEVICE_EQ_PRESET_NAME (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_EQ_PRESET_NAME (CID_TYPE_S | CID_TYPE_R)

#define CID_EQ_BASS          0x32  // Equalizer bass
#define CID_DEVICE_EQ_BASS   (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_EQ_BASS_HU  (CID_TYPE_S | CID_TYPE_R)
#define CID_TYPE_EQ_BASS_DSP CID_TYPE_R
#define CID_TYPE_EQ_BASS     CID_TYPE_EQ_BASS_HU

#define CID_EQ_TREBLE        0x33  // Equalizer treble
#define CID_DEVICE_EQ_TREBLE (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_EQ_TREBLE_HU (CID_TYPE_S | CID_TYPE_R)
#define CID_TYPE_EQ_TREBLE_DSP CID_TYPE_R
#define CID_TYPE_EQ_TREBLE   CID_TYPE_EQ_TREBLE_HU

#define CID_EQ_MID_RANGE     0x34  // Equalizer mid range
#define CID_DEVICE_EQ_MID_RANGE (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_EQ_MID_RANGE_HU (CID_TYPE_S | CID_TYPE_R)
#define CID_TYPE_EQ_MID_RANGE_DSP CID_TYPE_R
#define CID_TYPE_EQ_MID_RANGE CID_TYPE_EQ_MID_RANGE_HU

#define CID_BALANCE          0x35  // Balance
#define CID_DEVICE_BALANCE   (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_BALANCE_HU  (CID_TYPE_S | CID_TYPE_R)
#define CID_TYPE_BALANCE_DSP CID_TYPE_R
#define CID_TYPE_BALANCE     CID_TYPE_BALANCE_HU

#define CID_FADE             0x36  // Fade
#define CID_DEVICE_FADE      (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_FADE_HU     (CID_TYPE_S | CID_TYPE_R)
#define CID_TYPE_FADE_DSP    CID_TYPE_R
#define CID_TYPE_FADE        CID_TYPE_FADE_HU

#define CID_SUB_VOLUME       0x37  // Non-fader, sub volume
#define CID_DEVICE_SUB_VOLUME (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_SUB_VOLUME_HU (CID_TYPE_S | CID_TYPE_R)
#define CID_TYPE_SUB_VOLUME_DSP CID_TYPE_R
#define CID_TYPE_SUB_VOLUME  CID_TYPE_SUB_VOLUME_HU

#define CID_SUBWOOFER_SWITCH 0x38  // Subwoofer switch
#define CID_DEVICE_SUBWOOFER_SWITCH DEVICE_TYPE_ALL
#define CID_TYPE_SUBWOOFER_SWITCH CID_TYPE_S

#define CID_CENTER_SWITCH    0x39  // Center switch
#define CID_DEVICE_CENTER_SWITCH DEVICE_TYPE_ALL
#define CID_TYPE_CENTER_SWITCH CID_TYPE_S

#define CID_TONE_BATCH       0x3A  // Tone batch
#define CID_DEVICE_TONE_BATCH DEVICE_TYPE_ALL
#define CID_TYPE_TONE_BATCH  CID_TYPE_S

#define CID_BEEP_VOLUME      0x3B  // Beep volume
#define CID_DEVICE_BEEP_VOLUME DEVICE_TYPE_ALL
#define CID_TYPE_BEEP_VOLUME CID_TYPE_S

#define CID_SPEED_COMP       0x3C  // Speed compensation
#define CID_DEVICE_SPEED_COMP (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_SPEED_COMP_HU (CID_TYPE_S | CID_TYPE_R)
#define CID_TYPE_SPEED_COMP_DSP CID_TYPE_R
#define CID_TYPE_SPEED_COMP  CID_TYPE_SPEED_COMP_HU

#define CID_OVERHEAD_SWITCH  0x3D  // Overhead switch
#define CID_DEVICE_OVERHEAD_SWITCH DEVICE_TYPE_ALL
#define CID_TYPE_OVERHEAD_SWITCH CID_TYPE_S

#define CID_ANC_ZONE_ENABLE  0x3E  // ANC zone enable
#define CID_DEVICE_ANC_ZONE_ENABLE (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_ANC_ZONE_ENABLE_HU (CID_TYPE_S | CID_TYPE_R)
#define CID_TYPE_ANC_ZONE_ENABLE_DSP CID_TYPE_R
#define CID_TYPE_ANC_ZONE_ENABLE CID_TYPE_ANC_ZONE_ENABLE_HU

#define CID_BEEP             0x3F  // Beep
#define CID_DEVICE_BEEP      DEVICE_TYPE_ALL
#define CID_TYPE_BEEP        CID_TYPE_S

// Voice and Bluetooth related CIDs (0x40-0x56)
#define CID_VOICE_OUTPUT     0x40  // Voice output
#define CID_DEVICE_VOICE_OUTPUT DEVICE_TYPE_ALL
#define CID_TYPE_VOICE_OUTPUT CID_TYPE_S
// 0x41-0x45 are reserved

#define CID_BT_ADDR_AVAILABLE 0x46  // Number of Bluetooth addresses available
#define CID_DEVICE_BT_ADDR_AVAILABLE DEVICE_TYPE_HU
#define CID_TYPE_BT_ADDR_AVAILABLE (CID_TYPE_S | CID_TYPE_R)

#define CID_BT_DEVICE_ADDR   0x47  // Bluetooth device address
#define CID_DEVICE_BT_DEVICE_ADDR (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_BT_DEVICE_ADDR (CID_TYPE_S | CID_TYPE_R)

#define CID_BT_DEVICE_STATUS 0x48  // Bluetooth device status
#define CID_DEVICE_BT_DEVICE_STATUS (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_BT_DEVICE_STATUS (CID_TYPE_S | CID_TYPE_R)

#define CID_BT_DEVICE_NAME   0x49  // Bluetooth device name
#define CID_DEVICE_BT_DEVICE_NAME (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_BT_DEVICE_NAME (CID_TYPE_S | CID_TYPE_R)

#define CID_BT_PAIRING_STATUS 0x50  // Bluetooth pairing status
#define CID_DEVICE_BT_PAIRING_STATUS (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_BT_PAIRING_STATUS (CID_TYPE_S | CID_TYPE_R)

#define CID_BT_FORGET_DEVICE 0x51  // Forget Bluetooth device
#define CID_DEVICE_BT_FORGET_DEVICE (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_BT_FORGET_DEVICE (CID_TYPE_S | CID_TYPE_R)

#define CID_BT_DISCOVERING   0x56  // Bluetooth discovering
#define CID_DEVICE_BT_DISCOVERING (DEVICE_TYPE_HU | DEVICE_TYPE_DSP)
#define CID_TYPE_BT_DISCOVERING (CID_TYPE_S | CID_TYPE_R)
// 0x57-0x5F are reserved

// Audio configuration and PLL Lock CIDs (0x60-0x6F)
#define CID_CHANNEL_SLOT     0x60  // Channel slot assignment
#define CID_DEVICE_CHANNEL_SLOT (DEVICE_TYPE_HU | DEVICE_TYPE_DSP | DEVICE_TYPE_AMP)
#define CID_TYPE_CHANNEL_SLOT_HU (CID_TYPE_S | CID_TYPE_R)
#define CID_TYPE_CHANNEL_SLOT_DSP (CID_TYPE_S | CID_TYPE_R)
#define CID_TYPE_CHANNEL_SLOT_AMP CID_TYPE_R
#define CID_TYPE_CHANNEL_SLOT CID_TYPE_CHANNEL_SLOT_HU

#define CID_PLL_LOCK_STATUS  0x61  // PLL lock status
#define CID_DEVICE_PLL_LOCK_STATUS DEVICE_TYPE_ALL
#define CID_TYPE_PLL_LOCK_STATUS CID_TYPE_R
// 0x62-0x65 are reserved

// Sensor CIDs (0x66-0x6F)
#define CID_INPUT_VOLTAGE    0x66  // Input voltage
#define CID_DEVICE_INPUT_VOLTAGE DEVICE_TYPE_ALL
#define CID_TYPE_INPUT_VOLTAGE CID_TYPE_R

#define CID_INPUT_CURRENT    0x67  // Input current
#define CID_DEVICE_INPUT_CURRENT DEVICE_TYPE_ALL
#define CID_TYPE_INPUT_CURRENT CID_TYPE_R

#define CID_TEMPERATURE      0x68  // Temperature input filter
#define CID_DEVICE_TEMPERATURE DEVICE_TYPE_ALL
#define CID_TYPE_TEMPERATURE CID_TYPE_R

#define CID_SENSOR_GENERIC   0x69  // Sensor generic
#define CID_DEVICE_SENSOR_GENERIC_ALL (DEVICE_TYPE_ALL & ~DEVICE_TYPE_DSP)  // ALL except DSP
#define CID_DEVICE_SENSOR_GENERIC_DSP DEVICE_TYPE_DSP  // DSP only
#define CID_DEVICE_SENSOR_GENERIC (CID_DEVICE_SENSOR_GENERIC_ALL | CID_DEVICE_SENSOR_GENERIC_DSP)
#define CID_TYPE_SENSOR_GENERIC_ALL CID_TYPE_R  // Receiver only for ALL except DSP
#define CID_TYPE_SENSOR_GENERIC_DSP (CID_TYPE_S | CID_TYPE_R)  // Both S/R for DSP
#define CID_TYPE_SENSOR_GENERIC CID_TYPE_SENSOR_GENERIC_ALL  // Default to ALL behavior
// 0x6A-0x6F are reserved

// Module configuration CIDs (0x70-0x7F)
#define CID_MODULE_ENABLE    0x70  // Module enable
#define CID_DEVICE_MODULE_ENABLE_ALL (DEVICE_TYPE_ALL & ~(DEVICE_TYPE_DSP | DEVICE_TYPE_HU))  // ALL except DSP/HU
#define CID_DEVICE_MODULE_ENABLE_DSP_HU (DEVICE_TYPE_DSP | DEVICE_TYPE_HU)  // DSP and HU
#define CID_DEVICE_MODULE_ENABLE (CID_DEVICE_MODULE_ENABLE_ALL | CID_DEVICE_MODULE_ENABLE_DSP_HU)
#define CID_TYPE_MODULE_ENABLE_ALL CID_TYPE_R  // Receiver only for ALL except DSP/HU
#define CID_TYPE_MODULE_ENABLE_DSP_HU (CID_TYPE_S | CID_TYPE_R)  // Both S/R for DSP and HU
#define CID_TYPE_MODULE_ENABLE CID_TYPE_MODULE_ENABLE_DSP_HU  // Default to DSP/HU behavior

#define CID_POWER_SUPPLY_ENABLE 0x71  // Power supply enable
#define CID_DEVICE_POWER_SUPPLY_ENABLE DEVICE_TYPE_ALL
#define CID_TYPE_POWER_SUPPLY_ENABLE CID_TYPE_S

#define CID_RCA_ENABLE       0x72  // RCA enable
#define CID_DEVICE_RCA_ENABLE DEVICE_TYPE_ALL
#define CID_TYPE_RCA_ENABLE  CID_TYPE_S

#define CID_SOFT_START       0x73  // Soft start
#define CID_DEVICE_SOFT_START DEVICE_TYPE_ALL
#define CID_TYPE_SOFT_START  CID_TYPE_S

#define CID_UNDERVOLTAGE_THRESHOLD 0x74  // Undervoltage threshold
#define CID_DEVICE_UNDERVOLTAGE_THRESHOLD DEVICE_TYPE_ALL
#define CID_TYPE_UNDERVOLTAGE_THRESHOLD CID_TYPE_S

#define CID_OVERVOLTAGE_THRESHOLD 0x75  // Overvoltage threshold
#define CID_DEVICE_OVERVOLTAGE_THRESHOLD DEVICE_TYPE_ALL
#define CID_TYPE_OVERVOLTAGE_THRESHOLD CID_TYPE_S

#define CID_BT_WIFI_RESERVED 0x76  // Reserved for Bluetooth and Wi-Fi
#define CID_DEVICE_BT_WIFI_RESERVED DEVICE_TYPE_ALL
#define CID_TYPE_BT_WIFI_RESERVED (CID_TYPE_S | CID_TYPE_R)
// 0x77-0x7F are reserved

// Protection and diagnostics CIDs (0x80-0x8F)
#define CID_MODULE_STATUS    0x80  // Module status
#define CID_DEVICE_MODULE_STATUS_ALL (DEVICE_TYPE_ALL & ~(DEVICE_TYPE_DSP | DEVICE_TYPE_HU))  // ALL except DSP/HU
#define CID_DEVICE_MODULE_STATUS_DSP_HU (DEVICE_TYPE_DSP | DEVICE_TYPE_HU)  // DSP and HU
#define CID_DEVICE_MODULE_STATUS (CID_DEVICE_MODULE_STATUS_ALL | CID_DEVICE_MODULE_STATUS_DSP_HU)
#define CID_TYPE_MODULE_STATUS_ALL (CID_TYPE_S | CID_TYPE_R)  // Both S/R for ALL except DSP/HU
#define CID_TYPE_MODULE_STATUS_DSP_HU CID_TYPE_R  // Receiver only for DSP and HU
#define CID_TYPE_MODULE_STATUS CID_TYPE_MODULE_STATUS_ALL  // Default to ALL behavior

#define CID_CHANNEL_CLIP     0x81  // Channel clip detection
#define CID_DEVICE_CHANNEL_CLIP DEVICE_TYPE_AMP
#define CID_TYPE_CHANNEL_CLIP (CID_TYPE_S | CID_TYPE_R)

#define CID_CHANNEL_SHORT    0x82  // Channel short detection
#define CID_DEVICE_CHANNEL_SHORT DEVICE_TYPE_AMP
#define CID_TYPE_CHANNEL_SHORT (CID_TYPE_S | CID_TYPE_R)

#define CID_CHANNEL_OPEN     0x83  // Channel open detection
#define CID_DEVICE_CHANNEL_OPEN DEVICE_TYPE_AMP
#define CID_TYPE_CHANNEL_OPEN (CID_TYPE_S | CID_TYPE_R)

#define CID_SHARC_STATUS     0x84  // SHARC status
#define CID_DEVICE_SHARC_STATUS DEVICE_TYPE_DSP
#define CID_TYPE_SHARC_STATUS CID_TYPE_R
// 0x85-0x8F are reserved

// Response command status codes
#define RSP_STATUS_COMPLETION    0  // Completion response
#define RSP_STATUS_NOT_SUPPORTED 1  // CID not supported
#define RSP_STATUS_PARAM_ERROR   2  // Parameter error (out of range)
#define RSP_STATUS_BUSY          3  // Busy (command in execution)
#define RSP_STATUS_EXEC_FAILURE  4  // Execution failure
// 5-225 are reserved

// Function declarations
void aabcop_init(void);
bool aabcop_process_mailbox(uint8_t *mailbox, uint8_t *response);
void aabcop_set_state(protocol_state_t new_state);
protocol_state_t aabcop_get_state(void);

// Helper functions for checking CID properties
bool aabcop_is_sender_cid(uint8_t cid, uint8_t device_type);
bool aabcop_is_receiver_cid(uint8_t cid, uint8_t device_type);
uint8_t aabcop_get_cid_device_type(uint8_t cid);
uint8_t aabcop_get_cid_message_type(uint8_t cid);
const char* aabcop_get_cid_name(uint8_t cid);
void aabcop_print_cid_description(uint8_t cid);
void aabcop_print_cid_info(uint8_t cid, uint8_t* data, uint8_t dataLen);

// Frame creation and parsing functions
void aabcop_create_frames(uint8_t uid, uint8_t cid, uint8_t* data, uint8_t dataLength, 
                         void (*write_byte_callback)(uint8_t byte, void* ctx), void* ctx);
bool aabcop_parse_frame(uint8_t* mailbox, uint8_t* cid, uint8_t** data, uint8_t* dataIndex, uint8_t* dataSize);

#endif /* AABCOP_H */
