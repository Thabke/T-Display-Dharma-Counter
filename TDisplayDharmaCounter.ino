//****************************************
//* Dharma Counter for TTGO T-Display,   *
//* with prostrations counting function. *
//*             Version 1.0              *
//****************************************
//***********************************************************************************
//* Connections
//***********************************************************************************
// External buttons connected to GND and board pins:
// Pin 2  - [INC], [+] increment button
// Pin 27 - [DEC], [-] decrement button
// Pin 12 - [Audio], [Pause] button
// Build in buttons:
// Pin 0  - [NXT] next mode button
// Pin 35 - [PRV] previous mode button
// VL53L0X module for prostrations counter connected:
// SCL to pin 22 of board
// SDA to pin 21 of board
// Micro audio speaker (10mm) connected to:
// Pin 25 and GND
//***********************************************************************************
//* Functionality
//***********************************************************************************
// [INC]   - increase current counter
// [DEC]   - decrease current counter
// [DEC] long press  - reset current counter
// [NXT]   - next counter mode
// [PRV]   - previous counter mode
// [Audio] - Sound ON/OFF and Pause in prostartions counter mode
// [NXT] long press or [PRV] long press - Configuration menu
//***********************************************************************************
//* Configuration menu
//***********************************************************************************
// The menu has minimal functionality. Most of the
// configuration is carried out through the web server in AP mode.
// [NXT] - next menu element
// [PRV] - previous menu element
// [INC] - select/increase value
// [DEC] - decrease value
//***********************************************************************************
//* Web server
//***********************************************************************************
// Web server in AP (Access Point) mode.
// After connecting to the Access Point via WiFi,
// use the IP address in the browser to open the configuration page.
// [INC] - show QR code for server IP
// [DEC] - exit from AP mode
//***********************************************************************************
// Application size about 1.8 Mb, so use partition scheme with 2 Mb for APP.
// All files from \data folder, must be uploaded to the SPIFFS.
// Use this tool: https://github.com/me-no-dev/arduino-esp32fs-plugin
// Or this: https://github.com/dsiberia9s/DESKTOP_A-Explorer_File_Browser_via_Serial
//***********************************************************************************

#include "FS.h"						          //Files implementation
#include "SPIFFS.h"					        //SPI File System
#include <SPI.h>					          //SPI
#include <TFT_eSPI.h>         		  //Hardware-specific library
#include <EasyButton.h>       		  //Buttons
#include <ArduinoJson.h>      		  //JSON
//Distance sensor
#include <Adafruit_VL53L0X.h>       //VL53L0X laser distance meter's library
//Battery control
#include <Pangodream_18650_CL.h>    //https://www.pangodream.es/esp32-getting-battery-charging-level
//Audio library
#include "XT_DAC_Audio.h"           //https://www.xtronical.com/the-dacaudio-library-download-and-installation/
#include "sounds.h"                 //All sounds (converted WAV files)
//Web Server
#include <WiFi.h>
#include <AsyncTCP.h>               //https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h>      //https://github.com/me-no-dev/ESPAsyncWebServer
#include <qrcode.h>                 //QR code generation
//Header
#include "TDisplayDharmaCounter.h"	//Header file
//Fonts
#include "KuraleRegular24.h"        //Kurale font large
#include "KuraleRegular18.h"        //Kurale font medium
#include "KuraleRegular12.h"        //Kurale font small
//GFX
#include "dharma_counter_title.h" 	// Title screen
#include "big_num.h"              	// Big size numbers
#include "med_num.h"              	// Medium size numbers
#include "sml_num.h"              	// Small size numbers
#include "big_num_red.h"            // Big size numbers
#include "med_num_red.h"            // Medium size numbers
#include "sml_num_red.h"            // Small size numbers
#include "num0_gray.h"              // 0 number all three sizes in gray color
#include "vishva.h"               	// Vishva vajra image
#include "vajra.h"                	// Vajra image
#include "battery.h"                // Battery icon images
#include "mala108.h"              	// Mala image
#include "bead.h"                 	// Bead image
#include "playpause.h"              // "Play" and "Pause" icons for prostrations counter

//*************
//*** Setup ***
//*************
void setup()
{
  // Initialize Serial port
  Serial.begin(115200);

  //Mount SPIFFS
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Init Display
  tft.init();
  tft.setRotation(1); //Horizontal screen
  tft.setSwapBytes(true);

  //Display brightness regulation
  ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  ledcAttachPin(TFT_BL, pwmLedChannelTFT);              //Brightness LED on pin TFT_BL

  //Draw "Dharma counter" title
  tft.pushImage(0, 0, MAINTITLE_W, MAINTITLE_H, dharma_counter_title);

  //Initializing buttons
  btnINC.begin();
  btnDEC.begin();
  btnNXT.begin();
  btnPRV.begin();
  btnPRS.begin();
  btnINC.onPressed(INCjustpressed);                      //Just press
  btnDEC.onPressed(DECjustpressed);                      //Just press
  btnDEC.onPressedFor(LONG_PRESS_TIME, DEClongpress);    //Long press
  btnNXT.onPressed(NXTjustpressed);                      //Just press
  btnNXT.onPressedFor(LONG_PRESS_TIME, NXTlongpress);    //Long press
  btnPRV.onPressed(PRVjustpressed);                      //Just press
  btnPRV.onPressedFor(LONG_PRESS_TIME, PRVlongpress);    //Long press
  btnPRS.onPressed(PRSjustpressed);                      //Just press

  // Load a smooth font
  tft.loadFont(TITLE_FONT_LARGE);

  //Load data from JSON files
  loadModes();
  loadSystem();
  loadCounters();

  //Change display brightness
  changeBrightness (brightness);

  //Initialize the distance sensor
  if (counter[PROSTRATIONS].show)  //If prostrations mode is activated
  {
    if (!l0x.begin())
      Serial.println("Failed to initialize VL53L0X");
  }

  //Delay for showing the main title
  delay (1500); //Wait 1.5 sec

  //Clear screen
  tft.fillScreen(TFT_BLACK);

  //Check fresh start time for power saving
  pwr_last_time = millis();

  //If last mode after restart become hidden, go to next available mode
  if (!counter[mode].show)
  {
    last_mode = mode;
    do
    {
      if (mode < MODES - 1)
        mode++;
      else
        mode = 0;
      if (mode == last_mode) //All modes are hidden
      {
        all_hidden = true;
        break;
      }
    } while (!counter[mode].show); //If new mode is hidden, go to next mode
  }

  if (all_hidden) //All modes are hidden
    webServer();  //Start WiFi AP for edit settings
  else  //Usual start
  {
    //Draw mode title
    drawTitle();
    //Draw current counter mode
    drawMode ();
    //Draw battery status icon
    drawBattery ();
    //Draw audio status icon only on first start
    drawAudio (); //Draw audio status icon
  }
}
///////////////////////////////////////////////////////////////

//****************
//*** Main Loop **
//****************
void loop()
{
  //Sound buffer
  DacAudio.FillBuffer();  // This needs only be in your main loop once, suggest here at the top.

  //Buttons checking
  btnCheck ();

  //Update current time value
  current_time = millis ();

  //Battery level checking
  batteryCheck ();

  //Distance checking for prostrations mode
  distanceCheck();

  //Power saving handling
  powerSaving ();
}
//****************

//***************************
//Changing display brightness
//***************************
void changeBrightness (byte brt)
{
  ledcWrite(pwmLedChannelTFT, brt);
}

//****************************************
//Buttons checking
//[INC]               - increase counter
//[DEC]               - decrease counter
//[DEC] (long press)  - reset counter to 0
//[NXT]               - next mode
//[PRV]               - previous mode
//****************************************
void btnCheck ()
{
  btnINC.read();     //update the incrementing button state
  btnDEC.read();     //update the decrementing button state
  btnNXT.read();     //update the decrementing button state
  btnPRV.read();     //update the decrementing button state
  btnPRS.read();     //update the decrementing button state
}

//****************
//Buttons handlers
//////////////////

//[Increase] button press handler
void INCjustpressed()
{
  //Refresh last button press time for power saving
  pwr_last_time = millis();

  if (!isAPmode)
  {
    if (!isMenuMode)
    {
      increaseCounter ();  //Increase counter value

      drawMode ();
    }
    else  //Menu mode functionality
    {
      switch (menuPosition)
      {
        case 1:
          if (power_save)
          {
            playSound(dec);  //play sound
            power_save = false;
          }
          else
          {
            playSound(inc);  //play sound
            power_save = true;
          }
          modeMenuDraw(); //redraw menu
          break;
        case 2:
          if (brightness <= BRIGHTNESS_MAX - 5)
            brightness = brightness + 5;
          playSound(inc);   //play sound
          modeMenuDraw();   //redraw menu
          break;
        case 3:
          //isMenuMode = false;
          webServer();
          break;
        case 4: //Exit from configuration menu
          isMenuMode = false;
          power_save_disable = false; //Return sleeping possibility
          playSound(previous);        //play sound
          //Draw current mode
          tft.fillScreen(TFT_BLACK);  //Clear screen
          drawTitle();                //Draw current mode title
          drawMode();                 //Draw current mode screen
          drawBattery();              //Draw battery icon
          break;
      }
    }
  }
  else
  {
    if (isQRmode)
    {
      isQRmode = false;
      modeAPDraw();
      drawBattery (); //Draw battery status icon
    }
    else
      modeQRDraw();
  }
}
//[Decrease] button press handler
void DECjustpressed()
{
  //Refresh last button press time for power saving
  pwr_last_time = millis();

  if (!isAPmode)
  {
    if (!isMenuMode)
    {
      decreaseCounter ();  //Decrease counter value

      drawMode ();
    }
    else  //Menu mode functionality
    {
      if (menuPosition == 2)
      {
        if (brightness >= BRIGHTNESS_MIN + 5)
          brightness = brightness - 5;
        playSound(dec);   //play sound
        modeMenuDraw(); //redraw menu
      }
    }
  }
  else
    modeAPExit(); //Exit from AP mode
}
//[Next Mode] button press handler
void NXTjustpressed()
{
  //Refresh last button press time for power saving
  pwr_last_time = millis();

  if (!isAPmode)
  {
    if (!isMenuMode)
      nextMode(); //Next mode
    else  //Menu mode functionality
    {
      menuPositionPrv = menuPosition; //save old menu position
      menuPosition++;                 //increase current position
      if (menuPosition > ELEMENTS)    //if position was on last element go to first element
        menuPosition = 1;

      playSound(next);  //play sound
      modeMenuDraw();   //redraw menu
    }

  }
}
//[Previous Mode] button press handler
void PRVjustpressed()
{
  //Refresh last button press time for power saving
  pwr_last_time = millis();

  if (!isAPmode)
  {
    if (!isMenuMode)
      prevMode(); //Previous mode
    else  //Menu mode functionality
    {
      menuPositionPrv = menuPosition; //save old menu position
      menuPosition--;                 //decrease current position
      if (menuPosition < 1)           //if position was on first element go to last element
        menuPosition = ELEMENTS;

      playSound(previous);  //play sound
      modeMenuDraw();       //redraw menu
    }
  }
}

