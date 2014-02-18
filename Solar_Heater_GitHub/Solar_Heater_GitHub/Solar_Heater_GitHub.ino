/******************************************************************
 *******************************************************************
 ** Pool Solar Controller
 **
 ** The prupose of this controller is primarily to watch the pool solary array and switch on the
 ** pump when the array is capable of geenrating heat.
 ** The controller also controls a float valve and fill solinoid to control water level in the pool
 ** The metrics are available via Pachube so we don't have to keep going to the pool shed
 ** the check the temperature
 **
 ** V4.0  - Significant cleanup.
 **       - Added support for reset of of fill error condition
 **       - Cleaned up time based counters to work in minutes (not execution cycles)
 **       - Agregated all minute resolution checks into one block
 **       - Added support for regular flushing fo the array in winter
 **       - Added protection mode for array overheating
 **
 ** V3.2  - Changed board to optiboot to find additional space
 **
 ** V3.1  - Added MyTime functionality
 **       - Added manual fill screen
 **
 ** V3.0  - migrated to GitHub
 */

#define PROJECT_NAME "Solar Master 4.0"

#include <stdlib.h>

/******************************************************************
 *******************************************************************
 ** Define the variables and constants used
 */

/*
** Definitions around max temperautres for the pump shed
 */
#define MAX_SHED_TEMP  25      // temperature at which the case fan comes on
#define OK_SHED_TEMP   23      // temperature at which the case fan turns off

/*
** Define the I/O pins and what they control
 */
#define SHEDFAN       A0       // Pin for the fan control
#define SOLARPUMP     A1       // Pin for the Solar Pump (A1)
#define POOLFILLVALVE A2       // Pin for the pool fill valve
#define BUTTONS       A3       // Pin for the control buttons
//
#define LCDBACKLIGHT   3       // Pin for the LCD backlight switch


// Setup the reguired global varibles
boolean pump_on = true;        // Assume the pump is on
int     fill_on = true;        // Assume fill valve is on (type int as error needs to be a value less than 0)
boolean fan_on  = true;        // Assume fan is on
boolean pool_lights = true;    // Assume pool lights are on


// Keypad button related variables
boolean U_Button = false;      // Up button not pressed
boolean D_Button = false;      // Down button not pressed
boolean L_Button = false;      // Left button not pressed
boolean R_Button = false;      // Right button not pressed
boolean S_Button = false;      // Select button not pressed
boolean F_Switch = true;       // Fill Switch in pool active, pool level OK (note - reverse sense)
int button_pressed = 0;        // Used to hold the button value during the keyboard routine
#define BUTTON_DRIFT    5      // Drift in the value of the buttons


// Temperature related variables
float shed_temp   = 99.0;      // Assume it's too hot to begin with
float array_temp  = 10.0;      // Assume array is cold
float pool_temp   = 99.0;      // Assume pool is hot
float flow_temp   = 30.0;      // Assume flow is cooler than pool
float return_temp = 10.0;      // Assume return is warmer than array

#define TEMP_MAX_ERR  79       // Above this indicates sesnor read error
#define TEMP_MIN_ERR  1        // Below this indicates sesore read error
#define ON_HYSTERESIS  3       // Array must be this above the pool to turn on
#define OFF_HYSTERESIS 1       // Array must be this above the poo to stay on


// Display mode related variables
byte display_mode = 1;         // Controls what is being displayed at the moment
#define MIN_DISPLAY_MODE 1     // Lowest numbered display mode
#define DISP_TEMP    1          // DIsplays the current temperatures and the pump status
#define DISP_TIME    2          // Displays the current time/date from the RTC
#define DISP_OTHER   3          // Test diagnostics (for testing functions as they are added
#define DISP_PUMP    4          // Manual pump mode
#define DISP_ERROR   5          // A page for displaying errors
#define MAX_DISPLAY_MODE 5      // Highest numbered display mode

// Counters for running the circulation pump in manual mode
int man_pump_run = 0;
#define DEFAULT_RUN_TIME 420   // Default run time in minutes for the pump in manual mode
int man_pump_set = DEFAULT_RUN_TIME;   

//Pump hold-off timer to stop the pump from getting thrashed
#define PUMP_HOLD_OFF  5       // Minutes between the pump changing state
byte pump_hold_timer = 0;      // The pump can start straight away

