#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "aabcop_new.h"

// Static variables
static protocol_state_t aabcop_state = STATE_INIT;
static uint8_t message_buffer[256];  // Buffer for reassembling multi-frame messages
static uint8_t message_index = 0;
static uint8_t expected_frames = 0;
static uint8_t received_frames = 0;

// Implementation of helper functions for checking CID properties
bool aabcop_is_sender_cid(uint8_t cid, uint8_t device_type) {
    uint8_t cid_device_type = aabcop_get_cid_device_type(cid);
    uint8_t cid_message_type;
    
    // Special cases for different device types based on spec
    if (device_type == DEVICE_TYPE_DSP) {
        switch(cid) {
            case CID_SUB_VOLUME:
            case CID_EQ_BASS:
            case CID_EQ_TREBLE:
            case CID_EQ_MID_RANGE:
            case CID_BALANCE:
            case CID_FADE:
            case CID_SPEED_COMP:
            case CID_ANC_ZONE_ENABLE:
            case CID_MODULE_STATUS:  // Updated as per v2.0_CIDs
                // DSP is not a sender for these CIDs
                return false;
            case CID_SENSOR_GENERIC:  // Updated as per v2.0_CIDs
                // DSP is a sender for this CID
                return true;
            default:
                break;
        }
    } else if (device_type == DEVICE_TYPE_AMP) {
        switch(cid) {
            case CID_NODE_ENABLE:
            case CID_CHANNEL_SLOT:
            case CID_MUTE_CHANNELS:
            case CID_MODULE_ENABLE:  // Updated as per v2.0_CIDs
                // AMP is not a sender for these CIDs
                return false;
            default:
                break;
        }
    } else if ((device_type & DEVICE_TYPE_ALL) && !(device_type & (DEVICE_TYPE_DSP | DEVICE_TYPE_HU))) {
        // Special cases for "ALL except DSP/HU"
        switch(cid) {
            case CID_SENSOR_GENERIC:
                // ALL except DSP is not a sender for this CID
                return false;
            case CID_MODULE_STATUS:
                // ALL except DSP/HU is a sender for this CID
                return true;
            default:
                break;
        }
    }
    
    cid_message_type = aabcop_get_cid_message_type(cid);
    
    // Check if the device type matches and CID is a sender type (S or S+R)
    return ((cid_device_type & device_type) != 0) && 
           ((cid_message_type & CID_TYPE_S) != 0);
}

bool aabcop_is_receiver_cid(uint8_t cid, uint8_t device_type) {
    uint8_t cid_device_type = aabcop_get_cid_device_type(cid);
    uint8_t cid_message_type;
    
    // Special handling for device-specific receiver capabilities
    if (device_type == DEVICE_TYPE_DSP) {
        switch(cid) {
            case CID_SUB_VOLUME:
            case CID_EQ_BASS:
            case CID_EQ_TREBLE:
            case CID_EQ_MID_RANGE:
            case CID_BALANCE:
            case CID_FADE:
            case CID_SPEED_COMP:
            case CID_ANC_ZONE_ENABLE:
            case CID_MODULE_STATUS:  // Updated as per v2.0_CIDs
                // DSP is a receiver only for these CIDs
                return true;
            default:
                break;
        }
    } else if (device_type == DEVICE_TYPE_AMP) {
        switch(cid) {
            case CID_NODE_ENABLE:
            case CID_CHANNEL_SLOT:
            case CID_MUTE_CHANNELS:
            case CID_MODULE_STATUS:
            case CID_MODULE_ENABLE:  // Updated as per v2.0_CIDs
                // AMP is a receiver only for these CIDs
                return true;
            default:
                break;
        }
    } else if ((device_type & DEVICE_TYPE_ALL) && !(device_type & (DEVICE_TYPE_DSP))) {
        // Special cases for "ALL except DSP"
        switch(cid) {
            case CID_SENSOR_GENERIC:
                // ALL except DSP is a receiver for this CID
                return true;
            default:
                break;
        }
    } else if ((device_type & DEVICE_TYPE_ALL) && !(device_type & (DEVICE_TYPE_DSP | DEVICE_TYPE_HU))) {
        // Special cases for "ALL except DSP/HU"
        switch(cid) {
            case CID_MODULE_ENABLE:
                // ALL except DSP/HU is a receiver for this CID
                return true;
            default:
                break;
        }
    }
    
    cid_message_type = aabcop_get_cid_message_type(cid);
    
    // Check if the device type matches and CID is a receiver type (R or S+R)
    return ((cid_device_type & device_type) != 0) && 
           ((cid_message_type & CID_TYPE_R) != 0);
}

