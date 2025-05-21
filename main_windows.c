#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <windows.h>

// Constants for AABCOP protocol
#define UID_DANG8100           0x60
#define MAILBOX_SIZE           4

// Command IDs (CIDs) based on protocol
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

// System state structure
typedef struct {
    protocol_state_t state;
    uint8_t volume;
    bool muted;
    int8_t bass;
    int8_t treble;
    bool module_enabled;
    uint8_t mailbox[MAILBOX_SIZE];
    uint8_t response[MAILBOX_SIZE];
} system_state_t;

// Global state
system_state_t g_state;

// Simulate SPI communication with A2B transceiver
bool spi_transfer(uint8_t *tx_data, uint8_t *rx_data, size_t length) {
    printf("[SPI] TX: ");
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", tx_data[i]);
    }
    printf("\n");
    
    // Simulate response (just echo back for now with minor modification)
    if (rx_data != NULL) {
        memcpy(rx_data, tx_data, length);
        rx_data[0] ^= 0x80; // Modify first byte to simulate response
    }
    
    return true;
}

// Process mailbox message
void process_mailbox(uint8_t *mailbox) {
    uint8_t target_uid = mailbox[0];
    uint8_t cid = mailbox[1];
    uint8_t param1 = mailbox[2];
    uint8_t param2 = mailbox[3];
    
    // Check if message is for us
    if (target_uid != UID_DANG8100 && target_uid != 0xFF) {
        printf("[AABCOP] Message not for us (UID: 0x%02X)\n", target_uid);
        return;
    }
    
    printf("[AABCOP] Processing CID: 0x%02X, Params: 0x%02X, 0x%02X\n", 
           cid, param1, param2);
    
    // Prepare response
    g_state.response[0] = UID_DANG8100;  // Our UID
    g_state.response[1] = CID_RESPONSE;  // Response command
    g_state.response[2] = cid;           // Original command
    g_state.response[3] = 0x00;          // Status (OK)
    
    // Process based on Command ID
    switch (cid) {
        case CID_NETWORK_STARTUP:
            printf("[AABCOP] Network startup/validation\n");
            g_state.state = STATE_DISCOVERY;
            break;
            
        case CID_ZONE_VOLUME:
            printf("[AABCOP] Setting volume to %d\n", param1);
            g_state.volume = param1;
            break;
            
        case CID_MUTE_ZONE:
            printf("[AABCOP] Setting mute state to %s\n", param1 ? "ON" : "OFF");
            g_state.muted = (param1 != 0);
            break;
            
        case CID_EQ_BASS:
            printf("[AABCOP] Setting bass to %d\n", (int8_t)param1);
            g_state.bass = (int8_t)param1;
            break;
            
        case CID_EQ_TREBLE:
            printf("[AABCOP] Setting treble to %d\n", (int8_t)param1);
            g_state.treble = (int8_t)param1;
            break;
            
        case CID_MODULE_ENABLE:
            printf("[AABCOP] Setting module state to %s\n", param1 ? "ENABLED" : "DISABLED");
            g_state.module_enabled = (param1 != 0);
            break;
            
        case CID_MODULE_STATUS:
            printf("[AABCOP] Reporting module status\n");
            g_state.response[3] = g_state.module_enabled ? 0x01 : 0x00;
            break;
            
        default:
            printf("[AABCOP] Unsupported command: 0x%02X\n", cid);
            g_state.response[3] = 0xFF;  // Error
            break;
    }
    
    // Send response
    printf("[AABCOP] Response: %02X %02X %02X %02X\n", 
           g_state.response[0], g_state.response[1], 
           g_state.response[2], g_state.response[3]);
}

// Simulate ADC readings (voltage, current, temperature)
void update_adc_readings() {
    static uint32_t voltage_mv = 12000; // 12V
    static uint32_t current_ma = 500;   // 500mA
    static int32_t temp_c = 35;         // 35°C
    
    // Add some variation
    voltage_mv += rand() % 200 - 100;  // ±100mV
    current_ma += rand() % 20 - 10;    // ±10mA
    temp_c += rand() % 3 - 1;          // ±1°C
    
    // Keep in realistic ranges
    voltage_mv = max(10000, min(14000, voltage_mv)); // 10-14V
    current_ma = max(100, min(1000, current_ma));    // 100-1000mA
    temp_c = max(20, min(60, temp_c));               // 20-60°C
    
    printf("[ADC] Voltage: %.2fV, Current: %dmA, Temperature: %d°C\n", 
           voltage_mv / 1000.0, current_ma, temp_c);
}

// Display current state
void display_state() {
    printf("\n----- DANG8100 Emulator Status -----\n");
    printf("Protocol State: ");
    switch (g_state.state) {
        case STATE_INIT:      printf("INITIALIZATION\n"); break;
        case STATE_DISCOVERY: printf("DISCOVERY\n"); break;
        case STATE_RUNNING:   printf("RUNNING\n"); break;
        case STATE_ERROR:     printf("ERROR\n"); break;
    }
    printf("Volume: %d%s\n", g_state.volume, g_state.muted ? " (MUTED)" : "");
    printf("EQ Settings: Bass=%d, Treble=%d\n", g_state.bass, g_state.treble);
    printf("Module: %s\n", g_state.module_enabled ? "ENABLED" : "DISABLED");
    printf("-----------------------------------\n\n");
}

