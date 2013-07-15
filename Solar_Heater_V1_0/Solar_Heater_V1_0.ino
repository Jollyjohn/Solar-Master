/******************************************************************
*******************************************************************
** Pool Solar Controller
**
** The prupose of this controller is primarily to watch the pool solary array and switch on the
** pump when the array is capable of geenrating heat.
** The controller also controls the lights in the pool and the garden behind as well as the
** float valve and fill solinoid to control water level in the pool
** The functions are available via a web server so we don't have to keep going to the pool shed
** the check the temperature
*/


#define USE_ETHERNET true
//#define USE_SD true

#ifdef USE_ETHERNET
/******************************************************************
*******************************************************************
** Ethernet interface
** Circuit:
** Ethernet shield attached to pins 10, 11, 12, 13
*/

#include <SPI.h>
#include <Ethernet.h>


// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 100, 20 };
//byte gateway[] = { 192, 168, 100, 1 };
//byte subnet[] = { 255, 255, 255, 0 };
byte pachube[] = { 173, 203, 98, 29 };



/******************************************************************
*******************************************************************
** Pachube client
** Initialize the Ethernet server library
** with the IP address and port you want to use 
*/

EthernetClient client;

// this method makes a HTTP connection to the server:
void sendData(byte thisFeed, float thisData) {
  Serial.println("P1");


  // if there's a successful connection:

  if (client.connect(pachube, 80)) {
    Serial.println("P2");
    // send the HTTP PUT request. 
    // fill in your feed address here:
    client.print("PUT /v2/feeds/41617.csv HTTP/1.1\n");
    client.print("Host: api.pachube.com\n");
    client.print("X-PachubeApiKey: y_eXWNhsWfsaedd6VhbA13e9qVYCa1_ck5VniQ-3uUw\n");
    client.print("Content-Length: ");

    // calculate the length of the feed ID and the sensor reading in bytes:
    int thislength = get:ength(thisFeed);              // Length of the feed ID
    int thisLength = thisLength + 1;                   // Length of the comma
    int thislength = thislength + get:ength(thisFeed); // length of the value
    client.println(thisLength, DEC);

    // last pieces of the HTTP PUT request:
    client.print("Connection: close\n");

    // here's the actual content of the PUT request:
    client.print(thisfeed, DEC);
    client.print(",");
    client.println(thisData, DEC);
    
    Serial.println("PE");
    client.stop();
  }
  else {
    Serial.println("Pachube Failed"):
  }

  
}


// This method calculates the number of digits in the
// sensor reading.  Since each digit of the ASCII decimal
// representation is a byte, the number of digits equals
// the number of bytes:

int getLength(int someValue) {
  // there's at least one byte:
  int digits = 1;
  // continually divide the value by ten, 
  // adding one to the digit count for each
  // time you divide, until you're at 0:
  int dividend = someValue /10;
  while (dividend > 0) {
    dividend = dividend /10;
    digits++;
  }
  // return the number of digits:
  return digits;
}

#endif

/******************************************************************
*******************************************************************
**  LiquidCrystal Display
**
** The circuit:
** LCD RS pin to digital pin 8
** LCD Enable pin to digital pin 9
** LCD D4 pin to digital pin 4
** LCD D5 pin to digital pin 5
** LCD D6 pin to digital pin 6
** LCD D7 pin to digital pin 7
** LCD Backlight to digital pin 3
** LCD R/W pin to ground
** 10K resistor:
** ends to +5V and ground wiper to LCD VO pin (pin 3)
*/

// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);



/******************************************************************
*******************************************************************
**
** SD card datalogger
**
** The circuit:
** analog sensors on analog ins 0, 1, and 2
** SD card attached to SPI bus as follows:
** MOSI - pin 11
** MISO - pin 12
** CLK - pin 13
** CS - pin 4	 
*/

#ifdef USE_SD
#include <SD.h>

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 4;
#endif