uint8_t aabcop_get_cid_device_type(uint8_t cid) {
    switch(cid) {
        case CID_NETWORK_STARTUP: return CID_DEVICE_NETWORK_STARTUP;
        case CID_RESPONSE: return CID_DEVICE_RESPONSE;
        case CID_REQUEST_INFO: return CID_DEVICE_REQUEST_INFO;
        case CID_COMMUNICATION_ERROR: return CID_DEVICE_COMMUNICATION_ERROR;
        case CID_AUDIO_VIDEO_SOURCE_NAME: return CID_DEVICE_AUDIO_VIDEO_SOURCE_NAME;
        case CID_AUDIO_SOURCE_TYPE: return CID_DEVICE_AUDIO_SOURCE_TYPE;
        case CID_PLAY_STATUS: return CID_DEVICE_PLAY_STATUS;
        case CID_ZONE_VOLUME: return CID_DEVICE_ZONE_VOLUME;
        case CID_ZONE_VOLUME_STEP: return CID_DEVICE_ZONE_VOLUME_STEP;
        case CID_MUTE_ZONE: return CID_DEVICE_MUTE_ZONE;
        case CID_MUTE_CHANNELS: return CID_DEVICE_MUTE_CHANNELS;
        case CID_ELAPSED_TRACK_TIME: return CID_DEVICE_ELAPSED_TRACK_TIME;
        case CID_TRACK_TIME: return CID_DEVICE_TRACK_TIME;
        case CID_REPEAT_SUPPORT: return CID_DEVICE_REPEAT_SUPPORT;
        case CID_SHUFFLE_SUPPORT: return CID_DEVICE_SHUFFLE_SUPPORT;
        case CID_LIBRARY_DATA_TYPE: return CID_DEVICE_LIBRARY_DATA_TYPE;
        case CID_LIBRARY_DATA_NAME: return CID_DEVICE_LIBRARY_DATA_NAME;
        case CID_ARTIST_NAME: return CID_DEVICE_ARTIST_NAME;
        case CID_ALBUM_NAME: return CID_DEVICE_ALBUM_NAME;
        case CID_STATION_NAME: return CID_DEVICE_STATION_NAME;
        case CID_NODE_ENABLE: return CID_DEVICE_NODE_ENABLE;
        case CID_TOTAL_ZONES: return CID_DEVICE_TOTAL_ZONES;
        case CID_ZONE_NAME: return CID_DEVICE_ZONE_NAME;
        case CID_MAIN_SUB_SWITCHING: return CID_DEVICE_MAIN_SUB_SWITCHING;
        case CID_EQ_PRESET_NAME: return CID_DEVICE_EQ_PRESET_NAME;
        case CID_EQ_BASS: return CID_DEVICE_EQ_BASS;
        case CID_EQ_TREBLE: return CID_DEVICE_EQ_TREBLE;
        case CID_EQ_MID_RANGE: return CID_DEVICE_EQ_MID_RANGE;
        case CID_BALANCE: return CID_DEVICE_BALANCE;
        case CID_FADE: return CID_DEVICE_FADE;
        case CID_SUB_VOLUME: return CID_DEVICE_SUB_VOLUME;
        case CID_SUBWOOFER_SWITCH: return CID_DEVICE_SUBWOOFER_SWITCH;
        case CID_CENTER_SWITCH: return CID_DEVICE_CENTER_SWITCH;
        case CID_TONE_BATCH: return CID_DEVICE_TONE_BATCH;
        case CID_BEEP_VOLUME: return CID_DEVICE_BEEP_VOLUME;
        case CID_SPEED_COMP: return CID_DEVICE_SPEED_COMP;
        case CID_OVERHEAD_SWITCH: return CID_DEVICE_OVERHEAD_SWITCH;
        case CID_ANC_ZONE_ENABLE: return CID_DEVICE_ANC_ZONE_ENABLE;
        case CID_BEEP: return CID_DEVICE_BEEP;
        case CID_VOICE_OUTPUT: return CID_DEVICE_VOICE_OUTPUT;
        case CID_BT_ADDR_AVAILABLE: return CID_DEVICE_BT_ADDR_AVAILABLE;
        case CID_BT_DEVICE_ADDR: return CID_DEVICE_BT_DEVICE_ADDR;
        case CID_BT_DEVICE_STATUS: return CID_DEVICE_BT_DEVICE_STATUS;
        case CID_BT_DEVICE_NAME: return CID_DEVICE_BT_DEVICE_NAME;
        case CID_BT_PAIRING_STATUS: return CID_DEVICE_BT_PAIRING_STATUS;
        case CID_BT_FORGET_DEVICE: return CID_DEVICE_BT_FORGET_DEVICE;
        case CID_BT_DISCOVERING: return CID_DEVICE_BT_DISCOVERING;
        case CID_CHANNEL_SLOT: return CID_DEVICE_CHANNEL_SLOT;
        case CID_PLL_LOCK_STATUS: return CID_DEVICE_PLL_LOCK_STATUS;
        case CID_INPUT_VOLTAGE: return CID_DEVICE_INPUT_VOLTAGE;
        case CID_INPUT_CURRENT: return CID_DEVICE_INPUT_CURRENT;
        case CID_TEMPERATURE: return CID_DEVICE_TEMPERATURE;
        case CID_SENSOR_GENERIC: return CID_DEVICE_SENSOR_GENERIC;
        case CID_MODULE_ENABLE: return CID_DEVICE_MODULE_ENABLE;
        case CID_POWER_SUPPLY_ENABLE: return CID_DEVICE_POWER_SUPPLY_ENABLE;
        case CID_RCA_ENABLE: return CID_DEVICE_RCA_ENABLE;
        case CID_SOFT_START: return CID_DEVICE_SOFT_START;
        case CID_UNDERVOLTAGE_THRESHOLD: return CID_DEVICE_UNDERVOLTAGE_THRESHOLD;
        case CID_OVERVOLTAGE_THRESHOLD: return CID_DEVICE_OVERVOLTAGE_THRESHOLD;
        case CID_BT_WIFI_RESERVED: return CID_DEVICE_BT_WIFI_RESERVED;
        case CID_MODULE_STATUS: return CID_DEVICE_MODULE_STATUS;
        case CID_CHANNEL_CLIP: return CID_DEVICE_CHANNEL_CLIP;
        case CID_CHANNEL_SHORT: return CID_DEVICE_CHANNEL_SHORT;
        case CID_CHANNEL_OPEN: return CID_DEVICE_CHANNEL_OPEN;
        case CID_SHARC_STATUS: return CID_DEVICE_SHARC_STATUS;
        default: return DEVICE_TYPE_ALL; // Default to ALL if unknown
    }
}

