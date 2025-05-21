#ifndef AABCOP_H
#define AABCOP_H

#include <stdint.h>
#include <stdbool.h>

// Constants for AABCOP protocol
#define UID_DANG8100           0x60
#define MAILBOX_SIZE           4

// Command IDs (CIDs)
#define CID_NETWORK_STARTUP    0x00
#define CID_RESPONSE           0x01
#define CID_ZONE_VOLUME        0x13
#define CID_MUTE_ZONE          0x15
#define CID_MUTE_CHANNELS      0x16
#define CID_EQ_BASS            0x32
#define CID_EQ_TREBLE          0x33
#define CID_CHANNEL_SLOT       0x60
#define CID_PLL_LOCK_STATUS    0x61
#define CID_MODULE_ENABLE      0x70
#define CID_MODULE_STATUS      0x80

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