//[Next Mode] button longpress handler
void NXTlongpress ()
{
  //Refresh last button press time for power saving
  pwr_last_time = millis();

  menuMode(); //Start configuration menu mode
}
//[Previous Mode] button longpress handler
void PRVlongpress ()
{
  //Refresh last button press time for power saving
  pwr_last_time = millis();

  menuMode(); //Start configuration menu mode
}

//Sound ON/OFF or
//prostration counting START/PAUSE button press handler
void PRSjustpressed()
{
  //Refresh last button press time for power saving
  pwr_last_time = millis();

  if (mode == PROSTRATIONS && !isAPmode) //Work only in prostrations mode
  {
    if (isStart)
      pauseProstrations (); //Pause prostrations recognizing cycle
    else
    {
      isStart = true;

      //current_time = millis();          //Load current time
      last_msr_time = current_time;         //Reset time counting
    }

    prostrationsDraw ();
  }
  else  //In other then prostrations mode this button using for ON/OFF audio
  {
    if (soundON)
      soundON = false;
    else
      soundON = true;

    drawAudio (); //Draw audio status icon
    saveSystem();   //Save audio status
  }
}

//[Decrease] button longpress handler
//Reset the counter value to 0
void DEClongpress()
{
  //Refresh last button press time for power saving
  pwr_last_time = millis();

  if (!isAPmode)
  {
    if (counter[mode].value > 0)
      resetCounter (); //Reset the counter to 0

    drawMode ();
  }
}
/////////////////////////
//End of buttons handlers
//***********************

//****************************
//Increasing the counter value
//****************************
void increaseCounter ()
{
  if (counter[mode].value < counter[mode].max_value)
  {

    counter_old = counter[mode].value; //Save previous counter value
    counter[mode].value++;

    if (counter[mode].value < counter[mode].max_value)
      playSound(inc);  //play button sound
    else
      playSound(goal);  //play end of counting sound

    //Pause prostrations recognizing cycle
    if (mode == PROSTRATIONS)
      pauseProstrations ();

    if (mode > 1) //For modes 7 and 21 counters are not saved
      saveCounters();
  }
  else
    playSound(goal);  //Repeat end of counting sound
}

//Decrease the counter value
void decreaseCounter ()
{
  if (counter[mode].value > 0)
  {
    playSound(dec);  //play sound

    counter_old = counter[mode].value; //Save previous counter value
    counter[mode].value--;

    //Pause prostrations recognizing cycle
    if (mode == PROSTRATIONS)
      pauseProstrations ();

    if (mode > 1) //For modes 7 and 21 counters are not saved
      saveCounters();
  }
}

//Reset the counter value to 0
void resetCounter ()
{
  if (counter[mode].value > 0)
  {
    playSound(zero);  //play sound

    counter_old = counter[mode].value; //Save previous counter value
    counter[mode].value = 0;

    //Pause prostrations recognizing cycle
    if (mode == PROSTRATIONS)
      pauseProstrations ();

    if (mode > 1) //For mode 7 and mode 21 counters are not saved
      saveCounters();
  }
}

//Next mode
void nextMode ()
{
  //Pause prostrations recognizing cycle
  if (mode == PROSTRATIONS)
    pauseProstrations ();

  do
  {
    if (mode < MODES - 1)
      mode++;
    else
      mode = 0;
  } while (!counter[mode].show); //If new mode is hidden, go to next mode

  power_save_disable = false; //Return sleeping possibility if it was off

  saveSystem(); //Save current mode number

  playSound(next);  //play sound

  //Renew counter_old value
  counter_old = counter[mode].value;

  bat_y_shift = false; //Reset battery image shifting status
  snd_y_shift = false; //Reset audio state image shifting status

  tft.fillScreen(TFT_BLACK);  //Clear screen
  drawTitle();                //Draw new mode title
  drawMode();                 //Draw new mode screen
  drawBattery();              //Draw battery icon
}

//Previous mode
void prevMode ()
{
  //Pause prostrations recognizing cycle
  if (mode == PROSTRATIONS)
    pauseProstrations ();

  do
  {
    if (mode > 0)
      mode--;
    else
      mode = MODES - 1;
  } while (!counter[mode].show); //If new mode is hidden, go to next mode

  power_save_disable = false; //Return sleeping possibility if it was off

  saveSystem(); //Save current mode number

  playSound(previous);  //play sound

  //Renew counter_old value
  counter_old = counter[mode].value;

  bat_y_shift = false; //Reset battery image shifting status
  snd_y_shift = false; //Reset audio state image shifting status

  tft.fillScreen(TFT_BLACK);  //Clear screen
  drawTitle();                //Draw new mode title
  drawMode();                 //Draw new mode screen
  drawBattery();              //Draw battery icon
}

//Draw audio status icon
void drawAudio ()
{
  byte y_shift = 0;
  int x_shift  = 0;

  if (snd_y_shift)
  {
    y_shift = SND_Y_SHIFT;
    x_shift = SND_X_SHIFT;
  }

  if (soundON)
  {
    tft.pushImage(194 + x_shift, 3 + y_shift, AUDIO_W, AUDIO_H, audio_on); //Draw "Audio ON" icon
    playSound (snd_on);
  }
  else
    tft.pushImage(194 + x_shift, 3 + y_shift, AUDIO_W, AUDIO_H, audio_off); //Draw "Audio OFF" icon
}