// Process user commands for simulation control
void process_user_command(const char *cmd) {
    if (strcmp(cmd, "status") == 0) {
        display_state();
    }
    else if (strncmp(cmd, "volume ", 7) == 0) {
        int vol = atoi(cmd + 7);
        if (vol >= 0 && vol <= 100) {
            g_state.volume = (uint8_t)vol;
            printf("Volume set to %d\n", vol);
        } else {
            printf("Invalid volume value (0-100)\n");
        }
    }
    else if (strcmp(cmd, "mute") == 0) {
        g_state.muted = true;
        printf("Audio muted\n");
    }
    else if (strcmp(cmd, "unmute") == 0) {
        g_state.muted = false;
        printf("Audio unmuted\n");
    }
    else if (strncmp(cmd, "bass ", 5) == 0) {
        int bass = atoi(cmd + 5);
        if (bass >= -10 && bass <= 10) {
            g_state.bass = (int8_t)bass;
            printf("Bass set to %d\n", bass);
        } else {
            printf("Invalid bass value (-10 to 10)\n");
        }
    }
    else if (strncmp(cmd, "treble ", 7) == 0) {
        int treble = atoi(cmd + 7);
        if (treble >= -10 && treble <= 10) {
            g_state.treble = (int8_t)treble;
            printf("Treble set to %d\n", treble);
        } else {
            printf("Invalid treble value (-10 to 10)\n");
        }
    }
    else if (strcmp(cmd, "enable") == 0) {
        g_state.module_enabled = true;
        printf("Module enabled\n");
    }
    else if (strcmp(cmd, "disable") == 0) {
        g_state.module_enabled = false;
        printf("Module disabled\n");
    }
    else if (strncmp(cmd, "send ", 5) == 0) {
        // Parse hex values from command
        uint8_t mailbox[4] = {0};
        const char *hex = cmd + 5;
        char hex_byte[3] = {0};
        
        for (int i = 0; i < 4 && *hex; i++) {
            // Skip spaces
            while (*hex == ' ') hex++;
            if (!*hex) break;
            
            // Extract two hex characters
            hex_byte[0] = hex[0];
            hex_byte[1] = hex[1] ? hex[1] : '0';
            hex_byte[2] = '\0';
            
            // Convert to byte
            mailbox[i] = (uint8_t)strtol(hex_byte, NULL, 16);
            
            // Move to next position
            hex += hex_byte[1] ? 2 : 1;
        }
        
        printf("Sending mailbox: %02X %02X %02X %02X\n", 
               mailbox[0], mailbox[1], mailbox[2], mailbox[3]);
        process_mailbox(mailbox);
    }
    else if (strcmp(cmd, "help") == 0) {
        printf("Available commands:\n");
        printf("  status           - Display current state\n");
        printf("  volume <0-100>   - Set volume level\n");
        printf("  mute             - Mute audio\n");
        printf("  unmute           - Unmute audio\n");
        printf("  bass <-10 to 10> - Set bass level\n");
        printf("  treble <-10 to 10> - Set treble level\n");
        printf("  enable           - Enable module\n");
        printf("  disable          - Disable module\n");
        printf("  send <hex bytes> - Send mailbox message (4 bytes in hex)\n");
        printf("  quit             - Exit the program\n");
    }
    else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
        printf("Exiting...\n");
        exit(0);
    }
    else {
        printf("Unknown command. Type 'help' for available commands.\n");
    }
}

int main() {
    // Initialize random seed
    srand((unsigned int)time(NULL));
    
    // Initialize system state
    g_state.state = STATE_INIT;
    g_state.volume = 50;
    g_state.muted = false;
    g_state.bass = 0;
    g_state.treble = 0;
    g_state.module_enabled = false;
    memset(g_state.mailbox, 0, MAILBOX_SIZE);
    memset(g_state.response, 0, MAILBOX_SIZE);
    
    printf("=== DANG8100 A2B Secondary Node Emulator ===\n");
    printf("Type 'help' for available commands\n\n");
    
    // Main loop
    char cmd[256];
    
    // After initialization, transition to discovery state
    printf("Initializing emulator...\n");
    Sleep(1000);  // Simulate initialization time
    g_state.state = STATE_DISCOVERY;
    
    // Simulate discovery
    printf("Performing network discovery...\n");
    Sleep(1000);  // Simulate discovery time
    g_state.state = STATE_RUNNING;
    
    printf("Emulator running. Enter commands below:\n");
    
    while (1) {
        // Simulate periodic ADC readings
        static DWORD last_adc_update = 0;
        DWORD current_time = GetTickCount();
        
        if (current_time - last_adc_update > 5000) {  // Every 5 seconds
            update_adc_readings();
            last_adc_update = current_time;
        }
        
        // Process user input (non-blocking)
        printf("> ");
        if (fgets(cmd, sizeof(cmd), stdin)) {
            // Remove newline
            size_t len = strlen(cmd);
            if (len > 0 && cmd[len-1] == '\n') {
                cmd[len-1] = '\0';
            }
            
            if (strlen(cmd) > 0) {
                process_user_command(cmd);
            }
        }
    }
    
    return 0;
}