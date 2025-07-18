# STM32F756ZG Peripheral Tests

This repository includes:
* [f756-peripheral-tests-server](f756-peripheral-tests-server) A peripheral testing program for the STM32 Nucleo-F756ZG evaluation board, implemented as a server accepting test requests and outputting test results.
* [test-client](test-client) A Linux based CLI-driven client for authoring and sending test requests, which logs all requests and results in a local Sqlite DB.

<b>Usage:</b>

The <b>test server</b> must be connected via its ethernet port to a local network that can assign it an IP address via DHCP.

____ Details about the peripherals tested +  wiring go here


The <b>test client</b> must simply be connected to the same network.

When the client starts, it begins a simple procedure to automatically pair with the server.\*

The client presents a simple CLI loop, where the user is prompted to interactively form a test request.

The client may be terminated at any point using Ctrl-C, with no adverse effects.

A simple [bash script](test-client/bundled_scripts/print_db.sh) that will print all logged requests/results is included alongside the client executable.


---------------------------------------------------


\*currently no method for selecting between available servers has been implemented, and the presence of multiple test servers on the network will form a race condition
