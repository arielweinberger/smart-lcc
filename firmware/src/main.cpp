#include "mbed.h"
#include "SystemStatus.h"
#include "ControlBoardCommunication/ControlBoardTransceiver.h"
#include <USB/PluggableUSBSerial.h>
#include <UI/UIController.h>
#include <SystemController/SystemController.h>
#include "ssd1309/Adafruit_SSD1306.h"

REDIRECT_STDOUT_TO(SerialUSB);

using namespace std::chrono_literals;
mbed::DigitalOut led(LED1);

SystemStatus* systemStatus = new SystemStatus;

rtos::Thread controlBoardCommunicationThread;
ControlBoardTransceiver trx(SERIAL1_TX, SERIAL1_RX, systemStatus);

void mbed_error_hook(const mbed_error_ctx *error_context) {
    led = true;
}

class SPIPreInit : public mbed::SPI
{
public:
    SPIPreInit(PinName mosi, PinName miso, PinName clk) : SPI(mosi,miso,clk)
    {
        format(8,3);
        frequency(2000000);
    };
};

SPIPreInit gSpi(
        digitalPinToPinName(PIN_SPI_MOSI),
        digitalPinToPinName(PIN_SPI_MISO),
        digitalPinToPinName(PIN_SPI_SCK)
        );

Adafruit_SSD1306_Spi gOled1(
        gSpi,
        digitalPinToPinName(19),
        digitalPinToPinName(18),
        digitalPinToPinName(PIN_SPI_SS),
        64
        );

rtos::Thread uiThread;
UIController uiController(systemStatus, &gOled1);

rtos::Thread systemControllerThread;
SystemController systemController(systemStatus);

int main()
{
#ifdef SERIAL_DEBUG
#if defined(SERIAL_CDC)
    PluggableUSBD().begin();
    _SerialUSB.begin(9600);
#endif
#endif
    uiThread.start([] { uiController.run(); });

#ifdef SERIAL_DEBUG
    rtos::ThisThread::sleep_for(10000ms);
#endif

    controlBoardCommunicationThread.start([] { trx.run(); });
    systemControllerThread.start([] { systemController.run(); });

    while(true) {
#ifdef SERIAL_DEBUG
        printf("Main loop\n");
#endif
        rtos::ThisThread::sleep_for(1000ms);
    }
}