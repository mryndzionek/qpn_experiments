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

Implemented examples
--------------------

* simple `blink` example
		![](images/blink.png?raw=true)
* `Pelican crossing` example
		![](images/pelican.png?raw=true)
		

Notes and Things to Investigate
-------------------------------

* QM tool PNG export does not work on Linux (Mint)
* Use the latest QP-nano release to test the new QMsm class

Contact
-------
If you have questions, contact Mariusz Ryndzionek at:

<mryndzionek@gmail.com>