uint8_t aabcop_get_cid_message_type(uint8_t cid) {
    // Check for device type and return appropriate message type
    switch(cid) {
        // Network Generic CIDs
        case CID_NETWORK_STARTUP: 
            return CID_TYPE_NETWORK_STARTUP;
        case CID_RESPONSE: 
            return CID_TYPE_RESPONSE;
        case CID_REQUEST_INFO: 
            return CID_TYPE_REQUEST_INFO;
        case CID_COMMUNICATION_ERROR: 
            return CID_TYPE_COMMUNICATION_ERROR;
            
        // Audio Control CIDs
        case CID_AUDIO_VIDEO_SOURCE_NAME: 
            return CID_TYPE_AUDIO_VIDEO_SOURCE_NAME;
        case CID_AUDIO_SOURCE_TYPE: 
            return CID_TYPE_AUDIO_SOURCE_TYPE;
        case CID_PLAY_STATUS: 
            return CID_TYPE_PLAY_STATUS;
        case CID_ZONE_VOLUME: 
            return CID_TYPE_ZONE_VOLUME;
        case CID_ZONE_VOLUME_STEP: 
            return CID_TYPE_ZONE_VOLUME_STEP;
        case CID_MUTE_ZONE: 
            return CID_TYPE_MUTE_ZONE;
        case CID_MUTE_CHANNELS: 
            // Special handling for MUTE_CHANNELS
            return CID_TYPE_MUTE_CHANNELS;
        case CID_ELAPSED_TRACK_TIME: 
            return CID_TYPE_ELAPSED_TRACK_TIME;
        case CID_TRACK_TIME: 
            return CID_TYPE_TRACK_TIME;
        case CID_REPEAT_SUPPORT: 
            return CID_TYPE_REPEAT_SUPPORT;
        case CID_SHUFFLE_SUPPORT: 
            return CID_TYPE_SHUFFLE_SUPPORT;
            
        // Media Library Data CIDs
        case CID_LIBRARY_DATA_TYPE: 
            return CID_TYPE_LIBRARY_DATA_TYPE;
        case CID_LIBRARY_DATA_NAME: 
            return CID_TYPE_LIBRARY_DATA_NAME;
        case CID_ARTIST_NAME: 
            return CID_TYPE_ARTIST_NAME;
        case CID_ALBUM_NAME: 
            return CID_TYPE_ALBUM_NAME;
        case CID_STATION_NAME: 
            return CID_TYPE_STATION_NAME;
            
        // System and Zone CIDs
        case CID_NODE_ENABLE:
            // Per spec: DSP - S/R, AMP - R only
            return CID_TYPE_NODE_ENABLE;
        case CID_TOTAL_ZONES: 
            return CID_TYPE_TOTAL_ZONES;
        case CID_ZONE_NAME: 
            return CID_TYPE_ZONE_NAME;
            
        // Equalization CIDs
        case CID_MAIN_SUB_SWITCHING: 
            return CID_TYPE_MAIN_SUB_SWITCHING;
        case CID_EQ_PRESET_NAME: 
            return CID_TYPE_EQ_PRESET_NAME;
        case CID_EQ_BASS:
            // Per spec: HU - S/R, DSP - R only
            return CID_TYPE_EQ_BASS;
        case CID_EQ_TREBLE:
            // Per spec: HU - S/R, DSP - R only
            return CID_TYPE_EQ_TREBLE;
        case CID_EQ_MID_RANGE:
            // Per spec: HU - S/R, DSP - R only
            return CID_TYPE_EQ_MID_RANGE;
        case CID_BALANCE:
            // Per spec: HU - S/R, DSP - R only
            return CID_TYPE_BALANCE;
        case CID_FADE:
            // Per spec: HU - S/R, DSP - R only
            return CID_TYPE_FADE;
        case CID_SUB_VOLUME:
            // Per spec: HU - S/R, DSP - R only
            return CID_TYPE_SUB_VOLUME;
        case CID_SUBWOOFER_SWITCH: 
            return CID_TYPE_SUBWOOFER_SWITCH;
        case CID_CENTER_SWITCH: 
            return CID_TYPE_CENTER_SWITCH;
        case CID_TONE_BATCH: 
            return CID_TYPE_TONE_BATCH;
        case CID_BEEP_VOLUME: 
            return CID_TYPE_BEEP_VOLUME;
        case CID_SPEED_COMP:
            // Per spec: HU - S/R, DSP - R only
            return CID_TYPE_SPEED_COMP;
        case CID_OVERHEAD_SWITCH: 
            return CID_TYPE_OVERHEAD_SWITCH;
        case CID_ANC_ZONE_ENABLE:
            // Per spec: HU - S/R, DSP - R only
            return CID_TYPE_ANC_ZONE_ENABLE;
        case CID_BEEP: 
            return CID_TYPE_BEEP;
            
        // Voice and Bluetooth related CIDs
        case CID_VOICE_OUTPUT: 
            return CID_TYPE_VOICE_OUTPUT;
        case CID_BT_ADDR_AVAILABLE: 
            return CID_TYPE_BT_ADDR_AVAILABLE;
        case CID_BT_DEVICE_ADDR: 
            return CID_TYPE_BT_DEVICE_ADDR;
        case CID_BT_DEVICE_STATUS: 
            return CID_TYPE_BT_DEVICE_STATUS;
        case CID_BT_DEVICE_NAME: 
            return CID_TYPE_BT_DEVICE_NAME;
        case CID_BT_PAIRING_STATUS: 
            return CID_TYPE_BT_PAIRING_STATUS;
        case CID_BT_FORGET_DEVICE: 
            return CID_TYPE_BT_FORGET_DEVICE;
        case CID_BT_DISCOVERING: 
            return CID_TYPE_BT_DISCOVERING;
            
        // Audio Configuration and PLL Lock CIDs
        case CID_CHANNEL_SLOT:
            // Per spec: HU/DSP - S/R, AMP - R only
            return CID_TYPE_CHANNEL_SLOT;
        case CID_PLL_LOCK_STATUS: 
            return CID_TYPE_PLL_LOCK_STATUS;
            
        // Sensor CIDs
        case CID_INPUT_VOLTAGE: 
            return CID_TYPE_INPUT_VOLTAGE;
        case CID_INPUT_CURRENT: 
            return CID_TYPE_INPUT_CURRENT;
        case CID_TEMPERATURE: 
            return CID_TYPE_TEMPERATURE;
        case CID_SENSOR_GENERIC: 
            // Check if DSP or ALL except DSP
            if (aabcop_get_cid_device_type(cid) & DEVICE_TYPE_DSP) {
                return CID_TYPE_SENSOR_GENERIC_DSP;
            } else {
                return CID_TYPE_SENSOR_GENERIC_ALL;
            }
            
        // Module Configuration CIDs
        case CID_MODULE_ENABLE: 
            // Check if DSP/HU or ALL except DSP/HU
            if (aabcop_get_cid_device_type(cid) & (DEVICE_TYPE_DSP | DEVICE_TYPE_HU)) {
                return CID_TYPE_MODULE_ENABLE_DSP_HU;
            } else {
                return CID_TYPE_MODULE_ENABLE_ALL;
            }
        case CID_POWER_SUPPLY_ENABLE: 
            return CID_TYPE_POWER_SUPPLY_ENABLE;
        case CID_RCA_ENABLE: 
            return CID_TYPE_RCA_ENABLE;
        case CID_SOFT_START: 
            return CID_TYPE_SOFT_START;
        case CID_UNDERVOLTAGE_THRESHOLD: 
            return CID_TYPE_UNDERVOLTAGE_THRESHOLD;
        case CID_OVERVOLTAGE_THRESHOLD: 
            return CID_TYPE_OVERVOLTAGE_THRESHOLD;
        case CID_BT_WIFI_RESERVED: 
            return CID_TYPE_BT_WIFI_RESERVED;
            
        // Protection and Diagnostics CIDs
        case CID_MODULE_STATUS:
            // Check if DSP/HU or ALL except DSP/HU
            if (aabcop_get_cid_device_type(cid) & (DEVICE_TYPE_DSP | DEVICE_TYPE_HU)) {
                return CID_TYPE_MODULE_STATUS_DSP_HU;  // R only for DSP/HU
            } else {
                return CID_TYPE_MODULE_STATUS_ALL;  // S/R for ALL except DSP/HU
            }
        case CID_CHANNEL_CLIP: 
            return CID_TYPE_CHANNEL_CLIP;
        case CID_CHANNEL_SHORT: 
            return CID_TYPE_CHANNEL_SHORT;
        case CID_CHANNEL_OPEN: 
            return CID_TYPE_CHANNEL_OPEN;
        case CID_SHARC_STATUS: 
            return CID_TYPE_SHARC_STATUS;
            
        default: 
            return 0; // Return 0 if unknown
    }
}

