#ifndef AABCOP_H
#define AABCOP_H

#include <stdint.h>
#include <stdbool.h>

// Constants for AABCOP protocol
#define UID_DANG8100           0x60
#define MAILBOX_SIZE           4

// CID Message Type
#define CID_TYPE_S             0x01    // Sender messages
#define CID_TYPE_R             0x02    // Receiver messages
#define CID_TYPE_RS            0x03    // Both Sender and Receiver messages

// Network generic CIDs (0x00-0x0F)
#define CID_NETWORK_STARTUP    0x00
#define CID_TYPE_NETWORK_STARTUP CID_TYPE_S
#define CID_RESPONSE           0x01
#define CID_TYPE_RESPONSE      CID_TYPE_R
#define CID_COMMUNICATION_ERROR 0x02
#define CID_TYPE_COMMUNICATION_ERROR CID_TYPE_RS
// 0x03-0x0F are reserved

// Audio control CIDs (0x10-0x1F)
#define CID_AUDIO_VIDEO_SOURCE_NAME 0x10
#define CID_TYPE_AUDIO_VIDEO_SOURCE_NAME CID_TYPE_R
#define CID_AUDIO_SOURCE_TYPE 0x11
#define CID_TYPE_AUDIO_SOURCE_TYPE CID_TYPE_R
#define CID_PLAY_STATUS      0x12
#define CID_TYPE_PLAY_STATUS CID_TYPE_R
#define CID_ZONE_VOLUME      0x13
#define CID_TYPE_ZONE_VOLUME CID_TYPE_S
#define CID_ZONE_VOLUME_STEP 0x14
#define CID_TYPE_ZONE_VOLUME_STEP CID_TYPE_S
#define CID_MUTE_ZONE        0x15
#define CID_TYPE_MUTE_ZONE   CID_TYPE_S
#define CID_MUTE_CHANNELS    0x16
#define CID_TYPE_MUTE_CHANNELS CID_TYPE_S
#define CID_ELAPSED_TRACK_TIME 0x17
#define CID_TYPE_ELAPSED_TRACK_TIME CID_TYPE_R
#define CID_TRACK_TIME       0x18
#define CID_TYPE_TRACK_TIME  CID_TYPE_R
#define CID_REPEAT_SUPPORT   0x19
#define CID_TYPE_REPEAT_SUPPORT CID_TYPE_RS
#define CID_SHUFFLE_SUPPORT  0x1A
#define CID_TYPE_SHUFFLE_SUPPORT CID_TYPE_RS
// 0x1B-0x1F are reserved

// Audio inputs CIDs (0x20-0x2F)
#define CID_LIBRARY_DATA_TYPE 0x20
#define CID_TYPE_LIBRARY_DATA_TYPE CID_TYPE_S
#define CID_LIBRARY_DATA_NAME 0x21
#define CID_TYPE_LIBRARY_DATA_NAME CID_TYPE_R
#define CID_ARTIST_NAME      0x22
#define CID_TYPE_ARTIST_NAME CID_TYPE_R
#define CID_ALBUM_NAME       0x23
#define CID_TYPE_ALBUM_NAME  CID_TYPE_R
#define CID_STATION_NAME     0x24
#define CID_TYPE_STATION_NAME CID_TYPE_R
// 0x25-0x29 are reserved
#define CID_POWER            0x2A
#define CID_TYPE_POWER       CID_TYPE_RS
#define CID_TOTAL_ZONES      0x2B
#define CID_TYPE_TOTAL_ZONES CID_TYPE_R
#define CID_ZONE_NAME        0x2C
#define CID_TYPE_ZONE_NAME   CID_TYPE_R
// 0x2D-0x2F are reserved

// Equalization CIDs (0x30-0x3F)
#define CID_MAIN_SUB_SWITCHING 0x30
#define CID_TYPE_MAIN_SUB_SWITCHING CID_TYPE_S
#define CID_EQ_PRESET_NAME   0x31
#define CID_TYPE_EQ_PRESET_NAME CID_TYPE_R
#define CID_EQ_BASS          0x32
#define CID_TYPE_EQ_BASS     CID_TYPE_S
#define CID_EQ_TREBLE        0x33
#define CID_TYPE_EQ_TREBLE   CID_TYPE_S
#define CID_EQ_MID_RANGE     0x34
#define CID_TYPE_EQ_MID_RANGE CID_TYPE_S
#define CID_BALANCE          0x35
#define CID_TYPE_BALANCE     CID_TYPE_S
#define CID_FADE             0x36
#define CID_TYPE_FADE        CID_TYPE_S
#define CID_SUB_VOLUME       0x37
#define CID_TYPE_SUB_VOLUME  CID_TYPE_S
#define CID_SUBWOOFER_SWITCH 0x38
#define CID_TYPE_SUBWOOFER_SWITCH CID_TYPE_S
#define CID_CENTER_SWITCH    0x39
#define CID_TYPE_CENTER_SWITCH CID_TYPE_S
#define CID_TONE_BATCH       0x3A
#define CID_TYPE_TONE_BATCH  CID_TYPE_S
#define CID_BEEP_VOLUME      0x3B
#define CID_TYPE_BEEP_VOLUME CID_TYPE_S
#define CID_SPEED_COMP       0x3C
#define CID_TYPE_SPEED_COMP  CID_TYPE_S
#define CID_OVERHEAD_SWITCH  0x3D
#define CID_TYPE_OVERHEAD_SWITCH CID_TYPE_S
#define CID_ANC_ZONE_ENABLE  0x3E
#define CID_TYPE_ANC_ZONE_ENABLE CID_TYPE_S
#define CID_BEEP             0x3F
#define CID_TYPE_BEEP        CID_TYPE_S

