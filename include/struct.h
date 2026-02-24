#include <stdint.h>

struct __attribute__((__packed__)) ADC_Packet {
    uint8_t header;    // 0xAA
    uint8_t channel;   // 0x00 - 0x07
    uint8_t  data[3];   // 24-bit ADC value
    uint8_t checksum;  // XOR of above
};