// Get CID name as string
const char* aabcop_get_cid_name(uint8_t cid) {
    switch(cid) {
        case CID_NETWORK_STARTUP: return "Network Startup/Validation";
        case CID_RESPONSE: return "Response Command";
        case CID_REQUEST_INFO: return "Request Info";
        case CID_COMMUNICATION_ERROR: return "Communication Error";
        case CID_AUDIO_VIDEO_SOURCE_NAME: return "Audio/Video Source Name";
        case CID_AUDIO_SOURCE_TYPE: return "Audio Source Type and Capabilities";
        case CID_PLAY_STATUS: return "Play Status";
        case CID_ZONE_VOLUME: return "Zone Volume Absolute";
        case CID_ZONE_VOLUME_STEP: return "Zone Volume Step";
        case CID_MUTE_ZONE: return "Mute Zone";
        case CID_MUTE_CHANNELS: return "Mute Channels";
        case CID_ELAPSED_TRACK_TIME: return "Elapsed Track/Chapter Time";
        case CID_TRACK_TIME: return "Track/Chapter Time";
        case CID_REPEAT_SUPPORT: return "Repeat Support";
        case CID_SHUFFLE_SUPPORT: return "Shuffle Support";
        case CID_LIBRARY_DATA_TYPE: return "Library Data Type";
        case CID_LIBRARY_DATA_NAME: return "Library Data Name";
        case CID_ARTIST_NAME: return "Artist Name";
        case CID_ALBUM_NAME: return "Album Name";
        case CID_STATION_NAME: return "Station Name";
        case CID_NODE_ENABLE: return "Node Enable";
        case CID_TOTAL_ZONES: return "Total Number of Zones Available";
        case CID_ZONE_NAME: return "Zone Name";
        case CID_MAIN_SUB_SWITCHING: return "Main/Sub Switching";
        case CID_EQ_PRESET_NAME: return "EQ Preset Name";
        case CID_EQ_BASS: return "Equalizer Bass";
        case CID_EQ_TREBLE: return "Equalizer Treble";
        case CID_EQ_MID_RANGE: return "Equalizer Mid Range";
        case CID_BALANCE: return "Balance";
        case CID_FADE: return "Fade";
        case CID_SUB_VOLUME: return "Non-Fader, Sub Volume";
        case CID_SUBWOOFER_SWITCH: return "Subwoofer Direct Switching";
        case CID_CENTER_SWITCH: return "Center Direct Switching";
        case CID_TONE_BATCH: return "Tone Batch Direct Switching";
        case CID_BEEP_VOLUME: return "Beep Volume Direct Switching";
        case CID_SPEED_COMP: return "Speed Compensation";
        case CID_OVERHEAD_SWITCH: return "Overhead Direct Switching";
        case CID_ANC_ZONE_ENABLE: return "ANC Zone Enable";
        case CID_BEEP: return "Beep";
        case CID_VOICE_OUTPUT: return "Voice Output";
        case CID_BT_ADDR_AVAILABLE: return "Number of Bluetooth Addresses Available";
        case CID_BT_DEVICE_ADDR: return "Bluetooth Device Address";
        case CID_BT_DEVICE_STATUS: return "Bluetooth Device Status";
        case CID_BT_DEVICE_NAME: return "Bluetooth Device Name";
        case CID_BT_PAIRING_STATUS: return "Bluetooth Pairing Status";
        case CID_BT_FORGET_DEVICE: return "Forget Bluetooth Device";
        case CID_BT_DISCOVERING: return "Bluetooth Discovering";
        case CID_CHANNEL_SLOT: return "Channel Slot Assignment";
        case CID_PLL_LOCK_STATUS: return "PLL Lock Status";
        case CID_INPUT_VOLTAGE: return "Input Voltage";
        case CID_INPUT_CURRENT: return "Input Current";
        case CID_TEMPERATURE: return "Temperature Input Filter";
        case CID_SENSOR_GENERIC: return "Sensor Generic";
        case CID_MODULE_ENABLE: return "Module Enable";
        case CID_POWER_SUPPLY_ENABLE: return "Power Supply Enable";
        case CID_RCA_ENABLE: return "RCA Enable";
        case CID_SOFT_START: return "Soft Start";
        case CID_UNDERVOLTAGE_THRESHOLD: return "Undervoltage Threshold";
        case CID_OVERVOLTAGE_THRESHOLD: return "Overvoltage Threshold";
        case CID_BT_WIFI_RESERVED: return "Bluetooth/WiFi Reserved Command";
        case CID_MODULE_STATUS: return "Module Status";
        case CID_CHANNEL_CLIP: return "Channel Clip Detection";
        case CID_CHANNEL_SHORT: return "Channel Short Detection";
        case CID_CHANNEL_OPEN: return "Channel Open Detection";
        case CID_SHARC_STATUS: return "SHARC Status";
        default: return "Unknown/Custom Command";
    }
}

