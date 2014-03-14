QP-nano framework on ATmega32A
==============================

Introduction
------------

Some tests/examples of the [QP-framework](http://www.state-machine.com/qp/qpn/)

- learn the to use the framework and RTOS concepts 
- investigate performance/limitations while using low-end hardware
- determine usefulness of UML Statecharts modeling

Dependencies
------------

gcc-avr, avr-libc, avrdude, cmake

Build instructions
------------------

1. Clone the repo:

		git clone https://github.com/mryndzionek/qpn_experiments.git

2. Edit the gcc-avr options in `qpn_experiments/toolchain-avr-gcc.make` file

3. Download the QP-nano framework (4.5.02a) from [here](http://sourceforge.net/projects/qpc/files/QP-nano/4.5.02a/) and unpack it into `qpn` subdirectory

4. Download the QP-nano framework (5.2.0) from [here](http://sourceforge.net/projects/qpc/files/QP-nano/5.2.0/) and unpack it into `qpn_5.2.0` subdirectory

5. Download the [QM](http://sourceforge.net/projects/qpc/files/QM/3.0.1/) Graphical Modeling Tool (optional - to open the `*.qm` files)

6. Configure the project:

		cd qpn_experiments
		mkdir build
		cd build
		cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-avr-gcc.make -DCMAKE_BUILD_TYPE=MinSizeRel ..

7. Build the project:

		make

8. Upload the simple `blink` example hex file to the MCU:

		make upload_blink

Hardware setup
--------------

ATmega32A on a breadboard. 16MHz crystal. Fuses set to: `-U lfuse:w:0xff:m -U hfuse:w:0xd9:m`.
Serial LED resistors - 120Ohm. 16x2 LCD - HD44780 compatible - 10kOhm contrast pot.
AREF connected to GND via 100nF cap.
Detailed function of each peripheral is different for each example.
![setup](images/setup.png?raw=true "Breadboard setup")

Implemented examples
--------------------

* simple LED blink example - to test the setup and gcc compilation options
![blink](images/blink.png?raw=true "simple LED blink")

* simple 'safety critical' system - **PE**destrian **LI**ght **CON**trolled crossing (more info [here](http://www.state-machine.com/resources/AN_PELICAN.pdf))
![pelican](images/pelican.png?raw=true "Pelican crossing example")

| Pin  | Peripheral|
| ------------- | -------------|
| PD0  | red LED |
| PD1  | yellow LED |
| PD4  | green LED |
| PD5  | idle LED (UV) |
| PD7  | 5V buzzer - using NPN transistor |
| PC0  | LCD RS |
| PC1  | LCD EN |
| PC2  | LCD D4 |
| PC3  | LCD D5 |
| PC4  | LCD D6 |
| PC5  | LCD D7 |
| PD2  | On/Off button |
| PD3  | 'Pedestrians waiting' button |

* Capstone Dive Computer (more info [here](http://www.state-machine.com/resources/AN_Capstone.pdf) and [here](http://www.barrgroup.com/Dive-Computer))
![capstone](images/capstone.png?raw=true "Capstone dive computer example")
![capstone_alarm](images/alarm.png?raw=true "Capstone alarm SM")
![capstone_lcd](images/capstone_lcd.png?raw=true "Capstone LCD view")

| Pin  | Peripheral|
| ------------- | -------------|
| PD0  | ADC conversion LED (ascent rate) |
| PD1  | 'surfaced' LED |
| PD4  | heartbeat LED |
| PD5  | idle LED (UV) |
| PD7  | 5V alarm buzzer - using NPN transistor |
| PC0  | LCD RS |
| PC1  | LCD EN |
| PC2  | LCD D4 |
| PC3  | LCD D5 |
| PC4  | LCD D6 |
| PC5  | LCD D7 |
| PD2  | button one |
| PD3  | button two |
| PA0  | ADC ascent rate control |

Notes and Things to Investigate
-------------------------------

* QM graphical modeling tool does quite a nice job of alleviating the burden of SM code creation and maintenance 
* Use the latest QP-nano release to test the new QMsm class

Contact
-------
If you have questions, contact Mariusz Ryndzionek at:

<mryndzionek@gmail.com>