// Daily flush of the water in the array to keep it fresh
#define DAILY_FLUSH_HOUR  12   // At this time flush the array with clean water
#define DAILY_FLUSH_TIME  10   // Flush the array for this many minutes each day

// Counters for the pool fill function
int pool_fill_smooth  = 0;      // Used to smooth the pool level switch for when people are in the pool
int pool_fill_rem     = -1;     // Tracks how long we have left to fill for. This is a safety stop.
#define POOL_FILL_SMOOTHED 30   // Number of minutes required to debounce the pool fill float valve
#define MAX_POOL_FILL_TIME 60   // Only try to fill the pool for two hours before calling no joy.
                                // (The slow fill rate is so filling won't affect the house water preassure)
// Display dimming
byte backlight_timer = 0;      // Counts the number of execution loops between key presses
#define BACKLIGHT_TIME 200     // Execution loops before the display dims

// Error conditions
#define NO_ERR 0
byte error_no = NO_ERR;        // Used to hold the error code when an error is encountered
#define ERR_FILL    1          // The fill operation timed out.
#define ERR_PACHUBE 2          // The connection to Pachube wouldn't open
#define ERR_SENSORS 3          // Some of the 1-wire sensors couldn't be found
#define ERR_READ    4          // A sesnor returned a bad value

// General time/date related variables
byte old_minute = 0;           // To track the minutes going by for relative timings
byte old_hour = 0;             // To track the hours going by for relative timings
#define January   1
#define Feburary  2
#define March     3
#define April     4
#define May       5
#define June      6
#define July      7
#define August    8
#define September 9
#define October   10
#define Novemeber 11
#define December  12

/*
** This is the correct way around for the Southern Hemishpere
*/
#define SEASON_END    March
#define SEASON_START  July


/******************************************************************
 *******************************************************************
 ** Ethernet interface
 ** ------------------
 ** Ethernet shield attached to pins 10, 11, 12, 13
 */

#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   // Our MAC address
EthernetClient client;                    // We are a client meaning we call them, we don't look for connections

// Pachube data
#define PACHUBE_FEED "41617"
#define PACHUBE_KEY  "y_eXWNhsWfsaedd6VhbA13e9qVYCa1_ck5VniQ-3uUw"
#define PACHUBE_SERV "api.cosm.com"
int pachube_port = 80;                    // Normally 80; can be changed for debugging
char pachube_data[10];                    // 10 char maximum on any single value point send to PACHUBE, including channel number and comma

// Definitions around contacting the mytime service
#define MYTIME_SERV "mytime.frater-baird.com"                 // Address of MyTime server we want to use
#define MYTIME_PORT 3001                                      // Port to contact MyTime server on
#define CONNECTION_TIMEOUT 5000                               // Number of while loops to wait for data
#define SYNC_HOUR  4                                          // What hour of the day do we sync the time


/******************************************************************
 *******************************************************************
 ** LiquidCrystal Display
 ** ---------------------
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

#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);


/******************************************************************
 *******************************************************************
 **
 ** Real Time Clock
 ** ---------------
 ** Here are the items needed for handling the real time clock.
 ** The 2IC bus for this is on Analogue pins 4 & 5
 */
#include <Wire.h>
#include <RealTimeClockDS1307.h>

#define DS1307_I2C_ADDRESS 0x68                                 // This is the I2C address for the RTC
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;



/******************************************************************
 *******************************************************************
 **
 ** 1-wire
 ** ------
 ** Here are the items needed for handling the 1-wire devices
 ** The 1-wire bus for this is on Analogue pins 2
 **
 */
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2              // Data wire is plugged into pin D2 on the Arduino
#define TEMPERATURE_PRECISION 10    // 10 bits of precision for temperature sampling is enough (makes the ADC process faster)

