# STM32F756ZG Peripheral Tests

This repository includes:
* [f756-peripheral-tests-server](f756-peripheral-tests-server) A peripheral testing program for the STM32 Nucleo-F756ZG evaluation board, implemented as a server accepting test requests and outputting test results.
* [test-client](test-client) A Linux based CLI-driven client for authoring and sending test requests, which logs all requests and results in a local Sqlite DB.

----

<b>Usage:</b>

The <b>test server</b> must be connected via its ethernet port to a local network that can assign it an IP address via DHCP.

The peripheral tests and their wiring details are as follows:
* Timer 1 outputs PWM signals from Channel 3, evaluated using Input Capture on channel 2.
  * PE9 <-> PE 13
* USART2 and USART6 transmit and receive the supplied test data in both directions.
  * PD5 <-> PC7
  * PD6 <-> PC6
* SPI3 and SPI5 transmit and receive the supplied test data in both directions.
  * PC10 <-> PF7
  * PC11 <-> PF8
  * PC12 <-> PF9
* I2C1 and I2C2 transmit and receive the supplied test data in both directions.
  * PB9 <-> PF0
  * PB8 <-> PF1
* ADC1 is the simplest, only being evaluated on its ability to correctly measure the 3.3v port.
  * PA3 <-> 3.3v

The <b>test client</b> must simply be connected to the same network.
When the client starts, it begins a simple procedure to automatically pair with the server.\*

The client presents a simple CLI loop, where the user is prompted to interactively form a test request.
After sending a test request, the client awaits responses from the server,
and only resumes interactivity once a request has been completely fulfilled, rejected, or timed out with no acknowledgement.

The program may be terminated at any point using Ctrl-C, with no adverse effects.

A [bash script](test-client/bundled_scripts/print_db.sh) that will print all logged requests/results is included alongside the client executable.


---------------------------------------------------


\*Currently, no method for selecting between available servers has been implemented, and the presence of multiple test servers on the network will form a race condition.
