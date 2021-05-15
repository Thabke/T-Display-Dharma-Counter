# T-Display-Dharma-Counter
## Tally counter with prostrations auto counting function for LilyGO TTGO T-Display ESP32 board. 

<b>Version 1.0</b>

### Connections

<b>External buttons connected to GND and board pins:</b><br>
Pin 2  - [INC], [+] increment button<br>
Pin 27 - [DEC], [-] decrement button<br>
Pin 12 - [Audio], [Pause] button<br>
<b>Build in buttons:</b><br>
Pin 0  - [NXT] next mode button<br>
Pin 35 - [PRV] previous mode button<br>
<b>VL53L0X module for prostrations counter connected:</b><br>
SCL to pin 22 of board<br>
SDA to pin 21 of board<br>
<b>Micro audio speaker (10mm) connected to:</b><br>
Pin 25 and GND

### Schematic

<img src="https://github.com/Thabke/T-Display-Dharma-Counter/raw/main/photos/CounterSchematic.jpg" alt="Schematic" width="515" height="476">

### Functionality

[INC]   - increase current counter<br>
[DEC]   - decrease current counter<br>
[DEC] long press  - reset current counter<br>
[NXT]   - next counter mode<br>
[PRV]   - previous counter mode<br>
[Audio] - Sound ON/OFF and Pause/Count in prostartions counter mode<br>
[NXT] long press or [PRV] long press - Configuration menu

### Configuration menu

The menu has minimal functionality. Most of the configuration is carried out through the web server in AP mode.<br>
[NXT] - next menu element<br>
[PRV] - previous menu element<br>
[INC] - select/increase value<br>
[DEC] - decrease value

### Web server

Web server in AP (Access Point) mode. After connecting to the Access Point via WiFi, use the IP address in the browser to open the configuration page.<br>
<b>Buttons:</b><br>
[INC] - show QR code for server IP<br>
[DEC] - exit from AP mode<br>

### Prostrations auto counting

A distance sensor VL53L0X is used for auto counting.<br>
<b>Counting logic:</b><br>
If the distance is less than specified (70 cm) during a certain time, then this is defined as a prostration.<br>
After the distance has become more than specified (70 cm) for a certain time, it is defined as the beginning of a new prostration.<br>
<img src="https://github.com/Thabke/T-Display-Dharma-Counter/raw/main/photos/prostrations_demo.jpg" alt="Using demo" width="450" height="164">

### Compilation of the application

The application is compiled in the Arduino IDE.<br>
The application size is about 1.8 Mb, so use partition scheme with 2 Mb or more for APP.<br>
All files from \data folder, must be uploaded to the SPIFFS.<br>
For that purpose you can use this tool: https://github.com/me-no-dev/arduino-esp32fs-plugin<br>
Or this tool: https://github.com/dsiberia9s/DESKTOP_A-Explorer_File_Browser_via_Serial<br>
<b>Used external libraries:</b><br>
Battery level: Pangodream_18650_CL - https://www.pangodream.es/esp32-getting-battery-charging-level<br>
Audio library: XT_DAC_Audio - https://www.xtronical.com/the-dacaudio-library-download-and-installation<br>
VL53L0X Distance Sensor: Adafruit_VL53L0X - https://github.com/adafruit/Adafruit_VL53L0X<br>
Buttons: EasyButton - https://easybtn.earias.me/ , https://github.com/evert-arias/EasyButton<br>
JSON configuration files: ArduinoJson - https://arduinojson.org/ , https://github.com/bblanchon/ArduinoJson<br> 
Web Server:<br>
AsyncTCP - https://github.com/me-no-dev/AsyncTCP<br>
ESPAsyncWebServer - https://github.com/me-no-dev/ESPAsyncWebServer<br>
QR Code - https://github.com/ricmoo/qrcode/

### Some photos

<img src="https://github.com/Thabke/T-Display-Dharma-Counter/raw/main/photos/photo1.jpg" alt="Counter 7" width="800" height="567">
<img src="https://github.com/Thabke/T-Display-Dharma-Counter/raw/main/photos/photo2.jpg" alt="Counter 21" width="522" height="278">
<img src="https://github.com/Thabke/T-Display-Dharma-Counter/raw/main/photos/photo3.jpg" alt="Counter 108" width="522" height="296">
<img src="https://github.com/Thabke/T-Display-Dharma-Counter/raw/main/photos/photo4.jpg" alt="Prostrations counter" width="522" height="309">
<img src="https://github.com/Thabke/T-Display-Dharma-Counter/raw/main/photos/photo5.jpg" alt="Tally counter 3" width="522" height="300">