/******************************************************************
*******************************************************************
**
** Real Time Clock
** Here are the routines needed for handling the real time clock.
** The 2IC bus for this is on Analogue pins 4 & 5
*/
#include <Wire.h>
#include <RealTimeClockDS1307.h>

#define DS1307_I2C_ADDRESS 0x68  // This is the I2C address
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}
 
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}
 
// 1) Sets the date and time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock
// Assumes you're passing in valid numbers, Probably need to put in checks for valid numbers.

void setDateDs1307()                
{
 
   second = (byte) ((Serial.read() - 48) * 10 + (Serial.read() - 48)); // Use of (byte) type casting and ascii math to achieve result.  
   minute = (byte) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
   hour  = (byte) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
   dayOfWeek = (byte) (Serial.read() - 48);
   dayOfMonth = (byte) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
   month = (byte) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
   year= (byte) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
   Wire.beginTransmission(DS1307_I2C_ADDRESS);
   Wire.write((byte) 0x00);
   Wire.write(decToBcd(second));    // 0 to bit 7 starts the clock
   Wire.write(decToBcd(minute));
   Wire.write(decToBcd(hour));      // If you want 12 hour am/pm you need to set
                                   // bit 6 (also need to change readDateDs1307)
   Wire.write(decToBcd(dayOfWeek));
   Wire.write(decToBcd(dayOfMonth));
   Wire.write(decToBcd(month));
   Wire.write(decToBcd(year));
   Wire.endTransmission();
}


// Gets the date and time from the ds1307 and prints result
void getDateDs1307()
{
  // Reset the register pointer
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write((byte) 0x00);
  Wire.endTransmission();
 
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
 
  // A few of these need masks because certain bits are control bits
  second     = bcdToDec(Wire.read() & 0x7f);
  minute     = bcdToDec(Wire.read());
  hour       = bcdToDec(Wire.read() & 0x3f);  // Need to change this if 12 hour am/pm
  dayOfWeek  = bcdToDec(Wire.read());
  dayOfMonth = bcdToDec(Wire.read());
  month      = bcdToDec(Wire.read());
  year       = bcdToDec(Wire.read());
}



/******************************************************************
*******************************************************************
**
** 1-wire
** Here are the routines needed for handling the 1-wire devices
** The 1-wire bus for this is on Analogue pins 2
**
** 10FDAC0500080026 - The internal case sensor
*/
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 9

#define CASE_SENSOR   "10FDAC0500080026"
#define POOL_SENSOR   "285ADA85030000A0"
#define ARRAY_SENSOR  "285BD985030000D9"
#define FLOW_SENSOR   "-"
#define RETURN_SENSOR "-"

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

int numberOfDevices; // Number of temperature devices found

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address


/******************************************************************
*******************************************************************
** Define the constants used
*/
#define MAX_CASE_TEMP  35    // temperature at which the case fan comes on
#define OK_CASE_TEMP   25    // temperature at which the case fan turns off
#define BUTTON_DRIFT    5    // Drift in the value of the buttons

/*
** Define the I/O pins and what they control
*/
#define POOLFILLVALVE A2 // Pin for the pool fill valve
#define SOLARPUMP A1     // Pin for the Solar Pump (A1)
#define POOLLIGHTS A0    // Pin for the pool lights
#define BUTTONS A3       // Pin for the control buttons
#define CASEFAN A6       // Pin for the fan control
#define LCDBACKLIGHT 3   // Pin for the LCD backlight switch


// Setup the reguired global varibles
boolean pump_on = true;      // Assume the pump is on
boolean fill_on = true;      // Assume fill valve is on
boolean fan_on = true;       // Assume fan is on
boolean pool_lights = true;  // Assume pool lights are on

