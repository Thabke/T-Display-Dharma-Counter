//****************************************
//* T-Display Dharma Counter header file *
//****************************************

TFT_eSPI tft = TFT_eSPI();  //Display object

//Display size
#define TFT_H 135
#define TFT_W 240

//Buttons pins
//Pins 34 - 39 do not have internal pull-up or pull-down resistors, like the other I/O pins.
//So, for the buttons it is necessary to use other I/O pins or external 10K resistors.
#define BUTTON_INC_PIN    2   //Incrementing button pin
#define BUTTON_DEC_PIN    27  //Decrementing button pin
#define BUTTON_PRS_PIN    12  //Audio Status & Prostration counting START/PAUSE button pin
#define BUTTON_NXT_PIN    0   //Next mode button pin (T-Display build in button)
#define BUTTON_PRV_PIN    35  //Previous mode button pin (T-Display build in button)

//Colors
#define LIGHTRED    0xD165
#define DARKRED     0x6000
#define TRANSPARENT 0x0000    //Black is transparent color for sprite's background 

//Buttons
EasyButton btnINC(BUTTON_INC_PIN);    //Incrementing button
EasyButton btnDEC(BUTTON_DEC_PIN);    //Decrementing button
EasyButton btnNXT(BUTTON_NXT_PIN);    //Next mode button
EasyButton btnPRV(BUTTON_PRV_PIN);    //Previous mode button
EasyButton btnPRS(BUTTON_PRS_PIN);    //Prostration counting START/PAUSE button

#define LONG_PRESS_TIME 700 //700 milliseconds - button long press time, it will be doubled for tally counter modes 

//Power saving regulation
#define BRIGHTNESS_MAX 255
#define BRIGHTNESS_MIN 1
const int pwmFreq = 10000;              //10 kHz
const int pwmResolution = 8;            //8 bit
const int pwmLedChannelTFT = 0;         //LED pin
byte brightness = BRIGHTNESS_MAX;       //Current display brightness
byte dim_brightness = 24;               //Dimmed display brightness
bool power_save = true;                 //Power save mode
bool power_save_disable = false;        //Temporary disable sleeping in some modes
unsigned long  dim_time = 20000;        //Dim the display brightness time (millis) in power save mode
unsigned long  sleep_time = 20000;      //Put the device to sleep time (millis) in power save mode
unsigned long  pwr_last_time = 0;       //Time checking for power saving mode
#define BUTTON_PIN_BITMASK 0x000000004  //Pins 2, ... in hex for deep sleep wake-up detection
RTC_DATA_ATTR int bootCount = 0;


//Counters
#define MODES         14        //Quantity of modes
#define PROSTRATIONS  3         //Prostrations mode
byte mode = 0;                  //current mode, for usual modes and for tally counter modes
byte last_mode = mode;
bool all_hidden = false;
//Tally counter type
typedef struct
{
  String title;
  unsigned long value;
  unsigned long max_value;
  bool show;                    //show or hide current mode
}  tally_counter;
//'counter' variable for all 14 counter modes, these initial values are used only to create initial configuration files.
//If the configuration files already exist, then all values will be loaded from them.
tally_counter counter[MODES] = {
  {"• Mode 7 •", 0, 7, true},
  {"• Mode 21 •", 0, 21, true},
  {"• Mode 108 •", 0, 108, true},
  {"• Prostrations •", 0, 999999, false},
  {"Tally counter 1", 0, 999999, true},
  {"Tally counter 2", 0, 999999, true},
  {"Tally counter 3", 0, 999999, true},
  {"Tally counter 4", 0, 999999, false},
  {"Tally counter 5", 0, 999999, false},
  {"Tally counter 6", 0, 999999, false},
  {"Tally counter 7", 0, 999999, false},
  {"Tally counter 8", 0, 999999, false},
  {"Tally counter 9", 0, 999999, false},
  {"Tally counter 10", 0, 999999, false}
};
unsigned long counter_old = 0;  //previous counter value for all modes

//Beads coordinates for mode 108
const uint8_t beadXY [][2] PROGMEM = {{200, 60}, {199, 68}, {200, 76}, {201, 84}, {209, 83}, {217, 81}, {215, 73}, {216, 65}, {217, 57}, {216, 49}, {217, 41}, {216, 33}, {217, 25}, {215, 17}, {216, 9}, {215, 1}, {223, 2}, {231, 1}, {232, 9}, {233, 17}, {232, 25}, {233, 33}, {232, 41}, {233, 49}, {232, 57}, {233, 65}, {232, 73}, {233, 81}, {232, 89}, {233, 97}, {225, 98}, {217, 97}, {209, 98}, {201, 97}, {193, 98}, {185, 97}, {177, 98}, {169, 97}, {161, 98}, {153, 97}, {145, 98}, {137, 97}, {129, 98}, {121, 97}, {113, 98}, {105, 97}, {97, 98}, {89, 97}, {81, 98}, {73, 97}, {65, 98}, {57, 97}, {49, 98}, {41, 97}, {33, 98}, {25, 97}, {23, 89}, {21, 81}, {13, 81}, {11, 89}, {9, 97}, {1, 98}, {0, 90}, {1, 82}, {0, 74}, {1, 66}, {0, 58}, {1, 50}, {0, 42}, {1, 34}, {0, 26}, {1, 18}, {0, 10}, {1, 2}, {9, 1}, {10, 9}, {11, 17}, {19, 18}, {27, 17}, {26, 9}, {25, 1}, {33, 0}, {41, 1}, {49, 0}, {57, 1}, {65, 0}, {73, 1}, {81, 0}, {89, 1}, {97, 0}, {105, 1}, {113, 0}, {121, 1}, {129, 0}, {137, 1}, {145, 0}, {153, 1}, {161, 0}, {169, 1}, {177, 0}, {185, 1}, {193, 0}, {201, 1}, {200, 9}, {201, 17}, {200, 25}, {201, 33}, {200, 41}};

