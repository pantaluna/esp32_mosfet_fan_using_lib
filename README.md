## Project Description esp32_mosfet_fan_using_lib
This project demonstrates how to use a N Channel MOSFET to power on/off a peripheral. The MOSFET, controlled by the ESP32, is used to power-down power hungry devices when they are not needed. For example: a GPS device, a 5V fan, a 12V fan.

This example uses a MOSFET to turn **a 5V Pi-FAN fan (on the +5V power rail)** and **a standard LED (on the +3.3V power rail)** on and off using a GPIO pin of the ESP32 microcontroller.



## What are the HW SW requirements of the ESP32 MJD Starter Kit?

### Hardware

- ESP development board. I suggest to buy a popular development board with good technical documentation and a significant user base. Examples: [LOLIN D32](https://wiki.wemos.cc/products:d32:d32), [Adafruit HUZZAH32](https://www.adafruit.com/product/3405), [Espressif ESP32-DevKitC](http://espressif.com/en/products/hardware/esp32-devkitc/overview), [Pycom WiPy](https://pycom.io/hardware/).
- N Channel Power MOSFET that is 3.3V compatible. Good ones:
  - IRF3708 N-Channel Power MOSFET in package THT TO-220.
  - IRLML6244PBF N Channel MOSFET in package SMD SOT23-3 (can be soldered easily on a SOT23-3 breakout board).
- A 5V 200mA PI-FAN fan.
- A standard LED (red 20mA).



Note: you can omit one of the 2 peripherals (LED / fan) to make it simpler.



### Software: ESP-IDF v3.2

- A working installation of the **Espressif ESP-IDF V3.2 development framework** (detailed instructions @ http://esp-idf.readthedocs.io/en/latest/get-started/index.html).

```
mkdir ~/esp
cd    ~/esp
git clone -b v3.2 --recursive https://github.com/espressif/esp-idf.git esp-idf-v3.2
```

- A C language editor or the Eclipse IDE CDT (instructions also @ http://esp-idf.readthedocs.io/en/latest/get-started/index.html).



## Lab Setup / Wiring Diagram

![Wiring Diagram - xxxxxxxx.png](.\_doc\Wiring Diagram - Project esp32_mosfet_fan_using_lib-01.png)



```
PIN LAYOUT & WIRING

FAN            => ESP32 Development Board
--------------    -----------------------
1   Positive      5V/VUSB pin

LED            => 1K resistor => ESP32 Development Board
--------------    -----------    -------------
1   Positive      passthrough    3.3V pin (not the 5V or VUSB pin!)

MOSFET            Destination
----------------  -----------------------
1   GATE          ESP32 dev board: GPIO#4 (Adafruit Huzzah32 GPIO#4 = bottomleft-6)
2   DRAIN         LED: GND
2   DRAIN         FAN: GND
3	SOURCE (GND)  ESP32 dev board: GND pin
```



## Running the example
- Run `make flash monitor` to build and upload the example to your board and connect to its serial terminal.



## The output debug log

```
I (0) cpu_start: App cpu up.
...
I (397) mjd: *** DATETIME 19700101002041 Thu Jan  1 00:20:41 1970
I (417) gpio: GPIO[13]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (427) myapp: Init POWER MOSFET...
I (427) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (437) myapp: Wait 5 seconds before turning the Gate ON
I (5447) myapp: POWER MOSFET Gate := *ON (the FAN and the LED should turn on)...
I (5447) myapp: Wait 15 seconds whilst the gate is ON..
I (20447) myapp: POWER MOSFET Gate := *OFF (the FAN and the LED should turn off)...
I (20447) myapp: ***SECTION: DEEP SLEEP***
I (20447) myapp: Entering deep sleep (the MCU should wake up 15 seconds later)...

```



## Notes
- Change the logging level of each ESP32 project using `make menuconfig` from "INFO" to "DEBUG" if you want to get more details about the requests and responses that are exchanged using the UART data channel.



## Reference: the ESP32 MJD Starter Kit SDK

Do you also want to create innovative IoT projects that use the ESP32 chip, or ESP32-based modules, of the popular company Espressif? Well, I did and still do. And I hope you do too.

The objective of this well documented Starter Kit is to accelerate the development of your IoT projects for ESP32 hardware using the ESP-IDF framework from Espressif and get inspired what kind of apps you can build for ESP32 using various hardware modules.

Go to https://github.com/pantaluna/esp32-mjd-starter-kit