// Button related variables
boolean U_Button = false;    // Up button pressed
boolean D_Button = false;    // Down button pressed
boolean L_Button = false;    // Left button pressed
boolean R_Button = false;    // Right button pressed
boolean S_Button = false;    // Select button pressed
boolean F_Switch = true;     // Fill Switch in pool active (note - reverse sense)
int button_pressed = 0;      // Used to hold the button value during the keyboard routine

// Temperature related variables
float case_temp = 99.0;          // Assume it's too hot to begin with
float array_temp = 26.0;         // Assume array is cold
float pool_temp = 99.0;           // Assume pool is hot
float flow_temp = 30.0;          // Assume flow is cooler than pool
float return_temp = 33.0;        // Assume return is warmer than array
#define TEMP_ACCURACY 0.5          // Accuracy of the sensors in reading the temperature

// Display mode related variables
int display_mode = 1;        // Controls what is being displayed at the moment
#define MIN_DISPLAY_MODE 1   // Lowest numbered display mode
#define MAX_DISPLAY_MODE 6   // Highest numbered display mode
#define DISP_TEMP   1        // DIsplays the current temperatures and the pump status
#define DISP_TIME   2        // Displays the current time/date from the RTC
#define DISP_OTHER  3        // Test diagnostics (for testing functions as they are added
#define DISP_PUMP 4          // Manual pump mode
#define DISP_LIGHTS 5        // A page that shows the status of the lights
#define DISP_ERROR  6        // A page for displaying errors

// Counters for running the circulation pump in manual mode
int man_pump_set = 0;
int man_pump_run = 420;

//Counters for running the lights in manual mode
int man_light_set = 0;
int man_light_run = 120;

// Counters for the pol fill function
int pool_fill_smooth = 0;          // Used to smooth the pool level switch for when people are in the pool
int pool_fill_rem = 0;             // Tracks how long we have left to fill for. This is a safety stop.
#define POOL_FILL_SMOOTHED 300
#define MAX_POOL_FILL_TIME 30      // Only try to fill the pool for 30 minutes before calling no joy.

// Error conditions
int error_no = 0;
#define NO_ERR 0
#define ERR_FILL 1  // The fill operation timed out.

// General
int old_minute = 0;          // To track the minutes going by for relative timings

/******************************************************************
*******************************************************************
** Now set up all the bits and pieces
*/

void setup()
{
  
  delay(200);
  
  // start serial port:
  Serial.begin(9600);
  
  // set up the LCD
  pinMode(LCDBACKLIGHT, OUTPUT);            // LCD backlight
  digitalWrite(LCDBACKLIGHT, HIGH);         // Turn on the backlight
  
  lcd.begin(16,2);                         // number of columns and rows: 
  lcd.clear();                             // Clear the display
  lcd.print("> Solar Master <");
  delay(2000);
  lcd.clear();
  lcd.print("- Initialising -");           // Display startup message
  

  // Turn on the fan incase we are starting already hot
//  lcd.setCursor(0,1);
//  lcd.print("Fan ...         ");
  pinMode(CASEFAN, OUTPUT);
  digitalWrite(CASEFAN, HIGH);
  fan_on = true;
//  lcd.setCursor(0,1);
//  lcd.print("Fan ... OK      ");
//  delay(1000);
  
  

#ifdef USE_ETHERNET
  // start the Ethernet connection and the server:
//  lcd.setCursor(0,1);
//  lcd.print("Ethernet ...    ");
  Ethernet.begin(mac, ip);
//  lcd.setCursor(0,1);
//  lcd.print("Ethernet ... OK ");
//  delay(1000);
#endif

  
#ifdef USE_SD  
  // Setup the SD card for log storgae
//  lcd.setCursor(0,1);
//  lcd.print("SD card ...     ");
  
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    lcd.setCursor(0,1);
    lcd.print("SD Card failed  ");
  }
//  lcd.setCursor(0,1);
//  lcd.print("SD card ... OK");
//  delay(1000);
#endif  
  
  
  // Start up the 2IC bus to talk to the Real Time Clock