//Draw the Menu mode
void modeMenuDraw ()
{
  //Draw title
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  tft.setCursor(5, 0);
  tft.print("Configuration");
  tft.drawFastHLine(0, TITLE_LINE_Y, 240, TFT_GOLD); //Separation line

  // Remove previous font parameters from memory to recover RAM
  tft.unloadFont();
  // Load the medium font
  tft.loadFont(TITLE_FONT_MEDIUM);

  //Remove previous rectangle around selected menu element
  if (menuPositionPrv != menuPosition)
    tft.fillRoundRect(0, ELEMENT_Y + ELEMENT_H * (menuPositionPrv - 1) - 3, TFT_W, ELEMENT_H, 4, TFT_BLACK);
  //Draw rectangle around selected menu element
  tft.fillRoundRect(0, ELEMENT_Y + ELEMENT_H * (menuPosition - 1) - 3, TFT_W, ELEMENT_H, 4, TFT_BLACK);
  tft.drawRoundRect(0, ELEMENT_Y + ELEMENT_H * (menuPosition - 1) - 3, TFT_W, ELEMENT_H, 4, TFT_GOLD);

  //Power saving configuration menu element
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(6, ELEMENT_Y);
  tft.print("Power saving : ");
  if (power_save)
  {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.print("ON");
  }
  else
  {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.print("OFF");
  }

  //Brightness configuration menu element
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(6, ELEMENT_Y + ELEMENT_H);
  tft.print("Brightness : ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(brightness);

  //Start configuration web server menu element
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(6, ELEMENT_Y + ELEMENT_H * 2);
  tft.print("Configuration Web Server");

  //Start configuration web server menu element
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(6, ELEMENT_Y + ELEMENT_H * 3);
  tft.print("Exit from menu");

  // Restore to default large font
  // Remove previous font parameters from memory to recover RAM
  tft.unloadFont();
  // Load the large font
  tft.loadFont(TITLE_FONT_LARGE);
}

//Draw the Access Point (AP) mode
void modeAPDraw ()
{
  isAPmode = true;

  playSound(next);  //play sound

  tft.fillScreen(TFT_BLACK);  //Clear screen

  //Draw title
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  tft.setCursor(5, 0);
  tft.print("WiFi Access Point");
  tft.drawFastHLine(0, TITLE_LINE_Y, 240, TFT_GOLD); //Separation line

  // Remove previous font parameters from memory to recover RAM
  tft.unloadFont();
  // Load the small font
  tft.loadFont(TITLE_FONT_SMALL);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(0, 35);
  tft.print("WiFi Name :  ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(ssid);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(0, 51);
  tft.print("Password :  ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (password != "")
    tft.print(password);
  else
    tft.print("No password requred");

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(5, 71);
  tft.print("IP Address (Use it in browser address field)");


  // Remove previous font parameters from memory to recover RAM
  tft.unloadFont();
  // Load the large font
  tft.loadFont(TITLE_FONT_LARGE);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor((TFT_W - tft.textWidth(myIP)) / 2, 86); //Put text in the centre of the screen
  tft.print(myIP);

  // Remove previous font parameters from memory to recover RAM
  tft.unloadFont();
  // Load the medium font
  tft.loadFont(TITLE_FONT_MEDIUM);

  tft.drawFastHLine(0, TFT_H - TITLE_LINE_Y, 240, TFT_GOLD); //Separation line
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.setCursor(1, TFT_H - TITLE_LINE_Y + 5);
  tft.print("[INC] - QR Code    [DEC] - Exit");

  // Restore to default large font
  // Remove previous font parameters from memory to recover RAM
  tft.unloadFont();
  // Load the large font
  tft.loadFont(TITLE_FONT_LARGE);
}

//Exit from AP mode
void modeAPExit ()
{
  //If current mode become hidden, go to next available mode
  if (!counter[mode].show)
  {
    last_mode = mode;
    do
    {
      if (mode < MODES - 1)
        mode++;
      else
        mode = 0;
      if (mode == last_mode) //All modes are hidden
        return;  //return to WiFi AP for edit settings
    } while (!counter[mode].show); //If new mode is hidden, go to next mode
  }

  //Stop WiFi
  WiFi.mode(WIFI_OFF);

  Serial.println ("WiFi modem off...");

  //btStop(); //Stop Bluetooth

  isAPmode = false;

  playSound(previous);  //play sound

  isMenuMode = false;

  //Return sleeping possibility for all modes
  power_save_disable = false;

  //Draw current mode
  tft.fillScreen(TFT_BLACK);  //Clear screen
  drawTitle();                //Draw current mode title
  drawMode();                 //Draw current mode screen
  drawBattery();              //Draw battery icon
}

//Draw generated QR code for AP mode IP adress
void modeQRDraw ()
{
  // Create the QR code
  QRCode qrcode;
  //Convert myIP string to char*
  String str = "http://" + myIP;
  char qrtext[str.length() + 1];
  str.toCharArray(qrtext, str.length() + 1);

  Serial.println(str);

  uint8_t qrcodeData[qrcode_getBufferSize(2)];

  int QRxBegin = 65;
  int QRyBegin = 7;
  int QRmoduleSize = 4;

  isQRmode = true;  //QR code mode is started

  playSound(next);  //play sound

  tft.fillScreen(TFT_WHITE);  //Fill the screen with white color

  qrcode_initText(&qrcode, qrcodeData, 2, ECC_LOW, qrtext);

  // Draw QR code
  for (uint8_t y = 0; y < qrcode.size; y++)
  {
    // Each horizontal module
    for (uint8_t x = 0; x < qrcode.size; x++)
    {
      if (qrcode_getModule(&qrcode, x, y))
        tft.fillRect(QRxBegin + x * QRmoduleSize, QRyBegin + y * QRmoduleSize, QRmoduleSize, QRmoduleSize, TFT_BLACK);
    }
  }

  //Print numeric IP
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setCursor((TFT_W - tft.textWidth(myIP)) / 2, 115); //Put text in the centre of the screen
  tft.print(myIP);
}

//Draw the vishva vajra for mode 7
void mode7Draw ()
{
  if (counter[mode].value == 7) //7 is maximum counter value for this mode
  {
    //Draw counter value
    //Remove old value
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.setCursor(134, 0);
    tft.print("[");
    tft.print(counter_old);
    tft.print("]");
    //Print counter value
    tft.setTextColor(LIGHTRED, TFT_BLACK);
    tft.setCursor(134, 0);
    tft.print("[");
    tft.print(counter[mode].value);
    tft.print("]");

    for (int i = 0; i < 4; i++) {
      tft.pushImage((VISHVA_W + 4) * i + 10, 30, VISHVA_W, VISHVA_H, vishva_red); //Draw red color vishva vajra
    }
    for (int i = 0; i < 3; i++) {
      tft.pushImage((VISHVA_W + 4) * i + VISHVA_W / 2 + 10, VISHVA_H + 32, VISHVA_W, VISHVA_H, vishva_red); //Draw red color vishva vajra
    }
  }
  else
  {
    //Draw counter value
    //Remove previous number of counter value
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.setCursor(134, 0);
    tft.print("[");
    tft.print(counter_old);
    tft.print("]");
    //Print counter value
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.setCursor(134, 0);
    tft.print("[");
    tft.print(counter[mode].value);
    tft.print("]");

    if (counter[mode].value > 0) //if current counter value 1 or more
    {
      //Draw the vishva vajra sprites
      for (int i = 0; i < counter[mode].value; i++) {
        if (i < 4)
          tft.pushImage((VISHVA_W + 4) * i + 10, 30, VISHVA_W, VISHVA_H, vishva); //Draw vishva vajra
        else
          tft.pushImage((VISHVA_W + 4) * (i - 4) + VISHVA_W / 2 + 10, VISHVA_H + 32, VISHVA_W, VISHVA_H, vishva); //Draw vishva vajra
      }
      //Draw the gray vishva vajra footprints
      for (int i = counter[mode].value; i < 7; i++) {
        if (i < 4)
          tft.pushImage((VISHVA_W + 4) * i + 10, 30, VISHVA_W, VISHVA_H, vishva_footprint); //Draw vishva vajra
        else
          tft.pushImage((VISHVA_W + 4) * (i - 4) + VISHVA_W / 2 + 10, VISHVA_H + 32, VISHVA_W, VISHVA_H, vishva_footprint); //Draw vishva vajra
      }
    }
    else //counter value == 0
    {
      for (int i = 0; i < 4; i++) {
        tft.pushImage((VISHVA_W + 4) * i + 10, 30, VISHVA_W, VISHVA_H, vishva_footprint); //Draw gray vishva vajra
      }
      for (int i = 0; i < 3; i++) {
        tft.pushImage((VISHVA_W + 4) * i + VISHVA_W / 2 + 10, VISHVA_H + 32, VISHVA_W, VISHVA_H, vishva_footprint); //Draw gray vishva vajra
      }
    }
  }
}

//Draw the vajra for mode 21
void mode21Draw ()
{
  if (counter[mode].value == 21)  //21 is maximum counter value for this mode
  {
    //Draw counter value
    //Remove old value
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.setCursor(140, 0);
    tft.print("[");
    tft.print(counter_old);
    tft.print("]");
    //Print counter value
    tft.setTextColor(LIGHTRED, TFT_BLACK);
    tft.setCursor(140, 0);
    tft.print("[");
    tft.print(counter[mode].value);
    tft.print("]");

    for (int i = 0; i < 11; i++) {
      tft.pushImage((VAJRA_W + 2) * i + 4, 30, VAJRA_W, VAJRA_H, vajra_red); //Draw red color vishva vajra
    }
    for (int i = 0; i < 10; i++) {
      tft.pushImage((VAJRA_W + 2) * i + VAJRA_W / 2 + 4, VAJRA_H + 32, VAJRA_W, VAJRA_H, vajra_red); //Draw red color vishva vajra
    }
  }
  else
  {
    //Draw counter value
    //Remove previous number of counter value
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.setCursor(140, 0);
    tft.print("[");
    tft.print(counter_old);
    tft.print("]");
    //Print counter value
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.setCursor(140, 0);
    tft.print("[");
    tft.print(counter[mode].value);
    tft.print("]");

    if (counter[mode].value > 0) //if current counter value 1 or more
    {
      //Draw the vishva vajra sprites
      for (int i = 0; i < counter[mode].value; i++) {
        if (i < 11)
          tft.pushImage((VAJRA_W + 2) * i + 4, 30, VAJRA_W, VAJRA_H, vajra); //Draw vishva vajra
        else
          tft.pushImage((VAJRA_W + 2) * (i - 11) + VAJRA_W / 2 + 4, VAJRA_H + 32, VAJRA_W, VAJRA_H, vajra); //Draw vishva vajra
      }
      //Draw the gray vishva vajra footprints
      for (int i = counter[mode].value; i < 21; i++) {
        if (i < 11)
          tft.pushImage((VAJRA_W + 2) * i + 4, 30, VAJRA_W, VAJRA_H, vajra_footprint); //Draw vishva vajra
        else
          tft.pushImage((VAJRA_W + 2) * (i - 11) + VAJRA_W / 2 + 4, VAJRA_H + 32, VAJRA_W, VAJRA_H, vajra_footprint); //Draw vishva vajra
      }

    }
    else //counter == 0
    {
      for (int i = 0; i < 11; i++) {
        tft.pushImage((VAJRA_W + 2) * i + 4, 30, VAJRA_W, VAJRA_H, vajra_footprint); //Draw gray vishva vajra
      }
      for (int i = 0; i < 10; i++) {
        tft.pushImage((VAJRA_W + 2) * i + VAJRA_W / 2 + 4, VAJRA_H + 32, VAJRA_W, VAJRA_H, vajra_footprint); //Draw gray vishva vajra
      }
    }
  }
}

//Draw the mala for mode 108
void mode108Draw ()
{
  if (counter[mode].value == 108)  //108 is maximum counter value for this mode
  {
    //Draw counter value in red medium size digits
    draw_counter_med_red (-20, 15);

    //Draw mala background
    tft.pushImage(0, TFT_H - MALA_H - 1, MALA_W, MALA_H, mala108, TRANSPARENT); //Draw mala
    //Draw red beads
    for (int i = 0; i < 108; i++)
      tft.pushImage(beadXY[i][0], beadXY[i][1] + (TFT_H - MALA_H - 1), BEAD_W, BEAD_H, bead_red); //Draw red beads

    tft.pushImage(197, 79, BIGBEAD_W, BIGBEAD_H, big_bead_red); //Draw red beads
  }
  else if (counter[mode].value > 0)
  {
    //If new number have more or less digits then previous, clear screen
    if (numLength (counter[mode].value) != numLength (counter_old))
      tft.fillRect (25, 45, MEDNUM_W * 3, MEDNUM_H + 1, TFT_BLACK);

    //Draw counter value in medium size
    draw_counter_med (-20, 15);

    //Draw mala background
    tft.pushImage(0, TFT_H - MALA_H - 1, MALA_W, MALA_H, mala108, TRANSPARENT); //Draw mala
    //Draw the mala beads
    for (int i = 0; i < counter[mode].value; i++)
      tft.pushImage(beadXY[i][0], beadXY[i][1] + (TFT_H - MALA_H - 1), BEAD_W, BEAD_H, bead); //Draw yellow beads

    //If counter decreasing
    if (counter_old > counter[mode].value)
      tft.pushImage(beadXY[counter_old - 1][0], beadXY[counter_old - 1][1] + (TFT_H - MALA_H - 1), BEAD_W, BEAD_H, bead_grey); //Draw grey bead
  }
  else  //Draw mala background
  {
    tft.fillRect (0, 25, 240, 125, TFT_BLACK); //Clear screen
    draw_counter_med (-20, 15); //Draw counter value in medium size
    tft.pushImage(0, TFT_H - MALA_H - 1, MALA_W, MALA_H, mala108, TRANSPARENT); //Draw mala
  }
}

//Draw the tally counter modes
void tallymodeDraw ()
{
  if (counter[mode].value == counter[mode].max_value)  //Draw red colored counter number for showing end of counting
  {
    //If new number have more or less digits then previous, clear screen
    if (numLength (counter[mode].value) != numLength (counter_old))
      tft.fillRect (0, 25, 240, 125, TFT_BLACK);

    if (counter[mode].value < 1000)  //Counter length 1,2 or 3 digits
    {
      //Draw counter value in red big digits size
      draw_counter_big_red (0, 15);
    }
    else if (counter[mode].value < 10000)  //Counter length 4 digits
    {
      //Draw counter value in red medium digits size
      draw_counter_med_red (0, 15);
    }
    else  //Counter length 5 or 6 digits
    {
      //Draw counter value in red small digits size
      draw_counter_sml_red (0, 15);
    }
  }
  else if (counter[mode].value > 0)
  {
    if (counter[mode].value < counter[mode].max_value)
    {
      //If new number have more or less digits then previous, clear screen
      if (numLength (counter[mode].value) != numLength (counter_old))
        tft.fillRect (0, 25, 240, 125, TFT_BLACK);

      if (counter[mode].value < 1000)  //Counter length 1,2 or 3 digits
      {
        //Draw counter value in big digits size
        draw_counter_big (0, 15);
      }
      else if (counter[mode].value < 10000)  //Counter length 4 digits
      {
        //Draw counter value in medium digits size
        draw_counter_med (0, 15);
      }
      else  //Counter length 5 or 6 digits
      {
        //Draw counter value in small digits size
        draw_counter_sml (0, 15);
      }
    }
  }
  else
  {
    //Clear screen
    tft.fillRect (0, 25, 240, 125, TFT_BLACK);

    //Draw counter value in big digits size
    draw_counter_big (0, 15);
  }
}

//Draw the prostrations counter
void prostrationsDraw ()
{
  if (isStart)
    tft.pushImage(165, 1, PLAY_W, PLAY_H, play_img); //Draw "Play" icon
  else
    tft.pushImage(165, 1, PLAY_W, PLAY_H, pause_img); //Draw "Pause" icon

  if (counter[mode].value == counter[mode].max_value)  //Draw red colored counter number for showing end of counting
  {
    //If new number have more or less digits then previous, clear screen
    if (numLength (counter[mode].value) != numLength (counter_old))
      tft.fillRect (0, 25, 240, 125, TFT_BLACK);

    if (counter[mode].value < 1000)  //Counter length 1,2 or 3 digits
    {
      //Draw counter value in red big digits size
      draw_counter_big_red (0, 15);
    }
    else if (counter[mode].value < 10000)  //Counter length 4 digits
    {
      //Draw counter value in red medium digits size
      draw_counter_med_red (0, 15);
    }
    else  //Counter length 5 or 6 digits
    {
      //Draw counter value in red small digits size
      draw_counter_sml_red (0, 15);
    }
  }
  else if (counter[mode].value > 0)
  {
    if (counter[mode].value < counter[mode].max_value)
    {
      //If new number have more or less digits then previous, clear screen
      if (numLength (counter[mode].value) != numLength (counter_old))
        tft.fillRect (0, 25, 240, 125, TFT_BLACK);

      if (counter[mode].value < 1000)  //Counter length 1,2 or 3 digits
      {
        //Draw counter value in big digits size
        draw_counter_big (0, 15);
      }
      else if (counter[mode].value < 10000)  //Counter length 4 digits
      {
        //Draw counter value in medium digits size
        draw_counter_med (0, 15);
      }
      else  //Counter length 5 or 6 digits
      {
        //Draw counter value in small digits size
        draw_counter_sml (0, 15);
      }
    }
  }
  {
    //Clear screen
    tft.fillRect (0, 25, 240, 125, TFT_BLACK);

    //Draw counter value in big digits size
    draw_counter_big (0, 15);
  }
}

//Length of any number in digits
byte numLength (unsigned long num)
{
  String cnt;     //Number in text

  cnt = String (num); //Convert digits of the number to text
  return cnt.length();  //Find the length of the number
}

//Draw big size digits counter number from images (upto 3 digits on screen)
//x_shift, yshift - pixel shift from the centre of the screen
void draw_counter_big (int x_shift, int y_shift)
{
  String cnt;     //Counter value in text
  String digit;   //Different digits from the counter number
  int digits = 0; //length of the counter number

  cnt = String (counter[mode].value); //Convert digits of the counter to text
  digits = cnt.length();              //Find the length of the counter numbers

  switch (digits) {
    case 1: //Only one digit
      digit = cnt[0];
      if (counter[mode].value > 0)
        tft.pushImage((TFT_W - BIGNUM_W) / 2 + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum[digit.toInt()]); //Draw digit
      else
        tft.pushImage((TFT_W - BIGNUM_W) / 2 + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum0_gray); //Draw 0 in gray color
      break;
    case 2: //Two digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - BIGNUM_W + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum[digit.toInt()]); //Draw digit
      break;
    case 3: //Three digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage((TFT_W - BIGNUM_W) / 2 - BIGNUM_W + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage((TFT_W - BIGNUM_W) / 2 + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage((TFT_W - BIGNUM_W) / 2 + BIGNUM_W + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum[digit.toInt()]); //Draw digit
      break;
    default:
      // statements
      break;
  }
}

//Draw red color big size digits counter number from images (upto 3 digits on screen)
//x_shift, yshift - pixel shift from the centre of the screen
void draw_counter_big_red (int x_shift, int y_shift)
{
  String cnt;     //Counter value in text
  String digit;   //Different digits from the counter number
  int digits = 0; //length of the counter number

  cnt = String (counter[mode].value); //Convert digits of the counter to text
  digits = cnt.length();              //Find the length of the counter numbers

  switch (digits) {
    case 1: //Only one digit
      digit = cnt[0];
      tft.pushImage((TFT_W - BIGNUM_W) / 2 + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum_red[digit.toInt()]); //Draw digit
      break;
    case 2: //Two digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - BIGNUM_W + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum_red[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum_red[digit.toInt()]); //Draw digit
      break;
    case 3: //Three digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage((TFT_W - BIGNUM_W) / 2 - BIGNUM_W + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum_red[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage((TFT_W - BIGNUM_W) / 2 + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum_red[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage((TFT_W - BIGNUM_W) / 2 + BIGNUM_W + x_shift, (TFT_H - BIGNUM_H) / 2 + y_shift, BIGNUM_W, BIGNUM_H, bigNum_red[digit.toInt()]); //Draw digit
      break;
    default:
      // statements
      break;
  }
}

//Draw medium size digits counter number from images (upto 4 digits on screen)
//x_shift, yshift - pixel shift from the centre of the screen
void draw_counter_med (int x_shift, int y_shift)
{
  String cnt;     //Counter value in text
  String digit;   //Different digits from the counter number
  int digits = 0; //length of the counter number

  cnt = String (counter[mode].value); //Convert digits of the counter to text
  digits = cnt.length();              //Find the length of the counter numbers

  switch (digits) {
    case 1: //Only one digit
      digit = cnt[0];
      if (counter[mode].value > 0)
        tft.pushImage((TFT_W - MEDNUM_W) / 2 + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum[digit.toInt()]); //Draw digit
      else
        tft.pushImage((TFT_W - MEDNUM_W) / 2 + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum0_gray); //Draw digit 0 in gray color
      break;
    case 2: //Two digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - MEDNUM_W + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum[digit.toInt()]); //Draw digit
      break;
    case 3: //Three digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage((TFT_W - MEDNUM_W) / 2 - MEDNUM_W + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage((TFT_W - MEDNUM_W) / 2 + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage((TFT_W - MEDNUM_W) / 2 + MEDNUM_W + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum[digit.toInt()]); //Draw digit
      break;
    case 4: //Four digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - MEDNUM_W * 2 + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 - MEDNUM_W + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum[digit.toInt()]); //Draw digit
      //Fourth digit in the counter
      digit = cnt[3];
      tft.pushImage(TFT_W / 2 + MEDNUM_W + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum[digit.toInt()]); //Draw digit
      break;
    default:
      // statements
      break;
  }
}

//Draw red color medium size digits counter number from images (upto 4 digits on screen)
//x_shift, yshift - pixel shift from the centre of the screen
void draw_counter_med_red (int x_shift, int y_shift)
{
  String cnt;     //Counter value in text
  String digit;   //Different digits from the counter number
  int digits = 0; //length of the counter number

  cnt = String (counter[mode].value); //Convert digits of the counter to text
  digits = cnt.length();              //Find the length of the counter numbers

  switch (digits) {
    case 1: //Only one digit
      digit = cnt[0];
      tft.pushImage((TFT_W - MEDNUM_W) / 2 + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum_red[digit.toInt()]); //Draw digit
      break;
    case 2: //Two digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - MEDNUM_W + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum_red[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum_red[digit.toInt()]); //Draw digit
      break;
    case 3: //Three digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage((TFT_W - MEDNUM_W) / 2 - MEDNUM_W + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum_red[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage((TFT_W - MEDNUM_W) / 2 + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum_red[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage((TFT_W - MEDNUM_W) / 2 + MEDNUM_W + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum_red[digit.toInt()]); //Draw digit
      break;
    case 4: //Four digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - MEDNUM_W * 2 + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum_red[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 - MEDNUM_W + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum_red[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum_red[digit.toInt()]); //Draw digit
      //Fourth digit in the counter
      digit = cnt[3];
      tft.pushImage(TFT_W / 2 + MEDNUM_W + x_shift, (TFT_H - MEDNUM_H) / 2 + y_shift, MEDNUM_W, MEDNUM_H, medNum_red[digit.toInt()]); //Draw digit
      break;
    default:
      // statements
      break;
  }
}

//Draw small size digits counter number from images (upto 6 digits on screen)
//x_shift, yshift - pixel shift from the centre of the screen
void draw_counter_sml (int x_shift, int y_shift)
{
  String cnt;     //Counter value in text
  String digit;   //Different digits from the counter number
  int digits = 0; //length of the counter number

  cnt = String (counter[mode].value); //Convert digits of the counter to text
  digits = cnt.length();              //Find the length of the counter numbers

  switch (digits) {
    case 1: //Only one digit
      digit = cnt[0];
      if (counter[mode].value > 0)
        tft.pushImage((TFT_W - SMLNUM_W) / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      else
        tft.pushImage((TFT_W - SMLNUM_W) / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum0_gray); //Draw digit 0 in gray color
      break;
    case 2: //Two digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      break;
    case 3: //Three digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 - SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 + SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      break;
    case 4: //Four digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - SMLNUM_W * 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 - SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Fourth digit in the counter
      digit = cnt[3];
      tft.pushImage(TFT_W / 2 + SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      break;
    case 5: //Five digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 - SMLNUM_W * 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 - SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Fourth digit in the counter
      digit = cnt[3];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 + SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Fifth digit in the counter
      digit = cnt[4];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 + SMLNUM_W * 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      break;
    case 6: //Six digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - SMLNUM_W * 3 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 - SMLNUM_W * 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage(TFT_W / 2 - SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Fourth digit in the counter
      digit = cnt[3];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Fifth digit in the counter
      digit = cnt[4];
      tft.pushImage(TFT_W / 2 + SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      //Sixth digit in the counter
      digit = cnt[5];
      tft.pushImage(TFT_W / 2 + SMLNUM_W * 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum[digit.toInt()]); //Draw digit
      break;
    default:
      // statements
      break;
  }
}

//Draw red color small size digits counter number from images (upto 6 digits on screen)
//x_shift, yshift - pixel shift from the centre of the screen
void draw_counter_sml_red (int x_shift, int y_shift)
{
  String cnt;     //Counter value in text
  String digit;   //Different digits from the counter number
  int digits = 0; //length of the counter number

  cnt = String (counter[mode].value); //Convert digits of the counter to text
  digits = cnt.length();              //Find the length of the counter numbers

  switch (digits) {
    case 1: //Only one digit
      digit = cnt[0];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      break;
    case 2: //Two digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      break;
    case 3: //Three digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 - SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 + SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      break;
    case 4: //Four digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - SMLNUM_W * 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 - SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Fourth digit in the counter
      digit = cnt[3];
      tft.pushImage(TFT_W / 2 + SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      break;
    case 5: //Five digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 - SMLNUM_W * 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 - SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Fourth digit in the counter
      digit = cnt[3];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 + SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Fifth digit in the counter
      digit = cnt[4];
      tft.pushImage((TFT_W - SMLNUM_W) / 2 + SMLNUM_W * 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      break;
    case 6: //Six digits
      //First digit in the counter
      digit = cnt[0];
      tft.pushImage(TFT_W / 2 - SMLNUM_W * 3 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Second digit in the counter
      digit = cnt[1];
      tft.pushImage(TFT_W / 2 - SMLNUM_W * 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Third digit in the counter
      digit = cnt[2];
      tft.pushImage(TFT_W / 2 - SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Fourth digit in the counter
      digit = cnt[3];
      tft.pushImage(TFT_W / 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Fifth digit in the counter
      digit = cnt[4];
      tft.pushImage(TFT_W / 2 + SMLNUM_W + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      //Sixth digit in the counter
      digit = cnt[5];
      tft.pushImage(TFT_W / 2 + SMLNUM_W * 2 + x_shift, (TFT_H - SMLNUM_H) / 2 + y_shift, SMLNUM_W, SMLNUM_H, smlNum_red[digit.toInt()]); //Draw digit
      break;
    default:
      // statements
      break;
  }
}

//Draw current mode
void drawMode ()
{
  //This is possible situation if JSON file was edited
  if (counter[mode].value > counter[mode].max_value)
    counter[mode].value = counter[mode].max_value;

  //Draw according to current mode
  switch (mode) {
    case 0:
      mode7Draw (); //Draw a vishva vajra sprites for 7 counts mode
      break;
    case 1:
      mode21Draw (); //Draw a vajra sprites for 21 counts mode
      break;
    case 2:
      mode108Draw (); //Draw for 108 mala counts mode
      break;
    case 3:
      prostrationsDraw (); //Draw the prostrations counter
      break;
    default:
      tallymodeDraw();
      break;
  }
}

//Load configuration of counter modes from JSON file in SPIFFS
bool loadModes()
{
  // Open file for reading
  File modesFile = SPIFFS.open("/modes.json");
  if (!modesFile) {
    Serial.println("Failed to open modes configuration file.");
    return false;
  }

  //If the file does not exist, then create a new
  if (modesFile.size() == 0)
  {
    Serial.println("Create new config file...");
    saveModes();
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<JSON_BUFFER> jsonBuffer;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(jsonBuffer, modesFile);

  if (error) {
    Serial.print(F("modes config deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

  JsonArray max_value = jsonBuffer["max_value"];
  JsonArray title = jsonBuffer["title"];
  JsonArray show = jsonBuffer["show"];
  for (int i = 0; i < MODES; i++)
  {
    counter[i].max_value = max_value[i];            // Max value
    counter[i].title = title[i].as<String>();       // Mode title
    counter[i].show = show[i];                      // Show/hide current mode
  }

  //Clear buffer before next using
  //jsonBuffer.clear();

  Serial.println("Modes configuration loaded...");
  return true;
}

//Save configuration of counter modes to JSON file in SPIFFS
bool saveModes()
{
  // Open file for writing
  File modesFile = SPIFFS.open("/modes.json", FILE_WRITE);
  if (!modesFile) {
    Serial.println("Failed to open modes configuration file");
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<JSON_BUFFER> jsonBuffer;

  // Set the values in the document
  JsonArray title = jsonBuffer.createNestedArray("title");
  JsonArray max_value = jsonBuffer.createNestedArray("max_value");
  JsonArray show = jsonBuffer.createNestedArray("show");
  for (int i = 0; i < MODES; i++)
  {
    title.add(counter[i].title);
    max_value.add(counter[i].max_value);
    show.add(counter[i].show);
  }

  // Serialize JSON to file
  if (serializeJson(jsonBuffer, modesFile) == 0) {
    Serial.println(F("Failed to write to file modes.json"));
    return false;
  }

  // Close the file
  modesFile.close();

  //Clear buffer before next using
  //jsonBuffer.clear();

  Serial.println("Modes configuration saved...");
  return true;
}

//Load current system settings from JSON file in SPIFFS
bool loadSystem()
{
  // Open file for reading
  File systemFile = SPIFFS.open("/system.json");
  if (!systemFile) {
    Serial.println("Failed to open system.json file");
    return false;
  }

  //If the file does not exist, then create a new
  if (systemFile.size() == 0)
  {
    Serial.println("Create new system settings file...");
    saveSystem();
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<JSON_BUFFER> jsonBuffer;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(jsonBuffer, systemFile);

  if (error) {
    Serial.print(F("system settings deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

  soundON = jsonBuffer["sound"];                    // Audio status (ON/OFF)
  mode = jsonBuffer["mode"];                        // Current mode
  brightness = jsonBuffer["brightness"];            // Current display brightness
  dim_brightness = jsonBuffer["dim_brightness"];    // Dimmed display brightness
  power_save = jsonBuffer["power_save"];            // Power save mode
  dim_time = jsonBuffer["dim_time"];                // Dim the display brightness time (millis) in power save mode
  sleep_time = jsonBuffer["sleep_time"];            // Put the device to sleep time (millis) in power save mode
  prst_low_time = jsonBuffer["prst_low_time"];      // Minimum time of detecting low position in millis
  prst_sup_time = jsonBuffer["prst_sup_time"];      // Minimum time of detecting stand up position in millis
  prst_distance = jsonBuffer["prst_distance"];      // Maximum distance for low position detection in mm
  ssid = jsonBuffer["ssid"].as<String>();           // WiFi AP name
  new_ssid = ssid;
  password = jsonBuffer["password"].as<String>();   // WiFi AP password
  new_password = password;

  //Clear buffer before next using
  //jsonBuffer.clear();

  Serial.println("System settings loaded...");
  return true;
}

//Save current system settings to JSON file in SPIFFS
bool saveSystem()
{
  // Open file for writing
  File systemFile = SPIFFS.open("/system.json", FILE_WRITE);
  if (!systemFile) {
    Serial.println("Failed to open system.json file");
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<JSON_BUFFER> jsonBuffer;

  // Save the values in the document
  jsonBuffer["sound"] = soundON;                    // Audio status (ON/OFF)
  jsonBuffer["mode"] = mode;                        // Current mode
  jsonBuffer["brightness"] = brightness;            // Current display brightness
  jsonBuffer["dim_brightness"] = dim_brightness;    // Dimmed display brightness
  jsonBuffer["power_save"] = power_save;            // Power save mode
  jsonBuffer["dim_time"] = dim_time;                // Dim the display brightness time (millis) in power save mode
  jsonBuffer["sleep_time"] = sleep_time;            // Put the device to sleep time (millis) in power save mode
  jsonBuffer["prst_low_time"] = prst_low_time;      // Minimum time of detecting low position in millis
  jsonBuffer["prst_sup_time"] = prst_sup_time;      // Minimum time of detecting stand up position in millis
  jsonBuffer["prst_distance"] = prst_distance;      // Maximum distance for low position detection in mm
  jsonBuffer["ssid"] = new_ssid;                    // WiFi AP name
  jsonBuffer["password"] = new_password;          // WiFi AP password

  // Serialize JSON to file
  if (serializeJson(jsonBuffer, systemFile) == 0) {
    Serial.println(F("Failed to write to file system.json"));
    return false;
  }

  // Close the file
  systemFile.close();

  //Clear buffer before next using
  //jsonBuffer.clear();

  Serial.println("System settings saved...");
  return true;
}

//Load counters values from JSON file in SPIFFS
bool loadCounters()
{
  // Open file for reading
  File countersFile = SPIFFS.open("/counters.json");
  if (!countersFile) {
    Serial.println("Failed to open counters file");
    return false;
  }

  //If the file does not exist, then create a new
  if (countersFile.size() == 0)
  {
    Serial.println("Create new counters file...");
    saveCounters();
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<JSON_BUFFER> jsonBuffer;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(jsonBuffer, countersFile);

  if (error) {
    Serial.print(F("counters deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

  JsonArray value = jsonBuffer["value"];
  for (int i = 0; i < MODES; i++)
    counter[i].value = value[i];                    // Current counter value

  //Clear buffer before next using
  //jsonBuffer.clear();

  Serial.println("Counters loaded...");
  return true;
}

//Save counters values to JSON file in SPIFFS
bool saveCounters()
{
  // Open file for writing
  File countersFile = SPIFFS.open("/counters.json", FILE_WRITE);
  if (!countersFile) {
    Serial.println("Failed to open counters file");
    return false;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<JSON_BUFFER> jsonBuffer;

  // Set the values in the document
  JsonArray value = jsonBuffer.createNestedArray("value");
  for (int i = 0; i < MODES; i++)
  {
    if (i < 2)
      value.add(0); //Mode 7 and mode 21 always start from 0
    else
      value.add(counter[i].value);
  }

  // Serialize JSON to file
  if (serializeJson(jsonBuffer, countersFile) == 0) {
    Serial.println(F("Failed to write to file counters.json"));
    return false;
  }

  // Close the file
  countersFile.close();

  //Clear buffer before next using
  //jsonBuffer.clear();

  Serial.println("Counters saved...");
  return true;
}

//Draw battery icon
void drawBattery ()
{
  int imgNum = 0;   //Image from battery status icons array
  byte y_shift = 0; //Y coordinate shifting

  if (bat_y_shift)  //Draw battery lower
    y_shift = BAT_Y_SHIFT;

  if (BL.getBatteryVolts() >= MIN_USB_VOL)
  {
    //Draw charging battery icon
    tft.pushImage(TFT_W - (BATTERY_W + 3), 6 + y_shift, BATTERY_W, BATTERY_H, battery[0]);
  }
  else
  {
    batteryLevel = BL.getBatteryChargeLevel();

    if (batteryLevel >= 85) {
      imgNum = 1;
    } else if (batteryLevel < 85 && batteryLevel >= 70 ) {
      imgNum = 2;
    } else if (batteryLevel < 70 && batteryLevel >= 55 ) {
      imgNum = 3;
    } else if (batteryLevel < 55 && batteryLevel >= 40 ) {
      imgNum = 4;
    } else if (batteryLevel < 40 && batteryLevel >= 25 ) {
      imgNum = 5;
    } else if (batteryLevel < 25 && batteryLevel >= 10 ) {
      imgNum = 6;
    } else if (batteryLevel < 10 ) {
      imgNum = 7;
    }

    //Clear space under battery icon
    tft.fillRect (TFT_W - (BATTERY_W + 4), 0 + y_shift, BATTERY_W + 1, 24, TFT_BLACK);
    //Draw battery status icon
    tft.pushImage(TFT_W - (BATTERY_W + 3), 6 + y_shift, BATTERY_W, BATTERY_H, battery[imgNum]);
  }
}

//Battery level check and update battery status icon
void batteryCheck()
{
  int imgNum = 0;

  if (current_time - battery_last_time > BAT_TIME)  //Function work only once in BAT_TIME time
  {
    //Draw battery status icon
    drawBattery ();

    battery_last_time = millis(); //Update battery checking time
  }
}

//**********************************************************************************************************************************
//Distance checking for prostartions counter
//**********************************************************************************************************************************
//Counting logic:
//If the distance was smaller than the prst_distance during time more than prst_low_time, it is considered to be a prostration.
//After distance became longer than prst_distance for more than prst_sup_time, it is considered to be a begining of new prostration.
//**********************************************************************************************************************************
void distanceCheck()
{
  if (mode == PROSTRATIONS) //Works only in prostrations mode
  {
    if (isStart) //Counting START or PAUSE
    {
      power_save_disable = true;  //Remove sleeping possibility in prostrations counter mode

      //Get the sensor data.
      //Take a maximum of two measurements to remove rare isolated errors.
      l0x.rangingTest(&distance, false);  // pass in 'true' to get debug data printout!
      max_distance = distance.RangeMilliMeter;
      l0x.rangingTest(&distance, false);  // second measurement
      if (max_distance < distance.RangeMilliMeter)
        max_distance = distance.RangeMilliMeter;  //max_distance is maximum of two measurements

      if (max_distance < prst_distance)
      {
        tft.fillCircle(230, 38, 5, TFT_GREEN);  //Show symbol of low position
        tft.fillCircle(230, 53, 5, TFT_BLACK);  //Clear last draw
        if (!isEnd)
          tft.fillCircle(230, 53, 5, TFT_GREEN);  //Show symbol of low position detection
      }
      else
      {
        tft.fillCircle(230, 38, 5, LIGHTRED);  //Show symbol of high position
        tft.fillCircle(230, 53, 5, TFT_BLACK);  //Clear last draw
        if (isEnd)
          tft.fillCircle(230, 53, 5, LIGHTRED);  //Show symbol of high position detection
      }

      if (max_distance < prst_distance)
      {
        if (isLow)
        {
          if (current_time - last_msr_time > prst_low_time)
          {
            if (isEnd)  //Is it new prostration?
            {
              isEnd = false;
              counter[mode].value++;
              saveCounters();   //Save current counter value
              playSound (inc);  //Play incrementing sound
              //Counter drawing function
              prostrationsDraw ();

              //Renew last using time for power saving
              current_time = millis (); //take current timer value
              pwr_last_time = millis();
            }
          }
        }
        else
        {
          isLow = true;
          last_msr_time = current_time;
        }
      }
      else
      {
        if (!isLow)
        {
          if (current_time - last_msr_time > prst_sup_time)
            isEnd = true; //Finish prostration
        }
        else
        {
          isLow = false;
          last_msr_time = current_time;
        }
      }
    }
    else
      power_save_disable = false;  //Return sleeping possibility
  }
}

//Pause prostrations recognizing cycle
void pauseProstrations ()
{
  isLow = false;               //Position is UP
  isEnd = true;                //Last prostration is finish
  isStart = false;             //Prostations counting on pause
}

//Play sounds
void playSound (sounds sound)
{
  if (soundON)
  {
    //Play selected sound
    switch (sound) {
      case inc:
        DacAudio.Play(&inc_sound);
        break;
      case dec:
        DacAudio.Play(&dec_sound);
        break;
      case next:
        DacAudio.Play(&next_sound);
        break;
      case previous:
        DacAudio.Play(&previous_sound);
        break;
      case zero:
        DacAudio.Play(&zero_sound);
        break;
      case goal:
        DacAudio.Play(&end_sound);
        break;
      case snd_on:
        DacAudio.Play(&audio_on_sound);
        break;
      default:
        break;
    }
  }
}

//***************************
//Draw the counter mode title
//***************************
void drawTitle ()
{
  String str;
  int title_length_px = tft.textWidth(counter[mode].title); //Current title length in pixels

  //Draw title
  tft.setTextColor(TFT_GOLD, TFT_BLACK);

  if (title_length_px > TFT_W)  //Title size too big for large font
  {
    // Remove previous font parameters from memory to recover RAM
    tft.unloadFont();
    // Load the medium font
    tft.loadFont(TITLE_FONT_MEDIUM);

    title_length_px = tft.textWidth(counter[mode].title); //Current title length in pixels

    if (title_length_px > TFT_W)  //Title size too big for medium font
    {
      // Remove previous font parameters from memory to recover RAM
      tft.unloadFont();
      // Load the small font
      tft.loadFont(TITLE_FONT_SMALL);

      title_length_px = tft.textWidth(counter[mode].title); //Current title length in pixels

      if (title_length_px > TFT_W)  //Title size too big for small font
      {
        //Battery and audio status images shifted
        bat_y_shift = true;
        snd_y_shift = true;

        //Cut title length to screen size and put cutted value to str
        for (int i = countCharacters() - 1; i > 0; i--)
        {
          str = counter[mode].title.substring(0, countCharacterByte (i));
          if (tft.textWidth(str) < TFT_W)
            break;
        }

        tft.drawString(str, 0, TITLE_SMALL_Y); //Draw cutted small font title
      }
      else
      {
        if (title_length_px > TITLE_MAX_PX) //No place for battery and audio status images
        {
          bat_y_shift = true;
          snd_y_shift = true;
        }

        tft.drawString(counter[mode].title, 0, TITLE_SMALL_Y); //Draw small font title
      }
    }
    else  //Title size O.K. for medium font
    {
      if (title_length_px > TITLE_MAX_PX) //No place for battery and audio status images
      {
        bat_y_shift = true;
        snd_y_shift = true;
      }

      tft.drawString(counter[mode].title, 0, TITLE_MEDIUM_Y); //Draw medium font title
    }

    // Restore to default large font
    // Remove the small font parameters from memory to recover RAM
    tft.unloadFont();
    // Load the large font
    tft.loadFont(TITLE_FONT_LARGE);
  }
  else  //Title size O.K. for large font
  {
    if (title_length_px > TITLE_MAX_PX) //No place for battery and audio status images
    {
      bat_y_shift = true;
      snd_y_shift = true;
    }

    tft.drawString(counter[mode].title, 0, 0);  //Draw large font title
  }

  tft.drawFastHLine(0, TITLE_LINE_Y, 240, TFT_GOLD); //Separation line
}

//Count characters in the UTF-8 title
//For the ASCII characters it is one byte per symbol, for the UTF-8 characters it is two (or more) bytes per symbol
int countCharacters ()
{
  int num = 0;
  byte buf;

  for (int i = 0; i < counter[mode].title.length(); i++)
  {
    if (counter[mode].title.charAt(i) > 127)  //This character is UTF-8 code in two bytes (or more)
    {
      buf = counter[mode].title.charAt(i) >> 4; //We move 4 bits to the right, 4 remaining bits are used to determine the size of the symbol in bytes

      //Skipping all the additional bytes of the UTF-8 symbol
      switch (buf) {
        case 12:  //0b00001100
          //Two bytes UTF-8 symbol
          i++;
          break;
        case 14:  //0b00001110
          //Three bytes UTF-8 symbol
          i++;
          i++;
          break;
        case 15:  //0b00001111
          //Four bytes UTF-8 symbol
          i++;
          i++;
          i++;
          break;
        default:
          break;
      }
    }

    //It counted only the first byte of each character
    num++;
  }

  return num;
}

//Finding character byte number for the UTF-8 title
//It is one byte for the ASCII characters and two (or more) bytes for the UTF-8 characters
int countCharacterByte (int num)
{
  int chr_byte = 0;
  byte buf;

  for (int i = 0; i < num; i++)
  {
    if (counter[mode].title.charAt(i) > 127)  //This character is UTF-8 code in two bytes (or more)
    {
      buf = counter[mode].title.charAt(i) >> 4; //We move 4 bits to the right, 4 remaining bits are used to determine the size of the symbol in bytes

      //Determining of additional bytes of the UTF-8 symbol
      switch (buf) {
        case 12:  //0b00001100
          //Two bytes UTF-8 symbol
          chr_byte++;
          break;
        case 14:  //0b00001110
          //Three bytes UTF-8 symbol
          chr_byte++;
          chr_byte++;
          break;
        case 15:  //0b00001111
          //Four bytes UTF-8 symbol
          chr_byte++;
          chr_byte++;
          chr_byte++;
          break;
        default:
          break;
      }
    }

    //First byte of each symbol
    chr_byte++;
  }

  return chr_byte;
}

//Configuration menu
void menuMode()
{
  power_save_disable = true;  //Remove sleeping possibility in menu mode
  bat_y_shift = false;        //Off battery image Y coordinate shifting

  isMenuMode = true;          //Current mode is menu mode
  menuPosition = 1;           //current menu element

  playSound(next);            //play sound

  tft.fillScreen(TFT_BLACK);  //Clear screen

  //Draw menu screen
  modeMenuDraw();
  //Draw battery status icon
  drawBattery ();
}

//Configuration web server
void webServer ()
{
  //Remove sleeping possibility in web server mode
  power_save_disable = true;

  //Convert String to char*
  char myssid[ssid.length() + 1];
  ssid.toCharArray(myssid, ssid.length() + 1);
  char mypass[password.length() + 1];
  password.toCharArray(mypass, password.length() + 1);

  // Create Wi-Fi Access Point (AP) with SSID and password
  Serial.println("Setting AP (Access Point)…");
  if (password != "")
    WiFi.softAP(myssid, mypass);  //Passworded AP (Access Point)
  else
    WiFi.softAP(myssid);  //Open AP (Access Point)

  IPAddress IP = WiFi.softAPIP(); //Receive device IP address

  //IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  myIP = String() + IP[0] + "." + IP[1] + "." + IP[2] + "." + IP[3];  //Convert IP address to String

  //Draw AP mode info screen
  modeAPDraw();
  //Draw battery status icon
  drawBattery ();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/bootstrap.min.css", "text/css");
  });

  // Route to load prostrations.html web page
  server.on("/prostrations.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/prostrations.html", String(), false, processor_prostrations);
  });

  // Route to load system.html web page
  server.on("/system.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/system.html", String(), false, processor_system);
  });

  // Route to load availability.html web page
  server.on("/availability.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/availability.html", String(), false, processor_availability);
  });

  // Route to load tallymodes.html web page
  server.on("/tallymodes.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/tallymodes.html", String(), false, processor_tally);
  });

  // Route for tally counter edit web page tallyedit.html
  server.on("/edit1", HTTP_GET, [](AsyncWebServerRequest * request) {
    tally_edit = 1; //Current editable tally counter
    request->send(SPIFFS, "/tallyedit.html", String(), false, processor_tally_edit);
  });
  server.on("/edit2", HTTP_GET, [](AsyncWebServerRequest * request) {
    tally_edit = 2; //Current editable tally counter
    request->send(SPIFFS, "/tallyedit.html", String(), false, processor_tally_edit);
  });
  server.on("/edit3", HTTP_GET, [](AsyncWebServerRequest * request) {
    tally_edit = 3; //Current editable tally counter
    request->send(SPIFFS, "/tallyedit.html", String(), false, processor_tally_edit);
  });
  server.on("/edit4", HTTP_GET, [](AsyncWebServerRequest * request) {
    tally_edit = 4; //Current editable tally counter
    request->send(SPIFFS, "/tallyedit.html", String(), false, processor_tally_edit);
  });
  server.on("/edit5", HTTP_GET, [](AsyncWebServerRequest * request) {
    tally_edit = 5; //Current editable tally counter
    request->send(SPIFFS, "/tallyedit.html", String(), false, processor_tally_edit);
  });
  server.on("/edit6", HTTP_GET, [](AsyncWebServerRequest * request) {
    tally_edit = 6; //Current editable tally counter
    request->send(SPIFFS, "/tallyedit.html", String(), false, processor_tally_edit);
  });
  server.on("/edit7", HTTP_GET, [](AsyncWebServerRequest * request) {
    tally_edit = 7; //Current editable tally counter
    request->send(SPIFFS, "/tallyedit.html", String(), false, processor_tally_edit);
  });
  server.on("/edit8", HTTP_GET, [](AsyncWebServerRequest * request) {
    tally_edit = 8; //Current editable tally counter
    request->send(SPIFFS, "/tallyedit.html", String(), false, processor_tally_edit);
  });
  server.on("/edit9", HTTP_GET, [](AsyncWebServerRequest * request) {
    tally_edit = 9; //Current editable tally counter
    request->send(SPIFFS, "/tallyedit.html", String(), false, processor_tally_edit);
  });
  server.on("/edit10", HTTP_GET, [](AsyncWebServerRequest * request) {
    tally_edit = 10; //Current editable tally counter
    request->send(SPIFFS, "/tallyedit.html", String(), false, processor_tally_edit);
  });

  // Reset device
  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest * request) {
    ESP.restart();
  });

  //*****************************************
  // GET request to save tally caunter values
  server.on("/savetally", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    String str;

    // GET title value on <ESP_IP>/get?title=<edited value>
    if (request->hasParam("title")) {
      str = request->getParam("title")->value();
      //Only if a new value differs from the old value
      if (counter[tally_edit + 3].title != str)
      {
        counter[tally_edit + 3].title = str;
        isedited[0] = true;
        saveModes(); //Save new title to JSON file
        Serial.println("New title for tally counter " + String(tally_edit) + ": " + counter[tally_edit + 3].title);
      }
    }

    // GET counter value on <ESP_IP>/get?value=<edited value>
    if (request->hasParam("value")) {
      str = request->getParam("value")->value();
      //Convert string to char*
      char vl[str.length() + 1];
      str.toCharArray(vl, str.length() + 1);
      //Only if a new value differs from the old value
      if (counter[tally_edit + 3].value != atol(vl))
      {
        counter[tally_edit + 3].value = atol(vl);
        //Max/min wrong values detection
        if (counter[tally_edit + 3].value > counter[tally_edit + 3].max_value)
          counter[tally_edit + 3].value = counter[tally_edit + 3].max_value;
        if (counter[tally_edit + 3].value < 0)
          counter[tally_edit + 3].value = 0;
        isedited[1] = true;
        saveCounters(); //Save new counter value to JSON file
        Serial.println("New value for tally counter " + String(tally_edit) + ": " + String(counter[tally_edit + 3].value));
      }
    }

    // GET counter max value on <ESP_IP>/get?max_value=<edited value>
    if (request->hasParam("max_value")) {
      str = request->getParam("max_value")->value();
      //Convert string to char*
      char vl2[str.length() + 1];
      str.toCharArray(vl2, str.length() + 1);
      //Only if a new value differs from the old value
      if (counter[tally_edit + 3].max_value != atol(vl2))
      {
        counter[tally_edit + 3].max_value = atol(vl2);
        //Max/min wrong values detection
        if (counter[tally_edit + 3].max_value > 999999)
          counter[tally_edit + 3].max_value = 999999;
        if (counter[tally_edit + 3].max_value <= 0)
          counter[tally_edit + 3].max_value = 1;
        isedited[2] = true;
        saveModes(); //Save new counter max value to JSON file
        Serial.println("New max value for tally counter " + String(tally_edit) + ": " + String(counter[tally_edit + 3].max_value));
      }
    }

    // GET counter availability on <ESP_IP>/get?availability=<edited value>
    if (request->hasParam("availability"))
    {
      str = request->getParam("availability")->value();
      Serial.println(str);
      if (str == "show" && !counter[tally_edit + 3].show) //Only if a new value differs from the old value
      {
        counter[tally_edit + 3].show = true;
        isedited[3] = true;
        saveModes(); //Save new counter availability value to JSON file
        Serial.println("Now tally counter " + String(tally_edit) + " is " + str + ".");
      }
    } //Unchecked checkbox does not return any value
    else if (counter[tally_edit + 3].show) //Only if a new value differs from the old value
    {
      counter[tally_edit + 3].show = false;
      isedited[3] = true;
      saveModes(); //Save new counter availability value to JSON file
      Serial.println("Now tally counter " + String(tally_edit) + " is hidden.");
    }

    //Reupload page
    request->send(SPIFFS, "/tallyedit.html", String(), false, processor_tally_edit);
  });

  //************************************************
  // GET request to save prostrations counter values
  server.on("/saveprostrations", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    String str;

    // GET prostrations counter value on <ESP_IP>/get?value=<edited value>
    if (request->hasParam("value")) {
      str = request->getParam("value")->value();
      //Convert string to char*
      char vl[str.length() + 1];
      str.toCharArray(vl, str.length() + 1);
      //Only if a new value differs from the old value
      if (counter[PROSTRATIONS].value != atol(vl))
      {
        counter[PROSTRATIONS].value = atol(vl);
        //Max/min wrong values detection
        if (counter[PROSTRATIONS].value > counter[PROSTRATIONS].max_value)
          counter[PROSTRATIONS].value = counter[PROSTRATIONS].max_value;
        if (counter[PROSTRATIONS].value < 0)
          counter[PROSTRATIONS].value = 0;
        isedited[0] = true;
        saveCounters(); //Save new prostrations counter value to JSON file
        Serial.println("New value for prostrations counter : " + String(counter[PROSTRATIONS].value));
      }
    }

    // GET counter max value on <ESP_IP>/get?max_value=<edited value>
    if (request->hasParam("max_value")) {
      str = request->getParam("max_value")->value();
      //Convert string to char*
      char vl2[str.length() + 1];
      str.toCharArray(vl2, str.length() + 1);
      //Only if a new value differs from the old value
      if (counter[PROSTRATIONS].max_value != atol(vl2))
      {
        counter[PROSTRATIONS].max_value = atol(vl2);
        //Max/min wrong values detection
        if (counter[PROSTRATIONS].max_value > 999999)
          counter[PROSTRATIONS].max_value = 999999;
        if (counter[PROSTRATIONS].max_value <= 0)
          counter[PROSTRATIONS].max_value = 1;
        isedited[1] = true;
        saveModes(); //Save new prostrations counter max value to JSON file
        Serial.println("New max value for prostrations counter : " + String(counter[PROSTRATIONS].max_value));
      }
    }

    // GET counter availability on <ESP_IP>/get?availability=<edited value>
    if (request->hasParam("availability"))
    {
      str = request->getParam("availability")->value();
      if (str == "show" && !counter[PROSTRATIONS].show) //Only if a new value differs from the old value
      {
        counter[PROSTRATIONS].show = true;
        isedited[2] = true;
        saveModes(); //Save new prostrations counter availability to JSON file
        Serial.println("Now prostrations counter is " + str + ".");
      }
    } //Unchecked checkbox does not return any value
    else if (counter[PROSTRATIONS].show) //Only if a new value differs from the old value
    {
      counter[PROSTRATIONS].show = false;
      isedited[2] = true;
      saveModes(); //Save new prostrations counter availability to JSON file
      Serial.println("Now prostrations counter is hidden.");
    }

    // GET prostrations counter distance value on <ESP_IP>/get?distance=<edited value>
    if (request->hasParam("distance")) {
      str = request->getParam("distance")->value();
      //Only if a new value differs from the old value
      if (prst_distance != str.toInt() * 10)
      {
        prst_distance = str.toInt() * 10;
        //Max/min wrong values detection
        if (prst_distance > 1000)
          prst_distance = 1000;
        if (prst_distance < 100)
          prst_distance = 100;
        isedited[3] = true;
        saveSystem(); //Save new prostrations counter distance value to JSON file
        Serial.println("New detection distance value for prostrations counter : " + String(prst_distance));
      }
    }

    // GET prostrations counter low position detection time value on <ESP_IP>/get?lowtime=<edited value>
    if (request->hasParam("lowtime")) {
      str = request->getParam("lowtime")->value();
      //Only if a new value differs from the old value
      if (prst_low_time != int(str.toFloat() * 1000))
      {
        prst_low_time = int(str.toFloat() * 1000);
        //Max/min wrong values detection
        if (prst_low_time > 5000)
          prst_low_time = 5000;
        if (prst_low_time < 100)
          prst_low_time = 100;
        isedited[4] = true;
        saveSystem(); //Save new prostrations counter low position detection time value to JSON file
        Serial.println("New prostrations counter low position detection time value: " + String(prst_low_time));
      }
    }

    // GET prostrations counter stand up position detection time value on <ESP_IP>/get?suptime=<edited value>
    if (request->hasParam("suptime")) {
      str = request->getParam("suptime")->value();
      //Only if a new value differs from the old value
      if (prst_sup_time != int(str.toFloat() * 1000))
      {
        prst_sup_time = int(str.toFloat() * 1000);
        //Max/min wrong values detection
        if (prst_sup_time > 5000)
          prst_sup_time = 5000;
        if (prst_sup_time < 100)
          prst_sup_time = 100;
        isedited[5] = true;
        saveSystem(); //Save new prostrations counter stand up position detection time value to JSON file
        Serial.println("New prostrations counter stand up position detection time value: " + String(prst_sup_time));
      }
    }

    //Reupload page
    request->send(SPIFFS, "/prostrations.html", String(), false, processor_prostrations);
  });

  //************************************
  // GET request to save system settings
  server.on("/savesystem", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    String str;

    // GET display brightness value on <ESP_IP>/get?brightness=<edited value>
    if (request->hasParam("brightness")) {
      str = request->getParam("brightness")->value();
      //Only if a new value differs from the old value
      if (brightness != round (str.toInt() * 2.55))
      {
        brightness = round (str.toInt() * 2.55);
        //Max/min wrong values detection
        if (brightness > BRIGHTNESS_MAX)
          brightness = BRIGHTNESS_MAX;
        if (brightness < BRIGHTNESS_MIN)
          brightness = BRIGHTNESS_MIN;
        isedited[0] = true;
        saveSystem(); //Save new display brightness value to JSON file
        Serial.println("New value for display brightness: " + String(brightness));
      }
    }

    // GET power save mode status on <ESP_IP>/get?powersave=<edited value>
    if (request->hasParam("powersave"))
    {
      str = request->getParam("powersave")->value();
      if (str == "on" && !power_save) //Only if a new value differs from the old value
      {
        power_save = true;
        isedited[1] = true;
        saveSystem(); //Save power save mode status to JSON file
        Serial.println("Now power save mode is " + str + ".");
      }
    } //Unchecked checkbox does not return any value
    else if (power_save) //Only if a new value differs from the old value
    {
      power_save = false;
      isedited[1] = true;
      saveSystem(); //Save power save mode status to JSON file
      Serial.println("Now power save mode is off.");
    }

    // GET dim the display time on <ESP_IP>/get?dimtime=<edited value>
    if (request->hasParam("dimtime")) {
      str = request->getParam("dimtime")->value();
      //Only if a new value differs from the old value
      if (dim_time != str.toInt() * 1000)
      {
        dim_time = str.toInt() * 1000;
        //Max/min wrong values detection
        if (dim_time > 300000)
          dim_time = 300000;
        if (dim_time < 1000)
          dim_time = 1000;
        isedited[2] = true;
        saveSystem(); //Save new dim the display time to JSON file
        Serial.println("New dim the display time: " + String(dim_time));
      }
    }

    // GET dimmed display brightness value on <ESP_IP>/get?dimbrightness=<edited value>
    if (request->hasParam("dimbrightness")) {
      str = request->getParam("dimbrightness")->value();
      //Only if a new value differs from the old value
      if (dim_brightness != round (str.toInt() * 2.55))
      {
        dim_brightness = round (str.toInt() * 2.55);
        //Max/min wrong values detection
        if (dim_brightness > BRIGHTNESS_MAX)
          dim_brightness = BRIGHTNESS_MAX;
        if (dim_brightness < BRIGHTNESS_MIN)
          dim_brightness = BRIGHTNESS_MIN;
        isedited[3] = true;
        saveSystem(); //Save new dimmed display brightness value to JSON file
        Serial.println("New value for dimmed display brightness: " + String(dim_brightness));
      }
    }

    // GET put the device to sleep time on <ESP_IP>/get?sleeptime=<edited value>
    if (request->hasParam("sleeptime")) {
      str = request->getParam("sleeptime")->value();
      //Only if a new value differs from the old value
      if (sleep_time != str.toInt() * 1000)
      {
        sleep_time = str.toInt() * 1000;
        //Max/min wrong values detection
        if (sleep_time > 300000)
          sleep_time = 300000;
        if (sleep_time < 1000)
          sleep_time = 1000;
        isedited[4] = true;
        saveSystem(); //Save new put the device to sleep time to JSON file
        Serial.println("New put the device to sleep time: " + String(sleep_time));
      }
    }

    // GET audio status on <ESP_IP>/get?audio=<edited value>
    if (request->hasParam("audio"))
    {
      str = request->getParam("audio")->value();
      if (str == "on" && !soundON) //Only if a new value differs from the old value
      {
        soundON = true;
        isedited[5] = true;
        saveSystem(); //Save audio status to JSON file
        Serial.println("Now audio is " + str + ".");
      }
    } //Unchecked checkbox does not return any value
    else if (soundON) //Only if a new value differs from the old value
    {
      soundON = false;
      isedited[5] = true;
      saveSystem(); //Save audio status to JSON file
      Serial.println("Now audio is off.");
    }

    // GET AP name on <ESP_IP>/get?apname=<edited value>
    if (request->hasParam("apname")) {
      str = request->getParam("apname")->value();
      //Only if a new value differs from the old value
      if (String(ssid) != str)
      {
        new_ssid = str;
        isedited[6] = true;
        saveSystem(); //Save new AP name to JSON file
        Serial.println("New Access Point name: " + new_ssid);
      }
    }

    // GET AP password on <ESP_IP>/get?appass=<edited value>
    if (request->hasParam("appass")) {
      str = request->getParam("appass")->value();
      //Only if a new value differs from the old value
      if (String(password) != str)
      {
        new_password = str;
        isedited[7] = true;
        saveSystem(); //Save new AP password to JSON file
        if (new_password !=  "")
          Serial.println("New WiFi Access Point password: " + new_password);
        else
          Serial.println("WiFi Access Point without password");
      }
    }

    //Reupload page
    request->send(SPIFFS, "/system.html", String(), false, processor_system);
  });

  //******************************************
  // GET request to save availability settings
  server.on("/saveavailability", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    String str;

    for (int i = 0; i < MODES; i++)
    {
      // GET counter mode availability on <ESP_IP>/get?counter<number>=<edited value>
      if (request->hasParam("counter" + String(i + 1)))
      {
        str = request->getParam("counter" + String(i + 1))->value();
        if (str == "show" && !counter[i].show) //Only if a new value differs from the old value
        {
          counter[i].show = true;
          isedited[i] = true;
          saveModes(); //Save counter mode availability to JSON file
          Serial.println("Now " + counter[i].title + " is " + str + ".");
        }
      } //Unchecked checkbox does not return any value
      else if (counter[i].show) //Only if a new value differs from the old value
      {
        counter[i].show = false;
        isedited[i] = true;
        saveModes(); //Save counter mode availability to JSON file
        Serial.println("Now " + counter[i].title + " is " + str + ".");
      }
    }

    //Reupload page
    request->send(SPIFFS, "/availability.html", String(), false, processor_availability);
  });


  server.onNotFound(notFound);

  // Start server
  server.begin();
}

//Error 404
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

// Replaces placeholders in root index.html
String processor (const String& var)
{
  //Serial.println(var);

  return String();
}

// Replaces placeholders with tally modes values in tallymodes.html
String processor_tally (const String& var)
{
  //Serial.println(var);

  String str;

  //Tally counters variables
  for (int i = 1; i <= 10; i++)
  {
    str = "TALLY" + String(i) + "TITLE";
    if (var == str)
      return counter[i + 3].title;
    str = "TALLY" + String(i) + "VALUE";
    if (var == str)
      return String(counter[i + 3].value);
    str = "TALLY" + String(i) + "MAX";
    if (var == str)
      return String(counter[i + 3].max_value);
    str = "TALLY" + String(i) + "SHOW";
    if (var == str)
    {
      if (counter[i + 3].show)
        return "<span class=\"show\">Available</span>";
      else
        return "<span class=\"not_show\">Hidden</span>";
    }
  }

  return String();
}

// Replaces placeholders with tally mode values in tallyedit.html
String processor_tally_edit(const String& var)
{
  //Serial.println(var);

  String str;

  str = String(tally_edit); //Current tally counter number for editing

  if (var == "TALLYNUM")
    return str;
  if (var == "TALLYTITLE")
    return counter[tally_edit + 3].title;
  if (var == "TALLYVALUE")
    return String(counter[tally_edit + 3].value);
  if (var == "TALLYMAXVALUE")
    return String(counter[tally_edit + 3].max_value);
  if (var == "CHECKED")
  {
    if (counter[tally_edit + 3].show)
      return "checked";
    else
      return "";
  }

  //Here we add information to the page about the modified values
  for (int i = 0; i < 4; i++)
  {
    //If the value has been changed, the value name will change its color
    if (var == "COLOR" + String(i + 1))
    {
      if (isedited[i])
      {
        isedited[i] = false;    //Reset isedited[]
        return "text-danger";
      }
      else
        return "text-primary";
    }
  }

  return String();
}

// Replaces placeholders with prostration mode values in prostrations.html
String processor_prostrations (const String& var)
{
  //Serial.println(var);

  String str;

  if (var == "PRSTVALUE")
    return String(counter[PROSTRATIONS].value);
  if (var == "PRSTMAXVALUE")
    return String(counter[PROSTRATIONS].max_value);
  if (var == "SHOW")
  {
    if (counter[PROSTRATIONS].show)
      return "checked";
    else
      return "";
  }
  if (var == "PRSTDISTANCE")
    return String(prst_distance / 10);
  if (var == "PRSTLOWTIME")
    return String(float(prst_low_time) / 1000);
  if (var == "PRSTSUPTIME")
    return String(float(prst_sup_time) / 1000);

  //Here we add information to the page about the modified values
  for (int i = 0; i < 6; i++)
  {
    //If the value has been changed, the value name will change its color
    if (var == "COLOR" + String(i + 1))
    {
      if (isedited[i])
      {
        isedited[i] = false;    //Reset isedited[]
        return "text-danger";
      }
      else
        return "text-primary";
    }
  }

  return String();
}

// Replaces placeholders with system settings values in system.html
String processor_system (const String& var)
{
  //Serial.println(var);

  String str;

  if (var == "BRIGHTNESS")
    return String(int(round (brightness / 2.55)));
  if (var == "PSON")
  {
    if (power_save)
      return "checked";
    else
      return "";
  }
  if (var == "DIMTIME")
    return String(dim_time / 1000);
  if (var == "DIMBRIGHTNESS")
    return String(int(round (dim_brightness / 2.55)));
  if (var == "SLEEPTIME")
    return String(sleep_time / 1000);
  if (var == "AUON")
  {
    if (soundON)
      return "checked";
    else
      return "";
  }
  if (var == "APNAME")
    return new_ssid;
  if (var == "APPASS")
    return new_password;

  //Here we add information to the page about the modified values
  for (int i = 0; i < 8; i++)
  {
    //If the value has been changed, the value name will change its color
    if (var == "COLOR" + String(i + 1))
    {
      if (isedited[i])
      {
        isedited[i] = false;    //Reset isedited[]
        return "text-danger";
      }
      else
        return "text-primary";
    }
  }

  return String();
}

// Replaces placeholders with availability values in availability.html
String processor_availability (const String& var)
{
  //Serial.println(var);

  String str;

  for (int i = 0; i < MODES; i++)
  {
    //Current status for the counter (if counter available, checkbox is checked)
    if (var == "SHOW" + String(i + 1))
    {
      if (counter[i].show)
        return "checked";
      else
        return "";
    }
    //If the counter availability status has been changed, the counter name will change its color
    if (var == "COLOR" + String(i + 1))
    {
      if (isedited[i])
      {
        isedited[i] = false;    //Reset isedited[]
        return "text-danger";
      }
      else
        return "text-primary";
    }
  }

  //Titles for tally counter modes
  for (int i = 4; i < MODES; i++)
  {
    if (var == "TALLY" + String(i - 3) + "TITLE")
      return counter[i].title;
  }

  return String();
}

//**************************
//Power Saving mode handling
//**************************
void powerSaving ()
{
  if (power_save)
  {
    if (current_time - pwr_last_time > dim_time)
    {
      if (current_time - pwr_last_time > dim_time + sleep_time)
      {
        if (!power_save_disable)  //Do not sleep in prostration and WiFi modes
          lightSleep (); //Go to light sleep
      }
      else
      {
        //Change display brightness
        changeBrightness (dim_brightness);
      }
    }
    else
    {
      //Change display brightness
      changeBrightness (brightness);
    }
  }
}

//********************************************************************
//Method to print the reason by which ESP32 has been awaken from sleep
//********************************************************************
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

//*********************************************
//LightSleep functionality for Power Saving mode
//*********************************************
void lightSleep ()
{
  //Enable GPIO wake-up function for different pins
  gpio_wakeup_enable(GPIO_NUM_0, GPIO_INTR_LOW_LEVEL); //GPIO_INTR_HIGH_LEVEL or GPIO_INTR_LOW_LEVEL
  gpio_wakeup_enable(GPIO_NUM_2, GPIO_INTR_LOW_LEVEL); //GPIO_INTR_HIGH_LEVEL or GPIO_INTR_LOW_LEVEL
  gpio_wakeup_enable(GPIO_NUM_12, GPIO_INTR_LOW_LEVEL); //GPIO_INTR_HIGH_LEVEL or GPIO_INTR_LOW_LEVEL
  //gpio_wakeup_enable(GPIO_NUM_15, GPIO_INTR_LOW_LEVEL); //GPIO_INTR_HIGH_LEVEL or GPIO_INTR_LOW_LEVEL
  gpio_wakeup_enable(GPIO_NUM_27, GPIO_INTR_LOW_LEVEL); //GPIO_INTR_HIGH_LEVEL or GPIO_INTR_LOW_LEVEL
  gpio_wakeup_enable(GPIO_NUM_35, GPIO_INTR_LOW_LEVEL); //GPIO_INTR_HIGH_LEVEL or GPIO_INTR_LOW_LEVEL

  //Enable wakeup from light sleep using GPIOs
  esp_sleep_enable_gpio_wakeup();

  //Go to light sleep now
  Serial.println("Going to light sleep now...");
  esp_light_sleep_start();

  //Refresh last button press time for power saving
  pwr_last_time = millis();

  Serial.println("Wake up from the light sleep...");
}
