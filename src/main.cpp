#include <Arduino.h>

#include <ADS1256.h>

#include <struct.h>


//Platform-specific pin definitions
#if defined(ARDUINO_ARCH_RP2040)
#pragma message "Using RP2040"
//#define USE_SPI1 //Alternative USE_SPI for RP2040 - Uncomment to use SPI1
#if defined(USE_SPI1)
#pragma message "Using SPI1 (SPI1)"
#define SPI_MOSI 11
#define SPI_MISO 12
#define SPI_SCK 10
#define USE_SPI SPI1
#else
#pragma message "Using SPI (SPI0)"
#define SPI_MOSI 3  //19
#define SPI_MISO 4  //16
#define SPI_SCK 2   //18
#define USE_SPI SPI
#endif
//-----------------------------------------

#elif defined(ARDUINO_ARCH_STM32)
#pragma message "Using STM32"
//#define USE_SPI2  //Uncomment to use SPI2
#if defined(USE_SPI2)
#pragma message "Using SPI2"
#define USE_SPI spi2
SPIClass spi2(PB15, PB14, PB13);  //MOSI, MISO, SCK
#else
#pragma message "Using SPI (SPI1)"
#define USE_SPI SPI  //Default SPI1, pre-instantiated as 'SPI' on PA7, PA6, PA5
#endif
                                  //-----------------------------------------
#elif defined(TEENSYDUINO)
#pragma message "Using Teensy"
//#define USE_SPI1 //Uncomment to use SPI1 on Teensy 4.0 or 4.1
//#define USE_SPI2 //Uncomment to use SPI2 on Teensy 4.0 or 4.1
#if defined(USE_SPI2)
#pragma message "Using SPI2 (SPI3)"
#define USE_SPI SPI2
#elif defined(USE_SPI1)
#pragma message "Using SPI1 (SPI2)"
#define USE_SPI SPI1
#else
#pragma message "Using SPI (SPI1)"
#define USE_SPI SPI
#endif
//-----------------------------------------

#elif defined(ARDUINO_ARCH_ESP32)
#pragma message "Using ESP32"
SPIClass hspi(HSPI);
//#define USE_HSPI  // Uncomment to use HSPI instead of VSPI
#if defined(USE_HSPI)
#pragma message "Using HSPI"
#define USE_SPI hspi
#else
#pragma message "Using VSPI"
#define USE_SPI SPI
#endif
//-----------------------------------------
#else  //Default fallback (Arduino AVR)
#define SPI_MOSI MOSI
#define SPI_MISO MISO
#define SPI_SCK SCK
#define USE_SPI SPI
//-----------------------------------------
#endif

#define SAMPLING_SPEED 100  // in Hz

unsigned long lastSampleTime = 0;
const unsigned long interval = 1000 / SAMPLING_SPEED; // 1000ms / 100Hz = 10ms


//Below a few examples of pin descriptions for different microcontrollers I used:
ADS1256 A(2, ADS1256::PIN_UNUSED, 8, 10, 2.500, &USE_SPI); //DRDY, RESET, SYNC(PDWN), CS, VREF(float).    //Arduino Nano/Uno - OK
//ADS1256 A(7, ADS1256::PIN_UNUSED, 10, 9, 2.500, &USE_SPI); //DRDY, RESET, SYNC(PDWN), CS, VREF(float).      //ATmega32U4 -OK
//ADS1256 A(16, 17, ADS1256::PIN_UNUSED, 15, 2.500, &USE_SPI); //DRDY, RESET, SYNC(PDWN), CS, VREF(float).   //ESP32 WROOM 32 - OK (HSPI+VSPI)
//ADS1256 A(7, ADS1256::PIN_UNUSED, 8, 10, 2.500, &USE_SPI); //DRDY, RESET, SYNC(PDWN), CS, VREF(float).    //Teensy 4.0 - OK
//ADS1256 A(7, ADS1256::PIN_UNUSED, 6, 5, 2.500, &USE_SPI); //DRDY, RESET, SYNC(PDWN), CS, VREF(float).    //RP2040 Waveshare Mini - OK
//ADS1256 A(18, 20, 21, 19, 2.500, &USE_SPI); //DRDY, RESET, SYNC(PDWN), CS, VREF(float), SPI bus.  //RP2040 Zero - OK
//ADS1256 A(15, ADS1256::PIN_UNUSED, 14, 17, 2.500, &USE_SPI);  //DRDY, RESET, SYNC(PDWN), CS, VREF(float), SPI bus.  //RP2040 Pico W - OK
//ADS1256 A(PA2, ADS1256::PIN_UNUSED, ADS1256::PIN_UNUSED, PA4, 2.500, &USE_SPI); //DRDY, RESET, SYNC(PDWN), CS, VREF(float). //STM32 "blue pill" - SPI1 - OK
//ADS1256 A(PB10, PB11, ADS1256::PIN_UNUSED, PB12, 2.500, &USE_SPI);  // DRDY, RESET, SYNC, CS, VREF, SPI //STM32 "blue pill" - SPI2 - OK

void setup() {
  Serial.begin(115200);  //The value does not matter if you use an MCU with native USB

  while (!Serial) {
    ;  //Wait until the serial becomes available
  }

#if defined(ARDUINO_ARCH_RP2040)  //If RP2040 is used, we need to pass the SPI pins
  SPI.setSCK(SPI_SCK);
  SPI.setTX(SPI_MOSI);
  SPI.setRX(SPI_MISO);
#endif

#if defined(USE_HSPI)      //If ESP32 is used, we need to start SPI with a non-strapping MISO pin
  hspi.begin(14, 25, 13);  //SCK, MISO (safe), MOSI
#endif

  A.InitializeADC();  //See the documentation for every details

  A.setPGA(PGA_64);
  //Set input channels
  A.setMUX(DIFF_6_7);
  //Set DRATE
  A.setDRATE(DRATE_2000SPS);
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastSampleTime >= interval) {
    long data[4];
  
    for (size_t i = 0; i < 3; i++)
    {
      data[i] = A.cycleDifferential();
    }
  
    for (size_t i = 0; i < 3; i++)
    {
      ADC_Packet myPacket;
    
      myPacket.header = 0xAA;
      myPacket.channel = i; // You'll need to track which mux channel is active
  
      long val = data[i];
    
      // Fill the struct using the raw buffer from your library
      myPacket.data[0] = (val >> 16) & 0xFF; // MSB
      myPacket.data[1] = (val >> 8) & 0xFF;  // Mid
      myPacket.data[2] = val & 0xFF;         // LSB
    
      // Calculate Checksum
      myPacket.checksum = myPacket.header ^ myPacket.channel ^ 
                        myPacket.data[0] ^ myPacket.data[1] ^ myPacket.data[2];
    
      Serial.write((uint8_t*)&myPacket, sizeof(myPacket));  
    }
  }
}