// Initialize the AABCOP protocol
void aabcop_init(void) {
    aabcop_state = STATE_INIT;
    message_index = 0;
    expected_frames = 0;
    received_frames = 0;
}

// Set the current state of the protocol
void aabcop_set_state(protocol_state_t new_state) {
    aabcop_state = new_state;
}

// Get the current state of the protocol
protocol_state_t aabcop_get_state(void) {
    return aabcop_state;
}

// Create A2B frames from an application packet and send via callback
void aabcop_create_frames(uint8_t uid, uint8_t cid, uint8_t* data, uint8_t dataLength, 
                         void (*write_byte_callback)(uint8_t byte, void* ctx), void* ctx) {
    uint8_t L = dataLength;

    if (L <= 2) {
        // Single frame
        uint8_t header = 0x7F & L;  // FRAME_TYPE=0, DATA_SIZE=L
        write_byte_callback(header, ctx);
        write_byte_callback(cid, ctx);
        write_byte_callback(L > 0 ? data[0] : 0x00, ctx);
        write_byte_callback(L > 1 ? data[1] : 0x00, ctx);
    } else {
        // Start frame
        uint8_t header = 0x7F & L;  // FRAME_TYPE=0, DATA_SIZE=L
        write_byte_callback(header, ctx);
        write_byte_callback(cid, ctx);
        write_byte_callback(data[0], ctx);
        write_byte_callback(data[1], ctx);

        // Calculate number of multi-frames needed
        uint8_t multiCount = (L - 2 + 2) / 3;  // Ceiling division for (L-2)/3
        uint8_t dataIndex = 2;

        // Create multi-frames
        for (uint8_t counter = 1; counter <= multiCount; counter++) {
            uint8_t header = 0x80 | counter;  // FRAME_TYPE=1, COUNTER=counter
            write_byte_callback(header, ctx);

            // Add up to 3 data bytes per multi-frame
            for (int i = 0; i < 3; i++) {
                if (dataIndex < L) {
                    write_byte_callback(data[dataIndex++], ctx);
                } else {
                    write_byte_callback(0x00, ctx);  // Padding
                }
            }
        }
    }
}

