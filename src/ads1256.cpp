#include <ads1256.h>


ADS1256::ADS1256(Gain gain, DataRate datarate){
    this->gain = gain;
    this->datarate = datarate;
}

void  ADS1256::begin(){
    
}