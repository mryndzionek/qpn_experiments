QP-nano framework on ATmega32A
==============================

Introduction
------------

Some tests/examples of the [QP-framework](http://www.state-machine.com/qp/qpn/)

- learn the to use the framework and RTOS concepts 
- investigate performance/limitations while using low-end hardware

Dependencies
------------

gcc-avr, avr-libc, avrdude, cmake

Build instructions
------------------

1. Clone the repo:

		git clone https://github.com/mryndzionek/qpn_experiments.git

2. Edit the gcc-avr options in `qpn_experiments/toolchain-avr-gcc.make` file

3. Download the QP-nano framework (4.5.02a) from [here](http://sourceforge.net/projects/qpc/files/QP-nano/4.5.02a/) and unpack it into the cloned directory

4. Download the [QM](http://sourceforge.net/projects/qpc/files/QM/3.0.1/) Graphical Modeling Tool (optional - to open the `*.qm` files)

5. Configure the project:

		cd qpn_experiments
		mkdir build
		cd build
		cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-avr-gcc.make -DCMAKE_BUILD_TYPE=MinSizeRel ..

6. Build the project:

		make

7. Upload the simple `blink` example hex file to the MCU:

		make upload_blink

Hardware setup
--------------

ATmega32A on a breadboard. 16MHz crystal. Fuses set to: `-U lfuse:w:0xff:m -U hfuse:w:0xd9:m`.
Serial LED resistors - 120Ohm. 16x2 LCD - HD44780 compatible - 10kOhm contrast pot.

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
| PC2  | On/Off button |
| PC3  | 'Pedestrians waiting' button |
| AREF | 100nF to GND |


Implemented examples
--------------------

* simple LED blink example - to test the setup and gcc compilation options
![blink](images/blink.png?raw=true "simple LED blink")

* Basic 'safety critical' system - **PE**destrian **LI**ght **CON**trolled crossing
![pelican](images/pelican.png?raw=true "Pelican crossing example")
		

Notes and Things to Investigate
-------------------------------

* QM tool PNG export does not work on Linux (Mint)
* Use the latest QP-nano release to test the new QMsm class

Contact
-------
If you have questions, contact Mariusz Ryndzionek at:

<mryndzionek@gmail.com>
