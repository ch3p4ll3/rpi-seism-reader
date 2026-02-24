enum ScanMode {
    SINGLE_ENDED_INPUT = 0,
    DIFFERENTIAL_INPUT = 1
};

enum Gain {
    ADS1256_GAIN_1 = 0,
    ADS1256_GAIN_2 = 1,
    ADS1256_GAIN_4 = 2,
    ADS1256_GAIN_8 = 3,
    ADS1256_GAIN_16 = 4,
    ADS1256_GAIN_32 = 5,
    ADS1256_GAIN_64 = 6,
};

enum DataRate {
    ADS1256_30000SPS = 0xF0,
    ADS1256_15000SPS = 0xE0,
    ADS1256_7500SPS = 0xD0,
    ADS1256_3750SPS = 0xC0,
    ADS1256_2000SPS = 0xB0,
    ADS1256_1000SPS = 0xA1,
    ADS1256_500SPS = 0x92,
    ADS1256_100SPS = 0x82,
    ADS1256_60SPS = 0x72,
    ADS1256_50SPS = 0x63,
    ADS1256_30SPS = 0x53,
    ADS1256_25SPS = 0x43,
    ADS1256_15SPS = 0x33,
    ADS1256_10SPS = 0x20,
    ADS1256_5SPS = 0x13,
    ADS1256_2D5SPS = 0x03,
};

enum Registry {
    STATUS = 0,
    MUX = 1,
    ADCON = 2,
    DRATE = 3,
    IO = 4,
    OFC0 = 5,
    OFC1 = 6,
    OFC2 = 7,
    FSC0 = 8,
    FSC1 = 9,
    FSC2 = 10,
};

enum Commands {
    WAKEUP = 0x00,
    RDATA = 0x01,
    RDATAC = 0x03,
    SDATAC = 0x0F,
    RREG = 0x10,
    WREG = 0x50,
    SELFCAL = 0xF0,
    SELFOCAL = 0xF1,
    SELFGCAL = 0xF2,
    SYSOCAL = 0xF3,
    SYSGCAL = 0xF4,
    SYNC = 0xFC,
    STANDBY = 0xFD,
    RESET = 0xFE,
};