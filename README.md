# T-Display-Dharma-Counter
Tally counter with the function of the auto counting of prostrations with a dharmic bias for LilyGO TTGO T-Display ESP32 board. 

Version 1.0

Connections

External buttons connected to GND and board pins:
Pin 2  - [INC], [+] icrement button
Pin 27 - [DEC], [-] decrement button
Pin 12 - [Audio], [Pause] button
Build in buttons:
Pin 0  - [NXT] next mode button
Pin 35 - [PRV] previous mode button
VL53L0X module for prostrations counter connected:
SCL to pin 22 of board
SDA to pin 21 of board
Micro audio speaker (10mm) connected to:
Pin 25 and GND

Functionality

[INC]   - increase current counter
[DEC]   - decrease current counter
[DEC] long press  - reset current counter
[NXT]   - next counter mode
[PRV]   - previous counter mode
[Audio] - Sound ON/OFF and Pause in prostartions counter mode
[NXT] long press or [PRV] long press - Configuration menu

Configuration menu

The menu has minimal functionality. Most of the
configuration is carried out through the web server in AP mode.
[NXT] - next menu element
[PRV] - previous menu element
[INC] - select/increase value
[DEC] - decrease value

Web server

Web server in AP (Access Point) mode.
After connecting to the Access Point via WiFi,
use the IP address in the browser to open the configuration page.
[INC] - show QR code for server IP
[DEC] - exit from AP mode

Application size about 1.8 Mb, so use partition scheme with 2 Mb for APP.
All files from \data folder, must be uploaded to the SPIFFS.
Use this tool: https://github.com/me-no-dev/arduino-esp32fs-plugin
Or this: https://github.com/dsiberia9s/DESKTOP_A-Explorer_File_Browser_via_Serial