// Parse a mailbox frame
bool aabcop_parse_frame(uint8_t* mailbox, uint8_t* cid, uint8_t** data, uint8_t* dataIndex, uint8_t* dataSize) {
    // Check if this is a start frame or continuation frame
    uint8_t frameType = (mailbox[0] >> 7) & 0x01;
    
    if (frameType == FRAME_TYPE_START) {  // Start frame
        *dataSize = mailbox[0] & 0x7F;
        *cid = mailbox[1];
        *dataIndex = 0;
        
        // Reset message buffer
        message_index = 0;
        
        // Copy data from start frame
        if (*dataSize > 0 && message_index < sizeof(message_buffer)) {
            message_buffer[message_index++] = mailbox[2];
        }
        if (*dataSize > 1 && message_index < sizeof(message_buffer)) {
            message_buffer[message_index++] = mailbox[3];
        }
        
        // Calculate expected frames
        if (*dataSize <= 2) {
            expected_frames = 1;  // Only start frame
        } else {
            uint8_t remainingData = *dataSize - 2;  // Data after start frame
            expected_frames = 1 + (remainingData + 2) / 3;  // Start frame + multi frames
        }
        
        received_frames = 1;
        
        // Set data pointer to message buffer
        *data = message_buffer;
        
        // Return true if message is complete (single frame)
        return (expected_frames == 1);
    } else {  // Continuation frame
        uint8_t counter = mailbox[0] & 0x7F;
        
        // Copy data from continuation frame
        for (int i = 1; i < MAILBOX_SIZE; i++) {
            if (message_index < sizeof(message_buffer)) {
                message_buffer[message_index++] = mailbox[i];
            }
        }
        
        received_frames++;
        
        // Set data pointer to message buffer
        *data = message_buffer;
        
        // Return true if message is complete
        return (received_frames == expected_frames);
    }
}

// Process a mailbox frame and generate a response
bool aabcop_process_mailbox(uint8_t *mailbox, uint8_t *response) {
    uint8_t cid;
    uint8_t *data;
    uint8_t dataIndex = 0;
    uint8_t dataSize = 0;
    
    // Parse the incoming frame
    bool messageComplete = aabcop_parse_frame(mailbox, &cid, &data, &dataIndex, &dataSize);
    
    // If message is complete, process it
    if (messageComplete) {
        // Process the command based on CID
        switch (cid) {
            case CID_NETWORK_STARTUP:
                // Handle network startup
                // Create response with device UID
                response[0] = 0x01;  // DATA_SIZE=1
                response[1] = CID_RESPONSE;
                response[2] = cid;  // Echo the CID
                response[3] = RSP_STATUS_COMPLETION;  // Success
                return true;
                
            case CID_REQUEST_INFO:
                // Handle request info
                if (dataSize >= 1) {
                    uint8_t requestedCid = data[0];
                    // Create response with info about the requested CID
                    response[0] = 0x03;  // DATA_SIZE=3
                    response[1] = CID_RESPONSE;
                    response[2] = cid;  // Echo the CID
                    response[3] = RSP_STATUS_COMPLETION;  // Success
                    // Additional info would be sent in multi-frames
                    return true;
                }
                break;
                
            case CID_MODULE_ENABLE:
                // Handle module enable
                if (dataSize >= 1) {
                    // Implementation depends on specific module functionality
                    // For now, just acknowledge
                    response[0] = 0x02;  // DATA_SIZE=2
                    response[1] = CID_RESPONSE;
                    response[2] = cid;  // Echo the CID
                    response[3] = RSP_STATUS_COMPLETION;  // Success
                    return true;
                }
                break;
                
            // More cases can be added for other CIDs
                
            default:
                // Unknown or unsupported CID
                response[0] = 0x02;  // DATA_SIZE=2
                response[1] = CID_RESPONSE;
                response[2] = cid;  // Echo the CID
                response[3] = RSP_STATUS_NOT_SUPPORTED;  // Not supported
                return true;
        }
    }
    
    // Message is not complete or could not be processed
    return false;
}

