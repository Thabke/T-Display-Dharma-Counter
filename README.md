# T-Display-Dharma-Counter
<h2>Tally counter with the function of the auto counting of prostrations with a dharmic bias for LilyGO TTGO T-Display ESP32 board.</h2>

##Version 1.0

##Connections

External buttons connected to GND and board pins:<br>
Pin 2  - [INC], [+] icrement button<br>
Pin 27 - [DEC], [-] decrement button<br>
Pin 12 - [Audio], [Pause] button<br>
Build in buttons:<br>
Pin 0  - [NXT] next mode button<br>
Pin 35 - [PRV] previous mode button<br>
VL53L0X module for prostrations counter connected:<br>
SCL to pin 22 of board<br>
SDA to pin 21 of board<br>
Micro audio speaker (10mm) connected to:<br>
Pin 25 and GND

##Functionality

[INC]   - increase current counter<br>
[DEC]   - decrease current counter<br>
[DEC] long press  - reset current counter<br>
[NXT]   - next counter mode<br>
[PRV]   - previous counter mode<br>
[Audio] - Sound ON/OFF and Pause in prostartions counter mode<br>
[NXT] long press or [PRV] long press - Configuration menu

##Configuration menu

The menu has minimal functionality. Most of the configuration is carried out through the web server in AP mode.<br>
[NXT] - next menu element<br>
[PRV] - previous menu element<br>
[INC] - select/increase value<br>
[DEC] - decrease value

##Web server

Web server in AP (Access Point) mode. After connecting to the Access Point via WiFi, use the IP address in the browser to open the configuration page.<br>
[INC] - show QR code for server IP<br>
[DEC] - exit from AP mode<br>

Application size about 1.8 Mb, so use partition scheme with 2 Mb for APP.<br>
All files from \data folder, must be uploaded to the SPIFFS.<br>
Use this tool: https://github.com/me-no-dev/arduino-esp32fs-plugin<br>
Or this: https://github.com/dsiberia9s/DESKTOP_A-Explorer_File_Browser_via_Serial<br>