//  lcd.setCursor(0,1);
//  lcd.print("I2C ...         ");
  Wire.begin();
//  lcd.setCursor(0,1);
//  lcd.print("I2C ... OK");
//  delay(1000);
  
  
  
  // Startup the 1-Wire devices
  // Start up the library
  // This find the devices on the bus and builds an array of their addresses
//  lcd.setCursor(0,1);
//  lcd.print("1-Wire          ");

  sensors.begin();
  
  // Grab a count of devices on the wire
  numberOfDevices = sensors.getDeviceCount();
  
   // locate devices on the bus
  Serial.print(PSTR("Locating devices...\n"));
  Serial.print(PSTR("Found "));
  Serial.print(numberOfDevices, DEC);
  Serial.println(PSTR(" devices."));

  // report parasite power requirements
  Serial.print(PSTR("Parasitic power is: ")); 
  if (sensors.isParasitePowerMode()) Serial.println(PSTR("ON"));
  else Serial.println(PSTR("OFF"));
    
  // Loop through each device, and set the resolution
  for(int i=0;i<numberOfDevices; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i))
	{
          // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
	  sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);

          Serial.print(PSTR("Device "));
          Serial.print(i);
          Serial.print(PSTR(" address: "));
          for (uint8_t t = 0; t < 8; t++)
          {
            // zero pad the address if necessary
            if (tempDeviceAddress[t] < 16) Serial.print(PSTR("0"));
            Serial.print(tempDeviceAddress[t], HEX);
          }
          Serial.print(PSTR("\n"));
          
	}
  }
//  lcd.setCursor(0,1);
//  lcd.print("1-Wire ");
//  lcd.print(numberOfDevices);
//  lcd.print(" found");
//  delay(1500);
//  lcd.setCursor(0,1);
//  lcd.print("1-Wire ... OK   ");
//  delay(1000);
  
  
  // Turn off the pool lights
  pinMode (POOLLIGHTS, OUTPUT);
  digitalWrite (POOLLIGHTS, LOW);
  pool_lights = false;
  
  // Ensure everything is stopped
  pinMode (SOLARPUMP, OUTPUT);
  digitalWrite(SOLARPUMP, LOW);    // Turn pump off
  pump_on = false;
  
  pinMode (POOLFILLVALVE, OUTPUT);
  digitalWrite(POOLFILLVALVE, LOW); // Turn pool fill valve off
  fill_on = false;
}



/******************************************************************
*******************************************************************
** The main loop
** This needs to execute around 5 times a second in order to keep the display
** from flickering and to make the buttons reasonably responsive.
*/