// Print CID description with message type and device type
void aabcop_print_cid_description(uint8_t cid) {
    // Get CID name
    const char* cidName = aabcop_get_cid_name(cid);
    
    // Get device type
    uint8_t cidDevType = aabcop_get_cid_device_type(cid);
    
    // Print CID information
    printf("CID: 0x%02X - %s\n", cid, cidName);
    
    // Print message type header
    printf("Message Type:\n");
    
    // Special handling for MODULE_ENABLE (0x70)
    if (cid == CID_MODULE_ENABLE) {
        // Module Enable (0x70) is special case with different behavior for different device types
        printf("  ALL (except DSP/HU): R (Receiver Only)\n");
        printf("  DSP: S/R (Both Sender and Receiver)\n");
        printf("  HU: S/R (Both Sender and Receiver)\n");
    }
    // Special handling for MODULE_STATUS (0x80)
    else if (cid == CID_MODULE_STATUS) {
        // Module Status (0x80) is special case with different behavior for different device types
        printf("  ALL (except DSP/HU): S/R (Both Sender and Receiver)\n");
        printf("  DSP: R (Receiver Only)\n");
        printf("  HU: R (Receiver Only)\n");
    }
    // Special handling for SENSOR_GENERIC (0x69)
    else if (cid == CID_SENSOR_GENERIC) {
        // Sensor Generic (0x69) is special case with different behavior for different device types
        printf("  ALL (except DSP): R (Receiver Only)\n");
        printf("  DSP: S/R (Both Sender and Receiver)\n");
    }
    else {
        // Standard handling for other CIDs
        // Print sender/receiver status for DSP device type
        if (cidDevType & DEVICE_TYPE_DSP) {
            printf("  DSP: ");
            if (aabcop_is_sender_cid(cid, DEVICE_TYPE_DSP)) {
                if (aabcop_is_receiver_cid(cid, DEVICE_TYPE_DSP)) {
                    printf("S/R (Both Sender and Receiver)");
                } else {
                    printf("S (Sender Only)");
                }
            } else if (aabcop_is_receiver_cid(cid, DEVICE_TYPE_DSP)) {
                printf("R (Receiver Only)");
            } else {
                printf("N/A (Not applicable to DSP)");
            }
            printf("\n");
        }
        
        // Print sender/receiver status for HU device type
        if (cidDevType & DEVICE_TYPE_HU) {
            printf("  HU: ");
            if (aabcop_is_sender_cid(cid, DEVICE_TYPE_HU)) {
                if (aabcop_is_receiver_cid(cid, DEVICE_TYPE_HU)) {
                    printf("S/R (Both Sender and Receiver)");
                } else {
                    printf("S (Sender Only)");
                }
            } else if (aabcop_is_receiver_cid(cid, DEVICE_TYPE_HU)) {
                printf("R (Receiver Only)");
            } else {
                printf("N/A (Not applicable to HU)");
            }
            printf("\n");
        }
        
        // Print sender/receiver status for AMP device type
        if (cidDevType & DEVICE_TYPE_AMP) {
            printf("  AMP: ");
            if (aabcop_is_sender_cid(cid, DEVICE_TYPE_AMP)) {
                if (aabcop_is_receiver_cid(cid, DEVICE_TYPE_AMP)) {
                    printf("S/R (Both Sender and Receiver)");
                } else {
                    printf("S (Sender Only)");
                }
            } else if (aabcop_is_receiver_cid(cid, DEVICE_TYPE_AMP)) {
                printf("R (Receiver Only)");
            } else {
                printf("N/A (Not applicable to AMP)");
            }
            printf("\n");
        }
        
        // Print message type for ALL devices if DEVICE_TYPE_ALL is part of the device type
        // and this is not one of the special CIDs that need individual handling
        if (cidDevType & DEVICE_TYPE_ALL) {
            printf("  ALL: ");
            if (aabcop_is_sender_cid(cid, DEVICE_TYPE_ALL)) {
                if (aabcop_is_receiver_cid(cid, DEVICE_TYPE_ALL)) {
                    printf("S/R (Both Sender and Receiver)");
                } else {
                    printf("S (Sender Only)");
                }
            } else if (aabcop_is_receiver_cid(cid, DEVICE_TYPE_ALL)) {
                printf("R (Receiver Only)");
            } else {
                printf("N/A (Not applicable to ALL devices)");
            }
            printf("\n");
        }
    }
    
    // Print device type
    printf("Device Type: ");
    bool firstDevice = true;
    
    // Always print ALL if it's part of the device type
    if (cidDevType & DEVICE_TYPE_ALL) {
        printf("ALL");
        firstDevice = false;
    }
    
    // Also print individual device types regardless of whether ALL is present
    if (cidDevType & DEVICE_TYPE_DSP) {
        if (!firstDevice) printf("+");
        printf("DSP");
        firstDevice = false;
    }
    if (cidDevType & DEVICE_TYPE_HU) {
        if (!firstDevice) printf("+");
        printf("HU");
        firstDevice = false;
    }
    if (cidDevType & DEVICE_TYPE_AMP) {
        if (!firstDevice) printf("+");
        printf("AMP");
    }
    printf("\n");
}