// Bluetooth related CIDs (0x45-0x56)
#define CID_VOICE_OUTPUT     0x40
#define CID_TYPE_VOICE_OUTPUT CID_TYPE_S
// 0x41-0x45 are reserved
#define CID_BT_ADDR_AVAILABLE 0x46
#define CID_TYPE_BT_ADDR_AVAILABLE CID_TYPE_R
#define CID_BT_DEVICE_ADDR   0x47
#define CID_TYPE_BT_DEVICE_ADDR CID_TYPE_S
#define CID_BT_DEVICE_STATUS 0x48
#define CID_TYPE_BT_DEVICE_STATUS CID_TYPE_R
#define CID_BT_DEVICE_NAME   0x49
#define CID_TYPE_BT_DEVICE_NAME CID_TYPE_R
#define CID_BT_PAIRING_STATUS 0x50
#define CID_TYPE_BT_PAIRING_STATUS CID_TYPE_RS
#define CID_BT_FORGET_DEVICE 0x51
#define CID_TYPE_BT_FORGET_DEVICE CID_TYPE_S
#define CID_BT_DISCOVERING   0x56
#define CID_TYPE_BT_DISCOVERING CID_TYPE_S
// 0x57-0x5F are reserved

// Audio configuration and PLL Lock CIDs (0x60-0x6F)
#define CID_CHANNEL_SLOT     0x60
#define CID_TYPE_CHANNEL_SLOT CID_TYPE_S
#define CID_PLL_LOCK_STATUS  0x61
#define CID_TYPE_PLL_LOCK_STATUS CID_TYPE_R
// 0x62-0x65 are reserved
#define CID_INPUT_VOLTAGE    0x66
#define CID_TYPE_INPUT_VOLTAGE CID_TYPE_R
#define CID_INPUT_CURRENT    0x67
#define CID_TYPE_INPUT_CURRENT CID_TYPE_R
#define CID_TEMPERATURE      0x68
#define CID_TYPE_TEMPERATURE CID_TYPE_R
#define CID_SENSOR_GENERIC   0x69
#define CID_TYPE_SENSOR_GENERIC CID_TYPE_R
// 0x6A-0x6F are reserved

// Module configuration CIDs (0x70-0x7F)
#define CID_MODULE_ENABLE    0x70
#define CID_TYPE_MODULE_ENABLE CID_TYPE_S
#define CID_POWER_SUPPLY_ENABLE 0x71
#define CID_TYPE_POWER_SUPPLY_ENABLE CID_TYPE_S
#define CID_RCA_ENABLE       0x72
#define CID_TYPE_RCA_ENABLE  CID_TYPE_S
#define CID_SOFT_START       0x73
#define CID_TYPE_SOFT_START  CID_TYPE_S
#define CID_UNDERVOLTAGE_THRESHOLD 0x74
#define CID_TYPE_UNDERVOLTAGE_THRESHOLD CID_TYPE_S
#define CID_OVERVOLTAGE_THRESHOLD 0x75
#define CID_TYPE_OVERVOLTAGE_THRESHOLD CID_TYPE_S
#define CID_BT_WIFI_RESERVED 0x76
#define CID_TYPE_BT_WIFI_RESERVED CID_TYPE_RS
// 0x77-0x7F are reserved

// Protection and diagnostics CIDs (0x80-0x8F)
#define CID_MODULE_STATUS    0x80
#define CID_TYPE_MODULE_STATUS CID_TYPE_R
#define CID_CHANNEL_STATUS   0x81
#define CID_TYPE_CHANNEL_STATUS CID_TYPE_R
// 0x82-0x8F are reserved

// Response command status codes
#define RSP_STATUS_COMPLETION    0
#define RSP_STATUS_NOT_SUPPORTED 1
#define RSP_STATUS_PARAM_ERROR   2
#define RSP_STATUS_BUSY          3
#define RSP_STATUS_EXEC_FAILURE  4
// 5-225 are reserved

// Protocol states
typedef enum {
    STATE_INIT,
    STATE_DISCOVERY,
    STATE_RUNNING,
    STATE_ERROR
} protocol_state_t;

// Function declarations
void aabcop_init(void);
bool aabcop_process_mailbox(uint8_t *mailbox, uint8_t *response);
void aabcop_set_state(protocol_state_t new_state);
protocol_state_t aabcop_get_state(void);

#endif /* AABCOP_H */