DeviceAddress shed_sensor =   { 0x10, 0xFD, 0xAC, 0x05, 0x00, 0x08, 0x00, 0x26 };
DeviceAddress pool_sensor =   { 0x28, 0x38, 0x7C, 0x60, 0x03, 0x00, 0x00, 0x6A };
DeviceAddress flow_sensor =   { 0x28, 0x5B, 0xD9, 0x85, 0x03, 0x00, 0x00, 0xD9 };
DeviceAddress array_sensor =  { 0x28, 0xC0, 0xC7, 0x85, 0x03, 0x00, 0x00, 0x4E };
DeviceAddress return_sensor = { 0x28, 0x31, 0xC9, 0x85, 0x03, 0x00, 0x00, 0xA9 };

// 1-wire sensors
#define NUM_OF_SENSORS 5       // Number of sensors the system expects to see


OneWire oneWire(ONE_WIRE_BUS);        // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature. 
int numberOfDevices;                  // Number of temperature devices found
DeviceAddress tempDeviceAddress;      // We'll use this variable to store a found device address
byte sensor_errors = 0;               // Counter to track the number fo sensor errors in each pass



/******************************************************************
 *******************************************************************
 ** Now set up all the bits and pieces
 */

void setup()
{

  delay(200);                              // Ensure the Ethernet chip has powered up fully

  // set up the LCD
  pinMode(LCDBACKLIGHT, OUTPUT);           // LCD backlight
  digitalWrite(LCDBACKLIGHT, HIGH);        // Turn on the backlight

  lcd.begin(16,2);                         // number of columns and rows: 
  lcd.clear();                             // Clear the display
  lcd.print(PROJECT_NAME);                 // Who are we?
  lcd.setCursor(0,1);
  lcd.print("- Initialising -");           // Display startup message


  // start the Ethernet connection and the server:
  lcd.setCursor(0,1);
  lcd.print("  - Ethernet -  ");           // Display startup message
  Ethernet.begin(mac);


  // Start up the 2IC bus to talk to the Real Time Clock
  lcd.setCursor(0,1);
  lcd.print(" -   2IC Bus  - ");           // Display startup message
  Wire.begin();


  // Startup the 1-Wire devices
  lcd.setCursor(0,1);
  lcd.print(" -   OneWire  - ");           // Display startup message
  sensors.begin();

  // Grab a count of devices on the wire
  numberOfDevices = sensors.getDeviceCount();
  if ( numberOfDevices != NUM_OF_SENSORS )
  {
    error_no = ERR_SENSORS;              // Flag the error
    display_mode = DISP_ERROR;           // Display the error display page
  }

  // Loop through each device, and set the resolution
  for(byte i=0;i<numberOfDevices; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i))
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION); // set the resolution to TEMPERATURE_PRECISION bits
  }

  // Turn on the fan incase we are starting already hot
  pinMode(SHEDFAN, OUTPUT);
  digitalWrite(SHEDFAN, HIGH);
  fan_on = true;

  // Ensure the pump is stopped
  pinMode (SOLARPUMP, OUTPUT);
  digitalWrite(SOLARPUMP, LOW);    // Turn pump off
  pump_on = false;

  // Setup the fill valve
  pinMode (POOLFILLVALVE, OUTPUT);   // Make the output for the fill valve an output
  digitalWrite(POOLFILLVALVE, LOW);  // Turn the fill valve off
  fill_on = false;

  // Make sure the bit for the buttons is readable
  pinMode (BUTTONS, INPUT);

  delay(100);    // Wait for the Ethernet controller to fully initialise

  // Get the correct time:
  lcd.setCursor(0,2);
  lcd.print(" -   MyTime  - ");
  if (client.connect(MYTIME_SERV, MYTIME_PORT)) {   // Set the realtime clock from the mytimed service 
    client.println(PROJECT_NAME);
    int connect_timeout = CONNECTION_TIMEOUT;

    while ((client.available() == false) && (connect_timeout != 0))
    { 
      connect_timeout--;                                                       // If we wait too long just give up and don't change anything
    }

    if (connect_timeout != 0) {
      second =      (byte) ((client.read() - 48) * 10 + (client.read() - 48)); // Use of (byte) type casting and ascii math to achieve result.  
      minute =      (byte) ((client.read() - 48) * 10 + (client.read() - 48));
      hour  =       (byte) ((client.read() - 48) * 10 + (client.read() - 48));
      dayOfWeek =   (byte) ( client.read() - 48);
      dayOfMonth =  (byte) ((client.read() - 48) *10 +  (client.read() - 48));
      month =       (byte) ((client.read() - 48) *10 +  (client.read() - 48));
      year=         (byte) ((client.read() - 48) *10 +  (client.read() - 48));

      client.stop();

      Wire.beginTransmission(DS1307_I2C_ADDRESS);
      Wire.write(decToBcd(0x00));
      Wire.write(decToBcd(second));      // 0 to bit 7 starts the clock
      Wire.write(decToBcd(minute));
      Wire.write(decToBcd(hour));        // If you want 12 hour am/pm you need to set bit 6 (also need to change readDateDs1307)
      Wire.write(decToBcd(dayOfWeek));
      Wire.write(decToBcd(dayOfMonth));
      Wire.write(decToBcd(month));
      Wire.write(decToBcd(year));
      Wire.endTransmission();
    }
  }

  lcd.setCursor(0,2);
  lcd.print(" -    Done    - ");
}





