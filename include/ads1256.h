#include <enums.h>


class ADS1256 {
    public:
        ADS1256(Gain gain=Gain::ADS1256_GAIN_1, DataRate datarate = DataRate::ADS1256_30000SPS);
        void begin();
    
    private:
        Gain gain;
        DataRate datarate;
};