void loop()
{
  delay(100);
  
/*
** Get the current time fro mthe RTC
*/
  getDateDs1307();
  
/*
** Gather Temperatures
** Gather the temperature from the following sensors
**
** ROOF_ARRAY    - The temperature sensor on the back of the roof array
** ARRAY_FLOW    - The water flowing to the array
** ARRAY_RETURN  - The water returning from the array
** POOL          - The pool
** CASE_TEMP     - The case containing the control electronics
**
*/

// Read 1-wire temperatures

  sensors.requestTemperatures();                    // Send the command to get temperatures to all devices on the bus
  sensors.getAddress(tempDeviceAddress, 0);         // Get the address of device 0
  case_temp = sensors.getTempC(tempDeviceAddress);  // Assume it's the case temp for the moment
  sensors.getAddress(tempDeviceAddress, 1);         // Get the address of device 1
  pool_temp = sensors.getTempC(tempDeviceAddress);  // Assume it's the pool_temp for the moment
  sensors.getAddress(tempDeviceAddress, 2);         // Get the address of device 1
  return_temp = sensors.getTempC(tempDeviceAddress);  // Assume it's the pool_temp for the moment

/*
** Gather the button state
** Needs to allow for a range of values to compensate for thermal drift
** The pool fill switch is in parallel with the buttons on the front panel. This measns
** the buttons on the front panel will return one of two values depending upon status
** of the pool switch. We need to check for both.
*/
  U_Button = false;
  D_Button = false;
  L_Button = false;
  R_Button = false;
  S_Button = false;
  F_Switch = true;       // Note - reverse sense, true means the water level is good.
  
  button_pressed = analogRead(BUTTONS);
  
  if((button_pressed <= 0 + BUTTON_DRIFT) && (button_pressed >= 0)) R_Button = true;
    
  if(((button_pressed <= 144 + BUTTON_DRIFT) && (button_pressed >= 144 - BUTTON_DRIFT)) ||
     ((button_pressed <= 140 + BUTTON_DRIFT) && (button_pressed >= 140 - BUTTON_DRIFT)) ) U_Button = true;
    
  if(((button_pressed <= 331 + BUTTON_DRIFT) && (button_pressed >= 331 - BUTTON_DRIFT)) ||
     ((button_pressed <= 310 + BUTTON_DRIFT) && (button_pressed >= 310 - BUTTON_DRIFT)) ) D_Button = true; 
    
  if(((button_pressed <= 507 + BUTTON_DRIFT) && (button_pressed >= 507 - BUTTON_DRIFT)) ||
     ((button_pressed <= 461 + BUTTON_DRIFT) && (button_pressed >= 461 - BUTTON_DRIFT)) ) L_Button = true;
    
  if(((button_pressed <= 745 + BUTTON_DRIFT) && (button_pressed >= 745 - BUTTON_DRIFT)) ||
     ((button_pressed <= 650 + BUTTON_DRIFT) && (button_pressed >= 650 - BUTTON_DRIFT)) ) S_Button = true;
    
  if( (button_pressed <= 855 + BUTTON_DRIFT) && (button_pressed >= 855 - BUTTON_DRIFT)) F_Switch = false; 


/*
** Solar Ciculation Pump
** Figure out what the pump should be doing
** We will need to have hysteresis on this to avoid cycling the pump too many times
** We should turn the pump on when the roof array is hotter than the pool and off
** when the return temperature is the same as the pool (to within twice the error
** rating of the temperature sensors.
*/

  if (pump_on == false) {
    if( array_temp > ( pool_temp + 2*TEMP_ACCURACY) ) {
      digitalWrite(SOLARPUMP, HIGH);   // Turn pump on
      pump_on = true;
    }
  } else {
    if( return_temp < (flow_temp + 2*TEMP_ACCURACY) ) {
      digitalWrite(SOLARPUMP, LOW);   // Turn pump off
      pump_on = false;
    }
  }

/*
** Manual Modes
** Anything that should be manually runnable.
*/

/*
** Let the user set a manual run time for the pump
*/

  if( display_mode == DISP_PUMP ) {
    if(R_Button == true) man_pump_run = man_pump_run + 5;
    if(man_pump_run == 999) man_pump_run = 998;
    if(L_Button == true) man_pump_run = man_pump_run - 5;
    if(man_pump_run == 0) man_pump_run =1;
    if(S_Button == true) {
      man_pump_set = man_pump_run;
      digitalWrite(SOLARPUMP, HIGH);   // Turn pump on
      pump_on = true;
    }
  }
 
/*
** Let the user set a manual run tiem for the lights
*/

  if( display_mode == DISP_LIGHTS ) {
    if(R_Button == true) man_light_run = man_light_run + 5;
    if(man_light_run == 999) man_light_run = 998;
    if(L_Button == true) man_light_run = man_light_run - 5;
    if(man_light_run == 0) man_light_run =1;
    if(S_Button == true) {
      man_light_set = man_light_run;
      digitalWrite(POOLLIGHTS, HIGH);   // Turn pump on
      pool_lights = true;
    }
  } 
  
/*
** Check every minute what we should do
*/
  if (old_minute != minute) {
    old_minute = minute;
    
    if (man_pump_set > 0) {                // If we are in manual mode for the pump, have we finished?
      man_pump_set--;
      if(man_pump_set == 0) {
        digitalWrite(SOLARPUMP, LOW);     // Turn pump off
        pump_on = false;
        man_pump_run = 420;               // Go back to default value for manual run minutes
        display_mode = DISP_TEMP;         // Go back to the default display
      }
    }
    
    if (man_light_set > 0) {              // If we are in manual mode for the lights, have we finished?
      man_light_set--;
      if(man_light_set ==0) {
        digitalWrite(POOLLIGHTS, LOW);    // Turn the lights off
        pool_lights = false;
        man_light_run = 120;              // Go back to the default value ofr manually running the lights
        display_mode = DISP_TEMP;         // Go back to the default display
      }
    }
      
    if (pool_fill_rem > 0) {              // If we are filling the pool with water, have we timed out?
      pool_fill_rem--;
      if(pool_fill_rem == 0) {
        digitalWrite(POOLFILLVALVE, LOW); // turn the fill valve off
        fill_on = false;
        error_no = ERR_FILL;              // Flag the error
        display_mode = DISP_ERROR;        // Go to the error display page
      } 
    }
    
#ifdef USE_ETHERNET
/*
** Update Pachube
*/
  Serial.print("Updating pachube\n");
  sendData(pool_temp);
#endif
  
  }

    
/*
** Check for absolute time events
*/
  
// Pool Lights
// Decide if we should turn the pool lights on or off

  if ( (hour > 17) && (hour < 21) ) {
    digitalWrite (POOLLIGHTS, HIGH);  // Turn on the pool lights
    pool_lights = true;
  }
  
  if ( (hour == 21) && (man_light_set ==0) ) {
  digitalWrite (POOLLIGHTS, LOW);    // Turn off the pool lights
  pool_lights = false;
  }   
  
  
  
/*
** Case Fan
** This figures out what the case fan should do to keep the system cool.
** There shouldn't be a need for any hysteresis on this as the thermal mass
** of the case should do the job.
*/

//  NOTE: Due to the factI couldn't get A6 or A7 to work the case fan in permanently on.
//        If you ever get these pins to work you can just alter the fan connector to use the two outside pins
//        Looking from the top, pin 1 is controllable; pin 2 is earth and pin 3 is 12v
//
//  if(case_temp > MAX_CASE_TEMP) digitalWrite(CASEFAN, HIGH);      // Turn fan on
//  
//  if(case_temp < OK_CASE_TEMP) digitalWrite(CASEFAN, LOW);        // Turn pump off
  
  
/*
** Pool Filling
** This checks the pool level sensor and decides if the pool needs water added.
**
** 1. We need to effectively "debounce" the water level sensor to allow for people
**    playing in the pool.
** 2. We need a timer so failure of the pool sensor won't release too much water.
** 3. We should log how long the fill solinoid was on for to track water usage.
*/

  if( F_Switch == false ) {
    pool_fill_smooth++;
    if (pool_fill_smooth == POOL_FILL_SMOOTHED) {
      pool_fill_smooth--;
      digitalWrite(POOLFILLVALVE, HIGH);   // Turn pool fill valve on
      fill_on = true;
      pool_fill_rem = MAX_POOL_FILL_TIME;  // Start a timrer to stop the fill if it goes on too long
    } 
  } else {
      pool_fill_smooth = 0;
      digitalWrite(POOLFILLVALVE, LOW);    // Turn pool fill valve off
      fill_on = false;
  }
  

  
/*
** Update the display mode
*/

  if (U_Button == true) display_mode++;
  if (D_Button == true) display_mode--;
  
  if (display_mode > MAX_DISPLAY_MODE) display_mode = MAX_DISPLAY_MODE;
  if (display_mode < MIN_DISPLAY_MODE) display_mode = MIN_DISPLAY_MODE;
  
/*
** Update the display
*/
  lcd.clear();
  
  switch (display_mode) {
    
    case DISP_TEMP:
        // Display temperatures
        lcd.setCursor(0,0);
        lcd.print("P=");
        lcd.print(pool_temp);
  
        lcd.setCursor(9,0);
        lcd.print("A=");
        lcd.print(array_temp);
  
        lcd.setCursor(0,1);
        lcd.print("F=");
        lcd.print(flow_temp);
  
        lcd.setCursor(9,1);
        lcd.print("R=");
        lcd.print(return_temp);

        break;
        
    case DISP_TIME:
        // Display the current time from the RTC
        
        lcd.setCursor(0,0);
        lcd.print("Time: ");
        if (hour < 10) lcd.print("0");
        lcd.print(hour, DEC);
        lcd.print(":");
        if (minute < 10) lcd.print("0");
        lcd.print(minute, DEC);
        lcd.print(":");
        if (second < 10) lcd.print("0");
        lcd.print(second, DEC);
        
        lcd.setCursor(0,1);
        lcd.print("Date: ");
        if (dayOfMonth <10) lcd.print("0");
        lcd.print(dayOfMonth, DEC);
        lcd.print("/");
        if (month < 10) lcd.print("0");
        lcd.print(month, DEC);
        lcd.print("/");
        if (year <10) lcd.print("0");     // yeah, right. Like that's ever gonna happen!
        lcd.print(year, DEC);
        break;
        
        
    case DISP_OTHER:
        // Diagnostic display
        lcd.setCursor(0,0);
        lcd.print("C=");
        lcd.print(case_temp);
        
        if(F_Switch == true) {
          lcd.setCursor(0,1);
          lcd.print("H2O=OK");
        } else {
          lcd.setCursor(0,1);
          lcd.print("H2O=Low");
        }
     
        if(pump_on == true) {
          if(R_Button == true) {
            digitalWrite(SOLARPUMP, LOW);    // Manually turn pump off
            pump_on = false;
          }
          lcd.setCursor(8,0);
          lcd.print("Pump=On");
        } else {
          if(L_Button == true) {
            digitalWrite(SOLARPUMP, HIGH);   // Mannualy turn pump on
            pump_on = true;
          }
          lcd.setCursor(8,0);
          lcd.print("Pump=Off");
        }
               
        if(fill_on == true) {
          lcd.setCursor(8,1);
          lcd.print("Fill=On");
        } else {
          lcd.setCursor(8,1);
          lcd.print("Fill=Off");
        }
        break;
        
     case DISP_PUMP:
        // Manual pump timer mode
        lcd.setCursor(0,0);
        lcd.print("Timer: Pump ");
        if(pump_on == true) {
          lcd.print("On");
        } else {
          lcd.print("Off");
        }
        lcd.setCursor(0,1);
        lcd.print("Set ");
        lcd.print(man_pump_run);
        lcd.setCursor(8,1);
        lcd.print("Left ");
        lcd.print(man_pump_set);
        break;
        
     case DISP_LIGHTS:
        // Manual light timer mode
        lcd.setCursor(0,0);
        lcd.print("Timer: Light ");
        if(pool_lights == true) {
          lcd.print("On");
        } else {
          lcd.print("Off");
        }
        lcd.setCursor(0,1);
        lcd.print("Set ");
        lcd.print(man_light_run);
        lcd.setCursor(8,1);
        lcd.print("Rem ");
        lcd.print(man_light_set);
        break;

        
     case DISP_ERROR:
        // Something went wrong....
        switch (error_no) {
          case ERR_FILL:
             // The fill operation failed
             lcd.setCursor(0,0);
             lcd.print("Fill Operation");
             lcd.print("    failed");
             break;
             
          default:
             lcd.setCursor(0,0);
             lcd.print("    System OK");
             break;
        }
        break;
             
        
     default:
        // Invalid display mode
        lcd.setCursor(0,0);
        lcd.print("Invalid Mode");
        break;
        
  }
}

