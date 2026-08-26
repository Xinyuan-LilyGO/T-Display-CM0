// make sure this is always compiled
#include "TypeDef.h"

#if !defined(_RADIOLIB_CM0HAL_H)
#define _RADIOLIB_CM0HAL_H

#include "Hal.h"

/*!
  \class ArduinoHal
  \brief Arduino default hardware abstraction library implementation.
  This class can be extended to support other Arduino platform or change behaviour of the default implementation.
*/
class CM0Hal:public RadioLibHal {
  public:
    /*!
      \brief Arduino Hal constructor. Will use the default SPI interface and automatically initialize it.
    */
    CM0Hal();

    /*!
      \brief Arduino Hal constructor. Will not attempt SPI interface initialization.
      \param spi SPI interface to be used, can also use software SPI implementations.
      \param spiSettings SPI interface settings.
    */
    //explicit ArduinoHal(SPIClass& spi, SPISettings spiSettings = RADIOLIB_DEFAULT_SPI_SETTINGS);

    // implementations of pure virtual RadioLibHal methods
    void pinMode(uint32_t pin, uint32_t mode) override;
    void digitalWrite(uint32_t pin, uint32_t value) override;
    uint32_t digitalRead(uint32_t pin) override;
    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override;
    void detachInterrupt(uint32_t interruptNum) override;
    void delay(RadioLibTime_t ms) override;
    void delayMicroseconds(RadioLibTime_t us) override;
    RadioLibTime_t millis() override;
    RadioLibTime_t micros() override;
    long pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) override;
    void spiBegin() override;
    void spiBeginTransaction() override;
    void spiTransfer(uint8_t* out, size_t len, uint8_t* in) override;
    void spiEndTransaction() override;
    void spiEnd() override;
    // implementations of virtual RadioLibHal methods
    void init() override;
    void term() override;
    void tone(uint32_t pin, unsigned int frequency, RadioLibTime_t duration = 0) override;
    void noTone(uint32_t pin) override;
    void yield() override;
    uint32_t pinToInterrupt(uint32_t pin) override;

#if !RADIOLIB_GODMODE
  protected:
#endif
    //SPIClass* spi = NULL;
    //SPISettings spiSettings = RADIOLIB_DEFAULT_SPI_SETTINGS;
    //bool initInterface = false;
    int gpio_fd;
    int fd_soft_spi;
    int fd_hw_spi;
   
    
    
    
    
    
    
    
    
    
    
    
};

#endif