/******************************************************************
 *******************************************************************
 ** The main loop
 ** This needs to execute as frequently as possible in order to keep the display
 ** from flickering and to make the buttons reasonably responsive.
 */

void loop()
{

/*
** Gather the various bits of data we need to make decissions
*/

/*
** Gather the current time from the RTC
*/
  getDateDs1307();


/*
** Gather Temperatures from the 1wire sensors
**
*/
  sensors.requestTemperatures();                     // Send the command to get temperatures to all devices on the bus
  shed_temp   = sensors.getTempC(shed_sensor);       // Get the shed temparture (actually it's in the case, but close enough)
  array_temp  = sensors.getTempC(array_sensor);      // Get the array temperature from the roof 
  pool_temp   = sensors.getTempC(pool_sensor);       // Get the pool temparture
  return_temp = sensors.getTempC(return_sensor);     // Get the temperature of the returning water
  flow_temp   = sensors.getTempC(flow_sensor);       // Get the temperature of the flow water


/*
** Gather the button state from the keypad
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

  // Check for the Right button
  if((button_pressed <= 0 + BUTTON_DRIFT) && (button_pressed >= 0)) R_Button = true;

  // Check for the Up button
  if(((button_pressed <= 144 + BUTTON_DRIFT) && (button_pressed >= 144 - BUTTON_DRIFT)) ||
    ((button_pressed <= 140 + BUTTON_DRIFT) && (button_pressed >= 140 - BUTTON_DRIFT)) ) { 
    U_Button = true; 
    backlight_timer = 0; 
  }

  // Check for the Down button  
  if(((button_pressed <= 331 + BUTTON_DRIFT) && (button_pressed >= 331 - BUTTON_DRIFT)) ||
    ((button_pressed <= 310 + BUTTON_DRIFT) && (button_pressed >= 310 - BUTTON_DRIFT)) ) { 
    D_Button = true; 
    backlight_timer = 0; 
  }

  // Check for the Left button
  if(((button_pressed <= 507 + BUTTON_DRIFT) && (button_pressed >= 507 - BUTTON_DRIFT)) ||
    ((button_pressed <= 461 + BUTTON_DRIFT) && (button_pressed >= 461 - BUTTON_DRIFT)) ) { 
    L_Button = true; 
    backlight_timer = 0; 
  }

  // Check for the Select button
  if(((button_pressed <= 745 + BUTTON_DRIFT) && (button_pressed >= 745 - BUTTON_DRIFT)) ||
    ((button_pressed <= 650 + BUTTON_DRIFT) && (button_pressed >= 650 - BUTTON_DRIFT)) ) { 
    S_Button = true; 
    backlight_timer = 0; 
  }

  // Check for the Fill input (not a button, but the float switch in the pool)
  if((button_pressed <= 865 + BUTTON_DRIFT) && (button_pressed >= 840 - BUTTON_DRIFT)) F_Switch = false; 


/*
** Check for errors in the inputs gathered
*/

  // Check that we could read the shed temperature
  if ((shed_temp < TEMP_MIN_ERR) || (shed_temp > TEMP_MAX_ERR)) { 
    sensor_errors++; 
    error_no = ERR_READ; 
  } 
  
  // Check that we could read it array temperature
  if ((array_temp < TEMP_MIN_ERR) || (array_temp > TEMP_MAX_ERR)) { 
    sensor_errors++; 
    error_no = ERR_READ; 
  }

  // Check that we could read it pool temperature
  if ((pool_temp < TEMP_MIN_ERR) || (pool_temp > TEMP_MAX_ERR)) { 
    sensor_errors++; 
    error_no = ERR_READ; 
  }

  // Check that we could read it return temperature
  if ((return_temp < TEMP_MIN_ERR) || (return_temp > TEMP_MAX_ERR)) { 
    sensor_errors++; 
    error_no = ERR_READ; 
  } 
 
  // Check that we could read it flow temperature
  if ((flow_temp < TEMP_MIN_ERR) || (flow_temp > TEMP_MAX_ERR)) { 
    sensor_errors++; 
    error_no = ERR_READ; 
  }  
 
 
  // Check if no buttons have been pressed for a while. If so, turn out the backlight
  if ( backlight_timer == BACKLIGHT_TIME ) {
    digitalWrite(LCDBACKLIGHT, LOW);           // Turn off the backlight
  } 
  else {
    backlight_timer++;
    digitalWrite(LCDBACKLIGHT, HIGH);          // Turn on the backlight
  }



/**
** Regularly timed events
** Handle both relative and absolute times for starting things.
*/

/*
** We should only actually use the heating routines through the warmer months
*/
  if ( (month < SEASON_END) || (month > SEASON_START) ) {

/*
** Solar Ciculation Pump
** Figure out what the pump should be doing
** We should turn the pump on when the roof array is hotter than the pool and off
** when the return temperature is the same as the pool. The hysteresis factors are
** to allow for inefficencies in the system and inaccuracies in the temperature probes
*/

    if ((sensor_errors == 0) && (pump_hold_timer == 0)) {
      if (pump_on == false) {
        if( array_temp > ( pool_temp + ON_HYSTERESIS) ) {
          digitalWrite(SOLARPUMP, HIGH);   // Turn pump on
          pump_on = true;
          pump_hold_timer = PUMP_HOLD_OFF;
        }
      } 
      else {
        if( return_temp < (flow_temp + OFF_HYSTERESIS) ) {
          digitalWrite(SOLARPUMP, LOW);   // Turn pump off
          pump_on = false;
          pump_hold_timer = PUMP_HOLD_OFF;
        }
      }
    } 
    else {
      digitalWrite(SOLARPUMP, LOW);         // If the sensors give errors, turn the pump off. The error will be logged in COSM
      pump_on = false;
    }
  }  // End of summertime routines


/*
** All year round routines
*/

/*
** Every day at a set time we flush the solar system. This is to ensure that the solar system stays relatively clean
** even through winter.
*/
  if ((hour == DAILY_FLUSH_HOUR) && (minute == 0)) {      // If it is time to flush the array, turn the pump on
    digitalWrite(SOLARPUMP, HIGH);
    pump_on = true;
    man_pump_run = DAILY_FLUSH_TIME;
  }

/*
** If the solar array gets to over 50c turn on the pump for a bit to reduce the temperature and extend the life of the array
*/
  if( array_temp > 50 ) {
    digitalWrite(SOLARPUMP, HIGH);   // Turn pump on
    pump_on = true;
    man_pump_run = PUMP_HOLD_OFF;
  }


/*
** Every night at 1am clear the filling error
** This will suspend filling if it was active, but it will resume once the pool smoothing period is over.
*/
 if((hour == 1) && (minute == 0)) {               // If we had timed out before the pool was full, reset the error condition
    digitalWrite(POOLFILLVALVE, LOW);             // Turn pool fill valve off in case it was on
    fill_on = false;                              // The valve is now off
    pool_fill_rem = 0;                            // No filling, but everything OK
    error_no = NO_ERR;                            // CLear the error flag
    display_mode = DISP_TEMP;                     // Go to the temperature display page
  }

/*
** Every day at a set time we sync to the network time.
** Doing it everyday really isn't needed, but it does no harm and ensures daylight savings time is caught corectly.
*/
  if ((hour == SYNC_HOUR) && (minute == 0)){
    if (client.connect(MYTIME_SERV, MYTIME_PORT)) { 

      client.println(PROJECT_NAME);

      int connect_timeout = CONNECTION_TIMEOUT;

      while ((client.available() == false) && (connect_timeout != 0))
      { 
        connect_timeout--; 
      }                                                   // If we wait too long just give up and don't change anything

      if (connect_timeout != 0) {
        second =      (byte) ((client.read() - 48) * 10 + (client.read() - 48)); // Use of (byte) type casting and ascii math to achieve result.  
        minute =      (byte) ((client.read() - 48) * 10 + (client.read() - 48));
        hour  =       (byte) ((client.read() - 48) * 10 + (client.read() - 48));
        dayOfWeek =   (byte) ( client.read() - 48);
        dayOfMonth =  (byte) ((client.read() - 48) *10 +  (client.read() - 48));
        month =       (byte) ((client.read() - 48) *10 +  (client.read() - 48));
        year=         (byte) ((client.read() - 48) *10 +  (client.read() - 48));

        client.stop();

        Wire.beginTransmission(DS1307_I2C_ADDRESS);
        Wire.write(decToBcd(0x00));
        Wire.write(decToBcd(second));      // 0 to bit 7 starts the clock
        Wire.write(decToBcd(minute));
        Wire.write(decToBcd(hour));        // If you want 12 hour am/pm you need to set bit 6 (also need to change readDateDs1307)
        Wire.write(decToBcd(dayOfWeek));
        Wire.write(decToBcd(dayOfMonth));
        Wire.write(decToBcd(month));
        Wire.write(decToBcd(year));
        Wire.endTransmission();

        // Once we get the time wait one minute so the minute counter can tick over. This means we will try every second for the one
        // minute of 04:00 until we get the time, at which point we sleep for one minute and continue. This allows 60 retried per day
        // to maximise the chance of success, but also that we stop once we are successful to minimise the load on the MyTime server
        // as it's only single threaded and others may also want to get the time.
        delay(60000);
      }
    }
  }


/*
** Once per minute functions
*/
  if (old_minute != minute) {
    old_minute = minute;    

    /*
    ** Update Pachube with the current pool statistics
    ** At present we only send the latest set of readings. It may be better to send averaged readings where sensor errors are excluded
    ** from the averaging process to give a smoother value. Also, if a sensor failed to read on the pass that is sent to Pachube the value is
    ** unreliable. Averaging would also fix this.
    */
    if (client.connect(PACHUBE_SERV, pachube_port)) {

      // Send the HTTP PUT request. This is a Pachube V2 format request. The .csv tells Pachube what format we are using 
      client.print("PUT /v2/feeds/");
      client.print(PACHUBE_FEED);
      client.println(".csv HTTP/1.1");

      // Send the host command
      client.println("Host: api.cosm.com");

      // Prove we are authorised to update this feed
      client.print("X-PachubeApiKey: ");
      client.println(PACHUBE_KEY);

      // Send the length of the string being sent
      client.println("Content-Length: 88");

      // last pieces of the HTTP PUT request:
      client.println("Connection: close");

      // Don't forgte the empty line between the header and the data!!
      client.println();     

      sendData(0,pool_temp);
      sendData(1,array_temp);
      sendData(2,return_temp);
      sendData(3,flow_temp);
      if(F_Switch == true) {
        sendData(4,0);
      } 
      else {
        sendData(4,1);
      }
      if(fill_on == true) {
        sendData(5,1);
      } 
      else {
        sendData(5,0);
      }
      sendData(6,shed_temp);
      if(pump_on == true) {
        sendData(7,1);
      } 
      else {
        sendData(7,0);
      }
      if(fan_on == true) {
        sendData(8,1);
      } 
      else {
        sendData(8,0);
      }
      sendData(9,sensor_errors);
      sensor_errors = 0;

      // If pachube didn't work before, but does now, clear the error
      if (error_no == ERR_PACHUBE) {
        error_no = NO_ERR;
        display_mode = DISP_TEMP;
      }
    }
    else {
      error_no = ERR_PACHUBE;
      display_mode = DISP_ERROR;        // Go to the error display page
    }
    client.stop();

    /*
    ** Also once per minute we adjust all the various counters. This makes fill counters and hold off timers all work in minutes rather
    ** than execution cycles.
    */
    
    if (pool_fill_rem > 0)              // If we are filling the pool with water, have we timed out?
      pool_fill_rem--;
 
    if (pump_hold_timer > 0)            // Decrement the hold timer. This time ensures we don't go switching the pump on and off to often.
      pump_hold_timer--;

    if (man_pump_run > 0);              // If we are in manual mode for the pump, have we finished?
      man_pump_run--;
      
    if(F_Switch == false)              // If the water level is still low increase the smoothing counter
      pool_fill_smooth++;
    else
      pool_fill_smooth = 0;            // If we see the pool as full, then restart the smoothing counter
  }


/*
** Shed Fan
** This figures out what the shed fan should do to keep the shed cool.
** There shouldn't be a need for any hysteresis on this as the thermal mass
** of the shed should do the job.
*/
  if(shed_temp > MAX_SHED_TEMP) {
    digitalWrite(SHEDFAN, HIGH);      // Turn fan on
    fan_on = true;
  }

  if(shed_temp < OK_SHED_TEMP) {
    digitalWrite(SHEDFAN, LOW);       // Turn fan off
    fan_on = false;
  }


/*
** If the pump was running in manual mode, check if the timer has expired
*/
  if(man_pump_run == 0) {
    digitalWrite(SOLARPUMP, LOW);     // Turn pump off
    pump_on = false;
    man_pump_set = DEFAULT_RUN_TIME;  // Go back to default value for manual run minutes, ready for next time
    display_mode = DISP_TEMP;         // Go back to the default display
  }


/*
** Pool Filling
** This checks the pool level sensor and decides if the pool needs water added.
**
** 1. We need to effectively "debounce" the water level sensor to allow for people
**    playing in the pool. We do this by requiring the float valve to be held for a certain number
**    of consecutive execution loops.
** 2. We need a timer so failure of the pool sensor won't release too much water.
** 3. We should log how long the fill solinoid was on for to track water usage. (See Pachube section)
*/

/*
** Check if the pool is full
** If the smoothing counter has counted more than POOL_FILL_SMOOTHED and the pool sensor still shows the pool
** needs water and we are not already in the process of fill it, then set the fill remaing timer to MAX_POOL_FILL_TIME and start filling.
*/
  if ((pool_fill_smooth > POOL_FILL_SMOOTHED) && (fill_on == false)) {
    digitalWrite(POOLFILLVALVE, HIGH);   // Turn pool fill valve on
    fill_on = true;                      // The valve is now on
    pool_fill_rem = MAX_POOL_FILL_TIME;  // Start a timrer to stop the fill if it goes on too long
  }  

  if (pool_fill_smooth == 0) {           // The water level is now OK
    digitalWrite(POOLFILLVALVE, LOW);    // Turn pool fill valve off
    fill_on = false;                     // The valve is now off
    pool_fill_rem = 0;                   // No filling, but everything OK
  }

  if(pool_fill_rem == 0) {               // If we timed out before the pool was full, something may be wrong
    digitalWrite(POOLFILLVALVE, LOW);    // turn the fill valve off
    fill_on = -1;                        // Block future filling till someone resets the error
    error_no = ERR_FILL;                 // Flag the error
    display_mode = DISP_ERROR;           // Go to the error display page
    pool_fill_rem = 0;              
  }

  

/*
** Gather user input
*/

/*
** If the user pressed up or down, update the display mode
*/

  if (U_Button == true) display_mode++;
  if (D_Button == true) display_mode--;

  if (display_mode > MAX_DISPLAY_MODE) display_mode = MIN_DISPLAY_MODE;  // The two impliment wrap around on the menu
  if (display_mode < MIN_DISPLAY_MODE) display_mode = MAX_DISPLAY_MODE;

/*
** If we are in the manual pump setting screen, let the user set a manual run time for the pump.
*/

  if( display_mode == DISP_PUMP ) {
    if(R_Button == true) man_pump_set = man_pump_set + 5;
      if(man_pump_set >= 1000) man_pump_set = 995;
    if(L_Button == true) man_pump_set = man_pump_set - 5;
      if(man_pump_set == 0) man_pump_set =5;
    if(S_Button == true) {
      man_pump_run = man_pump_set;
      digitalWrite(SOLARPUMP, HIGH);   // Turn pump on
      pump_on = true;
    }
  }


/*
** Now update the display
*/
  lcd.clear();

  switch (display_mode) {

  case DISP_TEMP:
    // Display temperatures
    error_no = NO_ERR;
    lcd.setCursor(0,0);
    lcd.print("P=");
    if (pool_temp > 0) {
      lcd.print(pool_temp);
    } 
    else {
      lcd.print("Error");
    }

    lcd.setCursor(9,0);
    lcd.print("A=");
    if (array_temp > 0) {
      lcd.print(array_temp);
    } 
    else {
      lcd.print("Error");
    }

    lcd.setCursor(0,1);
    lcd.print("F=");
    if (flow_temp > 0) {
      lcd.print(flow_temp);
    } 
    else {
      lcd.print("Error");
    }

    lcd.setCursor(9,1);
    lcd.print("R=");
    if (return_temp > 0) {
      lcd.print(return_temp);
    } 
    else {
      lcd.print("Error");
    }

    break;

  case DISP_TIME:
    // Display the current time from the RTC
    error_no = NO_ERR;
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
    error_no = NO_ERR;
    lcd.setCursor(0,0);
    lcd.print("C=");
    if ( shed_temp > 0 ) {
      lcd.print(shed_temp);
    } 
    else {
      lcd.print("Error");
    }

    if(F_Switch == true) {
      lcd.setCursor(0,1);
      lcd.print("H2O=OK ");
    } 
    else {
      lcd.setCursor(0,1);
      lcd.print("H2O=Low");
    }

    if(pump_on == true) {
      if(R_Button == true) {
        digitalWrite(SOLARPUMP, LOW);    // Manually turn pump off
        pump_on = false;
      }
      lcd.setCursor(8,0);
      lcd.print("Pump=On ");
    } 
    else {
      if(L_Button == true) {
        digitalWrite(SOLARPUMP, HIGH);   // Mannualy turn pump on
        pump_on = true;
      }
      lcd.setCursor(8,0);
      lcd.print("Pump=Off");
    }

    if(fill_on == true) {
      lcd.setCursor(8,1);
      lcd.print("Fill=On ");
    } 
    else {
      lcd.setCursor(8,1);
      lcd.print("Fill=Off");
    }
    break;

  case DISP_PUMP:
    // Manual pump timer mode
    error_no = NO_ERR;
    lcd.setCursor(0,0);
    lcd.print("Timer: Pump ");
    if(pump_on == true) {
      lcd.print("On");
    } 
    else {
      lcd.print("Off");
    }
    lcd.setCursor(0,1);
    lcd.print("Set ");
    lcd.print(man_pump_set);
    lcd.setCursor(8,1);
    lcd.print("Left ");
    lcd.print(man_pump_run);
    break;


  case DISP_ERROR:                 // Something went wrong....
    lcd.print(PROJECT_NAME);       // Who are we?
    lcd.setCursor(0,1);
    switch (error_no) {
    case ERR_FILL:
      // The fill operation failed
      lcd.print("  Fill Failed");
      break;

    case ERR_PACHUBE:
      lcd.print(" Pachube failed");
      break;

    case ERR_SENSORS:
      lcd.print("Sensor not found");
      break;

    case ERR_READ:
      lcd.print("Bad sensor read");
      break;

    default:
      lcd.print("   System OK");
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


/******************************************************************
 *******************************************************************
 ** Pachube client
 ** --------------
 ** This routine takes a datastream number and a value and posts them to Pachube
 */

// This method makes a HTTP connection to the server and puts the argument thisData to the datastream thisFeed.
//
void sendData(int thisStream, float thisData) {

  // Create string to be sent to Pachube
  sprintf(pachube_data,"%i,", thisStream);
  dtostrf(thisData,5,2,&pachube_data[2]);     // Write this data into the data string, starting at the 3rd character. Make it 5 chars in total with precission of 2 => XX.XX   

  // here's the actual content of the PUT request:
  client.println(pachube_data);

}


/******************************************************************
 *******************************************************************
 **
 ** Real Time Clock
 ** ---------------
 ** Here are the routines needed for handling the real time clock.
 ** The 2IC bus for this is on Analogue pins 4 & 5
 */


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


// Gets the date and time from the ds1307 and stores the result for later use
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