//SPIFFS
#define FORMAT_SPIFFS_IF_FAILED true

// It is the size of the JSON buffer in bytes.
// Don't forget to change the capacity to match your requirements.
// Use arduinojson.org/v6/assistant to compute the capacity.
#define JSON_BUFFER 1536

//Battery indicator
#define MIN_USB_VOL   4.8
#define ADC_PIN       34
#define CONV_FACTOR   1.8
#define READS         20
#define BAT_TIME      5000                              //Period for battery checking in millis
#define BAT_Y_SHIFT   25                                //Y coordinate shift for battery image in pixels
Pangodream_18650_CL   BL(ADC_PIN, CONV_FACTOR, READS);  //Battery class variable
int batteryLevel;                                       //Curent battery charging level
unsigned long battery_last_time = 0;                    //Last time when battery status was checked
bool bat_y_shift = false;                               //Is battery image Y coordinate shifted?

//*********************
//Prostrations counting
//*********************
// VL53L0X sensor's SCL pin connect to pin 22 and SDA pin connect to pin 21 of T-Display board 
// Instantiate an object for the distance sensor
Adafruit_VL53L0X l0x = Adafruit_VL53L0X();
// Declare variables for storing the sensor data
VL53L0X_RangingMeasurementData_t distance;
int prst_low_time = 300;          //Minimum time of detecting low position in millis
int prst_sup_time = 500;          //Minimum time of detecting stand up position in millis
int prst_distance = 700;          //Maximum distance for low position detection in mm
int max_distance;                 //maximum of two measurements to remove rare isolated errors
unsigned long last_msr_time = 0;  //Last distance measuring time in millis
unsigned long current_time = 0;   //Current checking time in millis
bool isLow = false;               //Last position
bool isEnd = true;                //Is last prostration finished
bool isStart = false;             //Prostations counting pause/start

//************
//Audio output
//************
#define SND_Y_SHIFT 27    //Shifting of Y coordinate for sound status image
#define SND_X_SHIFT -194  //Shifting of X coordinate for sound status image
bool soundON = true; //Audio is ON or OFF
bool snd_y_shift = false; //Is Y coordinate for sound status image shifted?
// create an object of type XT_Wav_Class that is used by
// the dac audio class (below), passing wav data as parameter.
XT_Wav_Class inc_sound(incsnd);
XT_Wav_Class dec_sound(decsnd);
XT_Wav_Class next_sound(nextsnd);
XT_Wav_Class previous_sound(previoussnd);
XT_Wav_Class zero_sound(zerosnd);
XT_Wav_Class end_sound(endsnd);
XT_Wav_Class audio_on_sound(audiosnd);
// Create the main player class object.
// Use GPIO 25, one of the 2 DAC pins and timer 0
XT_DAC_Audio_Class DacAudio(25, 0);
enum sounds {
  inc,
  dec,
  next,
  previous,
  zero,
  goal,
  snd_on
};

//Title
#define TITLE_MAX_PX      192               //Title max length in pixels
#define TITLE_MEDIUM_Y    2                 //Medium font title Y coordinate
#define TITLE_SMALL_Y     6                 //Small font title Y coordinate
#define TITLE_FONT_LARGE  KuraleRegular24   //Big font
#define TITLE_FONT_MEDIUM KuraleRegular18   //Medium font
#define TITLE_FONT_SMALL  KuraleRegular12   //Small font
#define TITLE_LINE_Y      24                //Title separation line Y coordinate

//**********
//Web Server
//**********
bool isAPmode = false;                //is AP mode started
bool isQRmode = false;                //is QR code showing mode is started
int tally_edit;                       //Current editable tally counter
//true if some of values vas edited
bool isedited[MODES] = {false, false, false, false, false, false, false, false, false, false, false, false, false, false};
//AP network credentials
String ssid = "Dharma Counter";  //Access Point name
String password = "";           //Access Point password (NULL = no password)
String new_ssid = ssid;         //New AP name for saving in settings
String new_password = password; //New AP password for saving in settings
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
//IP adress in string format
String myIP;

//******************
//Configuration menu
//******************
#define ELEMENTS  4         //Menu elements quantity
#define ELEMENT_H 25        //Menu elements height
#define ELEMENT_Y 30        //First menu element Y coordinate
bool isMenuMode = false;    //true if the menu mode is started
byte menuPosition = 1;      //current menu element
byte menuPositionPrv = 1;   //previous menu element