// Print detailed information about a CID message
void aabcop_print_cid_info(uint8_t cid, uint8_t* data, uint8_t dataLen) {
    printf("Command ID (CID): 0x%02X - %s\n", cid, aabcop_get_cid_name(cid));
    
    // Print device types that can handle this CID
    uint8_t cidDevType = aabcop_get_cid_device_type(cid);
    printf("Applicable Device Types: ");
    bool firstDevice = true;
    
    if (cidDevType & DEVICE_TYPE_ALL) {
        printf("ALL");
        firstDevice = false;
    }
    if (cidDevType & DEVICE_TYPE_DSP) {
        if (!firstDevice) printf(", ");
        printf("DSP");
        firstDevice = false;
    }
    if (cidDevType & DEVICE_TYPE_HU) {
        if (!firstDevice) printf(", ");
        printf("HU");
        firstDevice = false;
    }
    if (cidDevType & DEVICE_TYPE_AMP) {
        if (!firstDevice) printf(", ");
        printf("AMP");
    }
    printf("\n");
    
    // Print S/R capabilities for each device type
    printf("Sender/Receiver Roles:\n");
    
    if (cidDevType & DEVICE_TYPE_DSP) {
        printf("  DSP: %s\n", 
               aabcop_is_sender_cid(cid, DEVICE_TYPE_DSP) ? 
                   (aabcop_is_receiver_cid(cid, DEVICE_TYPE_DSP) ? "S/R" : "S") : 
                   (aabcop_is_receiver_cid(cid, DEVICE_TYPE_DSP) ? "R" : "N/A"));
    }
    
    if (cidDevType & DEVICE_TYPE_HU) {
        printf("  HU: %s\n", 
               aabcop_is_sender_cid(cid, DEVICE_TYPE_HU) ? 
                   (aabcop_is_receiver_cid(cid, DEVICE_TYPE_HU) ? "S/R" : "S") : 
                   (aabcop_is_receiver_cid(cid, DEVICE_TYPE_HU) ? "R" : "N/A"));
    }
    
    if (cidDevType & DEVICE_TYPE_AMP) {
        printf("  AMP: %s\n", 
               aabcop_is_sender_cid(cid, DEVICE_TYPE_AMP) ? 
                   (aabcop_is_receiver_cid(cid, DEVICE_TYPE_AMP) ? "S/R" : "S") : 
                   (aabcop_is_receiver_cid(cid, DEVICE_TYPE_AMP) ? "R" : "N/A"));
    }
    
    if (dataLen > 0) {
        printf("Data Values: ");
        for (int i = 0; i < dataLen; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    }
    
    // Interpret the data based on CID
    printf("Data Interpretation:\n");
    switch (cid) {
        // Network generic CIDs
        case CID_NETWORK_STARTUP:
            printf("  Network Startup/Validation CID\n");
            printf("  Used at startup to verify the configured network\n");
            printf("  Every sub-node should respond with its UID\n");
            break;
            
        case CID_RESPONSE:
            if (dataLen >= 2) {
                printf("  Echo CID: 0x%02X, Status: %d", data[0], data[1]);
                if (data[1] == RSP_STATUS_COMPLETION) printf(" (Completion)");
                else if (data[1] == RSP_STATUS_NOT_SUPPORTED) printf(" (CID Not Supported)");
                else if (data[1] == RSP_STATUS_PARAM_ERROR) printf(" (Parameter Error)");
                else if (data[1] == RSP_STATUS_BUSY) printf(" (Busy)");
                else if (data[1] == RSP_STATUS_EXEC_FAILURE) printf(" (Execution Failure)");
                printf("\n");
            }
            break;
            
        case CID_COMMUNICATION_ERROR:
            if (dataLen >= 1) {
                printf("  Error type: %d", data[0]);
                if (data[0] == 0) printf(" (None)");
                else if (data[0] == 1) printf(" (Error at Sender)");
                else if (data[0] == 2) printf(" (Error at Receiver)");
                printf("\n");
            }
            break;
            
        case CID_ZONE_VOLUME:
            if (dataLen >= 2) {
                printf("  Zone: %d", data[0]);
                if (data[0] == 0) printf(" (All Zones)");
                else printf(" (Zone %d)", data[0]);
                
                printf(", Volume: %d%%\n", data[1]);
                printf("  Valid range is 0-100%%. Values >100%% are interpreted as 100%%\n");
            }
            break;
            
        case CID_MUTE_CHANNELS:
            if (dataLen >= 2) {
                uint16_t bitMap = (data[0] << 8) | data[1];
                printf("  Mute Bitmap: 0x%04X\n", bitMap);
                
                // Print which channels are muted
                printf("  Muted channels: ");
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
                printf("  Mute: Logic high (1), Unmute: Logic low (0)\n");
            }
            break;
            
        case CID_NODE_ENABLE:
            if (dataLen >= 1) {
                printf("  Status: %d - ", data[0]);
                switch(data[0]) {
                    case 0: printf("Standby\n"); break;
                    case 1: printf("Enabled\n"); break;
                    case 2: printf("Error\n"); break;
                    case 3: printf("Reset\n"); break;
                    default: printf("Unknown\n"); break;
                }
            }
            break;
            
        case CID_CHANNEL_SLOT:
            if (dataLen >= 1) {
                uint8_t channel = (data[0] >> 4) & 0x0F;
                uint8_t slot = data[0] & 0x0F;
                printf("  Channel: %d, Slot: %d\n", channel, slot);
                printf("  This CID assigns slot %d to channel %d\n", slot, channel);
            }
            break;
            
        case CID_MODULE_STATUS:
            if (dataLen >= 2) {
                uint8_t enable = (data[0] >> 7) & 0x01;
                uint8_t voltageLevel = (data[0] >> 2) & 0x1F;
                uint8_t ovp = (data[0] >> 1) & 0x01;
                uint8_t uvp = data[0] & 0x01;
                
                uint8_t ocp = (data[1] >> 7) & 0x01;
                uint8_t temperature = (data[1] >> 2) & 0x1F;
                uint8_t thermalFb = (data[1] >> 1) & 0x01;
                uint8_t thermalSd = data[1] & 0x01;
                
                printf("  Enable: %d (%s)\n", enable, enable ? "Enabled" : "Disabled");
                printf("  Voltage Level: %d (%.1f V)\n", voltageLevel, 6.0 + (voltageLevel * 0.5));
                printf("  OVP: %d (%s)\n", ovp, ovp ? "Active" : "Inactive");
                printf("  UVP: %d (%s)\n", uvp, uvp ? "Active" : "Inactive");
                printf("  OCP: %d (%s)\n", ocp, ocp ? "Active" : "Inactive");
                
                // Calculate actual temperature range from -40°C to 175°C
                int actualTemp = -40 + (temperature * 7);
                printf("  Temperature: %d (~%d°C)\n", temperature, actualTemp);
                
                printf("  Thermal Foldback: %d (%s)\n", thermalFb, thermalFb ? "Active" : "Inactive");
                printf("  Thermal Shutdown: %d (%s)\n", thermalSd, thermalSd ? "Active" : "Inactive");
                printf("  This CID reports device status including flags for Over Voltage, Under Voltage, Over Current,\n");
                printf("  Thermal Foldback, and Thermal Shutdown protection mechanisms\n");
            }
            break;
            
        case CID_CHANNEL_CLIP:
        case CID_CHANNEL_SHORT:
        case CID_CHANNEL_OPEN:
            if (dataLen >= 2) {
                uint16_t bitMap = (data[0] << 8) | data[1];
                const char* statusType = (cid == CID_CHANNEL_CLIP) ? "clipping" :
                                         (cid == CID_CHANNEL_SHORT) ? "short" : "open";
                printf("  Channel %s bitmap: 0x%04X\n", statusType, bitMap);
                
                printf("  Affected channels: ");
                int hasFlags = 0;
                for (int i = 0; i < 16; i++) {
                    if (bitMap & (1 << i)) {
                        printf("%d ", i + 1);
                        hasFlags = 1;
                    }
                }
                if (!hasFlags) {
                    printf("None");
                }
                printf("\n");
                printf("  Each bit corresponds to a channel and shows logic high (1) for %s condition\n", statusType);
            }
            break;
            
        // Add more cases for other CIDs as needed
            
        default:
            // For CIDs that aren't specifically handled, display some general info
            printf("  Refer to AAB Specification v1.3 for details on this CID\n");
            printf("  Raw data shown above (if available)\n");
            break;
    }
}
