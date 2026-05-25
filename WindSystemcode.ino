/*
Wind Measurement System with Visual Feedback using a Custom Made PCB and Anemometer
Ryan B
Firmware, Wiring, and System Integration

This code displays animated USA and Guatemala Flags on a 5x6 LED matrix
SW1 toggles between the flags (short press) and resets the max gust value (long press), SW2 increases the animation speed (wrap around), 
SW3 adjusts the LED brightness (short press) and toggles the serial monitor data formatting (hold) between 'Readable' and 
'CSV' (for easy data export and analysis). 
The flag animation is made by sequentially turning off one column at a time to replicate the waving of a flag.
The animation speed is also dynamically adjusted using an anemometer (wind sensor).
The analog output voltage of the anemometer is used to scale the animation's update rate based on the wind speed.
This effectively makes the flag animation faster/slower for higher/lower wind speeds.
The system includes easy to adjust parameters for wind calibration and flag animation sensitivity,
allowing the user to fine tune the system for various wind conditions and environments. 
See the "ADJUSTMENTS" section of the code below.
The system includes multiple functionalities for recording wind measurements. Firstly, the instantaneous wind speed in mph and
anemometer voltage is printed to the OLED screen for easy viewing. Also printed to the OLED screen is the 
max gust wind speed, which records the highest observed wind speed in mph. The average wind speed in mph and m/s, and the average anemometer voltage
over a user set period of time (see "ADJUSTMENTS") is printed to the serial monitor. The user can toggle between 'Readable' and 'CSV' output modes
for enhanced readability or easy data export.
*/

#include <FastLED.h>
#include <SPI.h>               
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>   

#define ROWS 5
#define COLS 6
#define NUM_LEDS 30
CRGB leds[NUM_LEDS];                //Makes LED array 

#define DATA_PIN 5                  //D5 LED data line
#define SW1_PIN 7                    
#define SW2_PIN 12   
#define SW3_PIN 6
#define WIND_PIN A0                 //Wind sensor data pin 


#define OLED_DC   9                 //Data/command
#define OLED_CS   10                //Chip select
#define OLED_RST  8                 //Reset

Adafruit_SH1106G display(128, 64, &SPI, OLED_DC, OLED_RST, OLED_CS);    //Initializes display - 128x64 resolution - uses hardware SPI bus
unsigned long lastOLED = 0;                                             //Stores the last time the OLED was updated

bool showUSA = true;                
uint8_t brightness = 5;                   //Initial LED brightness
uint8_t waveCol = 0;                

bool lastSW2State = HIGH;                 //Initializes variables for edge transition detection
bool lastSW3State = HIGH; 
bool sw1Pressed = false; 
bool sw3Pressed = false;  
    

unsigned long lastUpdate = 0;              //Stores time of last animation update
unsigned long sw1PressStartTime = 0;
unsigned long sw3pressStartTime = 0;       //Stores the start time of SW3 press to check for short or long press 
unsigned long avgStartTime = 0;            //Start time for averaging
unsigned long avgWindow;     
const unsigned long SECOND = 1000;
const unsigned long MINUTE = 60 * SECOND;

float maxGust_mph = 0.0;     
float windVoltage = 0.0;                   //Initialize to zero for first reading
float windScale = 1.0;                     //Starts flag animation at 'waveSpeed' 
float avgWind_mph = 0;
float avgWind_mps = 0;
float windSum = 0;                         //Stores the sum of average voltages from anemometer
float avgWind = 0;
int windCount = 0;                         //Stores the number of wind sensor reads

bool headerPrinted = false;                
bool csvMode = true;                      //false = readable, true = CSV


//**ADJUSTMENTS
//Adjustable timing for calculating average wind speeds  
int avgMinutes = 1;             //Number of minutes the system averages the wind speed over

//Voltage calibration: measured sensor output range: determined based on min and max anemometer voltage outputs corresponding to min and max wind speeds
const float maxVoltage = 1.3;   //*Measured a mean RC low pass output of 1.12 V for max fan speed. 1.3V gives some margin*
const float minVoltage = 0.02;  //*Measured a mean RC low pass output of 11mV. 20mV gives some margin*

//Wind scaling: estimated max wind speed corresponding to maxVoltage (*1.12V for our case*)
const float maxWindSpeed = 8.0;

//Animation sensitivity adjustments:
const float minScale = .3;      
const float maxScale = 3.5;      
//Increasing (maxScale - minScale) increases sensitivity, decreasing it reduces animation sensitivity (larger change in speed with wind)
//Larger maxScale increases maximum possible animation speed
//Lower minScale decreases the minimum animation speed (normalized = 0 -> windScale = minScale)
//windScale is clamped to prevent extreme animation speeds
//Ensure maxScale aligns with clamp limits if adjusting sensitivity

//Adjustable hold duration for SW1 and SW3 long press 
const unsigned long HOLD_TIME = 1000;  //1 second hold

//Adjustable default wave update delay
uint16_t waveSpeed = 130;              //Base delay between updates in ms (scaled by wind)  | lower = faster animation | Higher = slower animation
//**ADJUSTMENTS


//SERPENTINE MAPPING
int getIndex(int row, int col) {              //Convert row, col to LED index
  if (row % 2 == 0) {                         //Checks for even row 
    return row * COLS + col;                  //Left to right mapping - COLS = 6 | ex. row = 2 col = 2 : returns 14 | col = 3 : returns 15
  } else {                                    //For odd rows
    return row * COLS + (COLS - 1 - col);     //Right to left mapping COLS = 6 | ex. row = 3 col = 2 : returns 21 | col = 3 : returns 20
  }                                           //This handled snake wiring from layout Row 0:  0 -> 1 -> 2 -> 3 -> 4 -> 5
//                                                                                    Row 1: 11 <- 10 <- 9 <- 8 <- 7 <- 6
//                                                                                    Row 2: 12 -> 13 -> 14 -> 15 -> 16 -> 17 

}


//USA
CRGB usa[ROWS][COLS] = {                                                          //2D array called usa, size 5x6,  type CRGB from fastLED  
  {CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Red, CRGB::Red, CRGB::Red},          //Sets colors
  {CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::White, CRGB::White, CRGB::White},
  {CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Red, CRGB::Red, CRGB::Red},
  {CRGB::White, CRGB::White, CRGB::White, CRGB::White, CRGB::White, CRGB::White},
  {CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red},
};

//GUATEMALA
CRGB guatemala[ROWS][COLS] = {
  {CRGB::Blue, CRGB::Blue, CRGB::White, CRGB::White, CRGB::Blue, CRGB::Blue},   //2D array called guatemala, size 5x6,  type CRGB from fastLED
  {CRGB::Blue, CRGB::Blue, CRGB::White, CRGB::White, CRGB::Blue, CRGB::Blue},   //Sets colors
  {CRGB::Blue, CRGB::Blue, CRGB::Green, CRGB::Green, CRGB::Blue, CRGB::Blue},
  {CRGB::Blue, CRGB::Blue, CRGB::White, CRGB::White, CRGB::Blue, CRGB::Blue},
  {CRGB::Blue, CRGB::Blue, CRGB::White, CRGB::White, CRGB::Blue, CRGB::Blue},
};


//DRAW FLAG FUNCTION
void drawFlag(CRGB flag[][COLS]) {              //Function takes 2D flag array 
  for (int row = 0; row < ROWS; row++) {        //Iterates through rows
    for (int col = 0; col < COLS; col++) {      //Iterates through columns | hits every LED position "for each row go through each column..."

      int idx = getIndex(row, col);             //Sets idx to the LED index

      if (idx < NUM_LEDS) {                     //For memory safety: keeps operation inside LED array
        if (col == waveCol) {                   //Checks if col is the one to be turned off
          leds[idx] = CRGB::Black;              //LED at index off
        } else {
          leds[idx] = flag[row][col];           //LED normal color from CRGB array
        }
      }
    }
  }
  FastLED.show();                               //LED update
}

//SETUP
void setup() {                                                
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);         //Initializes LEDs & configures PIN 5 to handle LED data | WS2812 LED type
  FastLED.setBrightness(brightness);                              //Sets 'brightness' in FastLED

  pinMode(SW1_PIN, INPUT_PULLUP);                                 //Pull up | Button not pressed: HIGH | Button Press: LOW
  pinMode(SW2_PIN, INPUT_PULLUP);      
  pinMode(SW3_PIN, INPUT_PULLUP);
  
  avgWindow = avgMinutes * MINUTE;                                //Averaging time in ms

  Serial.begin(9600);                                                                                                                 
  display.begin(0x3C, true);                                      //Initialize OLED, clear buffer, set text formatting
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  avgStartTime = millis();                                        //Record start time for averaging window 
}


void loop() {

unsigned long totalSec = millis() / 1000;                      //Convert ms to S
bool sw1State = digitalRead(SW1_PIN);
bool sw2State = digitalRead(SW2_PIN);
bool sw3State = digitalRead(SW3_PIN);                         

if (!headerPrinted && Serial) {                                  //Print CSV header once when serial monitor connects (prevents missing header at startup)
  Serial.println("Time_s,Avg_mph,Avg_mps,Voltage_V,MaxGust_mph");
  headerPrinted = true;
}

// SW1 TOGGLE FLAGS (SHORT PRESS) MAX GUST RESET (LONG PRESS)
if (!sw1Pressed && sw1State == LOW) {                          //Detects when the button is first pressed (HIGH to LOW edge (button pressed))
  sw1Pressed = true;                                           //Sets button press flag to true "button is currently being held"
  sw1PressStartTime = millis();                                //sw1pressStartTime records the start time of the button hold
}

if (sw1Pressed && sw1State == HIGH) {                          //Detects button release (LOW to HIGH edge)
  unsigned long duration = millis() - sw1PressStartTime;       //Calculates and records the duration of the button hold

  if (duration > HOLD_TIME) {                                  //Input is a long press
    maxGust_mph = 0;                                           //Resets maxGust
    Serial.println("Max gust reset");                          //Prints notification message to serial
  } else {
    showUSA = !showUSA; // normal toggle                       //Toggle flags for short press
  }
  sw1Pressed = false;                                          //Reset button state for next press
}

//SW2 MANUAL ANIMATION SPEED INCREASE (SHORT PRESS)
  if (lastSW2State == HIGH && sw2State == LOW) {               //Detects HIGH to LOW edge (button pressed) 

    if (waveSpeed > 40) {                                      //40 is the delay in ms: Higher delay -> slower flag speed
      waveSpeed -= 20;                                         //Button press makes delay shorter -> faster flag speed 
    } else {
      waveSpeed = 200;                                         //If the delay is too short the delay is set 200ms for wrap around
    }
    delay(200);                                                //Debounce delay to prevent multiple rapid triggers
  }
  lastSW2State = sw2State;                                     //Stores value for next edge detection


//SW3 BRIGHTNESS (SHORT PRESS) and CSV MODE (LONG PRESS) 
if (!sw3Pressed && sw3State == LOW) {                            //Detects when the button is first pressed (HIGH to LOW edge (button pressed))
  sw3Pressed = true;                                             //Sets button press flag to true "button is currently being held"
  sw3pressStartTime = millis();                                  //sw3pressStartTime records the start time of the button hold
} 
if (sw3Pressed && sw3State == HIGH) {                            //Detects button release (LOW to HIGH edge)
  unsigned long pressDuration = millis() - sw3pressStartTime;    //Calculates and records the duration of the button hold

  if (pressDuration < HOLD_TIME) {                               //Input is a short press
    brightness += 10;                                            //Adds brightness
    if (brightness > 50){
      brightness = 5;                                            //Wrap around
    } 
    FastLED.setBrightness(brightness);                           //Updates LED brightness
  } else {                                                     
    csvMode = !csvMode;                                          //Toggles serial output mode (long press)
    Serial.print("Mode changed to: ");                    
    Serial.println(csvMode ? "CSV" : "Readable");                //csvMode true - prints csvMode | csvMode false - prints Readable
  }
  sw3Pressed = false;                                            //Reset
}


//WIND SENSOR READ
int rawWind = analogRead(WIND_PIN);                         //Reads voltage from Anemometer voltage output -> ADC 0-1023 10 bit
float newVoltage = (rawWind / 1023.0) * 5.0;                //Converts ADC reading (0–1023) to voltage using the Arduino's ADC reference voltage (5V)
windVoltage = 0.8 * windVoltage + 0.2 * newVoltage;         //Digital low pass filter applied to ADC readings to reduce noise. 80% previous value + 20% current sample for smoothing

if (windVoltage < minVoltage) windVoltage = minVoltage;     //Clamp voltage to calibrated sensor range to prevent invalid normalization
if (windVoltage > maxVoltage) windVoltage = maxVoltage;     

windSum = windVoltage + windSum;                            //Accumulates total sensor voltage for averaging
windCount++;                                                //Counts the number of samples collected

//INSTANTANEOUS WIND SPEED CALCULATIONS
float normalized = (windVoltage - minVoltage) / (maxVoltage - minVoltage);      //Gives fraction of calibrated FSR (Full Scale Range) using minVoltage and maxVoltage
if (normalized < 0.0) normalized = 0.0;                                         //Clamp normalized value to [0,1] to handle out of range inputs
if (normalized > 1.0) normalized = 1.0;

//CONVERT INSTANTANEOUS WIND SPEED FROM VOLTAGE TO m/s AND MPH 
float windSpeed_mps = normalized * maxWindSpeed;                                //Uses the normalized value (0–1) to scale the wind speed as a fraction of maxWindSpeed
float windSpeed_mph = windSpeed_mps * 2.237;                                    //1mps = 2.237mph

if (windSpeed_mph > maxGust_mph + .1) {                                         //maxGust_mph updates only when wind speed is more than 0.1 mph higher (filters small fluctuations for a stable display)
  maxGust_mph = windSpeed_mph;
}                                        

//CONTROL - ANIMATION SCALING
windScale = minScale + normalized * (maxScale - minScale);                      //windScale adjusts animation speed by scaling the update delay (higher windScale = faster wave)
//                                                                              //(maxScale - minScale) sets the windScale range. Multiplying by normalized selects a position in this range
//                                                                              //Increasing (maxScale - minScale) increases sensitivity; decreasing it reduces sensitivity 
//                                                                              //Larger maxScale increases maximum possible animation speed
//                                                                              //Lower minScale decreases the minimum animation speed (normalized = 0 -> windScale = minScale)
//                                                                              //Higher windScale -> faster animation | Lower windScale -> slower animation 

if (windScale < 0.2) windScale = 0.2;                                           //windScale clamp to prevent extreme animation speeds
if (windScale > 5.0) windScale = 5.0;                                           


//OLED DISPLAY                     *
if (millis() - lastOLED > 100) {                                //Updates OLED every 100ms using non blocking timing
                                             
  display.clearDisplay();                                       //Clear display buffer (wipe memory)

  display.setTextSize(1);                                       
  display.setCursor(0,0);                
  display.print("TheBlowedPCB");                                //Lighthearted project label from initial concept (manual airflow testing)
 
  display.setCursor(0,20);                
  display.print("Speed: ");
  display.print(windSpeed_mph, 1);
  display.println(" mph");

  display.setCursor(0,35);               
  display.print("Voltage: ");
  display.print(windVoltage, 2);
  display.println(" V");

  display.setCursor(0,50);   
  display.print("Max Gust: ");
  display.print(maxGust_mph, 1);
  display.print(" mph");

  display.display();                                           //Push buffered graphics to the OLED screen

  lastOLED = millis();
}                                


//WAVE UPDATE
  if (millis() - lastUpdate > (waveSpeed / windScale)) {      //Update animation when elapsed time exceeds the scaled delay (waveSpeed / windScale)
    waveCol++;                                                //Move to next column for wave effect                       

    if (waveCol >= COLS) {                                    //Performs wrap around for continuous animation
      waveCol = 0;
    }

    lastUpdate = millis();                                    //Reset timer reference
  }


//AVERAGE WIND SPEED CALCULATIONS  
if (millis() - avgStartTime >= avgWindow) {     

  if (windCount > 0) {                                                         //Cannot divide by zero
    avgWind = windSum / windCount;                                             //Computes the average anemometer voltage by dividing the accumulated voltage by the number of samples
  }
  float avgNormalized = (avgWind - minVoltage) / (maxVoltage - minVoltage);    //Gives fraction of calibrated FSR (Full Scale Range)  using minVoltage and maxVoltage
  if (avgNormalized < 0.0) avgNormalized = 0.0;                                //Clamp normalized value to [0,1] to handle out of range inputs
  if (avgNormalized > 1.0) avgNormalized = 1.0;

//CONVERT AVERAGE WIND SPEED FROM VOLTAGE TO m/s and MPH 
  avgWind_mps = avgNormalized * maxWindSpeed;                                  //Uses the normalized value (0–1) to scale the avg wind speed as a fraction of maxWindSpeed
  avgWind_mph = avgWind_mps * 2.237;                                           //1m/s = 2.237mph



//SERIAL MONITOR PRINTING               
//CSV MODE | ex. 1920,6.65,2.97,0.50,8 | Seconds elapsed, avg wind speed in MPH. avg wind speed in m/s, average anemometer voltage, max gust speed in MPH
if (csvMode) {                                    
  Serial.print(totalSec);
  Serial.print(",");
  Serial.print(avgWind_mph);
  Serial.print(",");
  Serial.print(avgWind_mps);
  Serial.print(",");
  Serial.print(avgWind);
  Serial.print(",");
  Serial.println(maxGust_mph);

} else {              

//READABLE MODE | [Hrs:Mins:Seconds] | ex. [00:07:00] 6.44 mph | 2.88 m/s | 0.48 V | 10 mph
  int hours = totalSec / 3600;                    //Convert total seconds to hours
  int minutes = (totalSec % 3600) / 60;           //Takes the seconds remaining after removing full hours, then converts to minutes  
  int seconds = totalSec % 60;                    //Gives the remaining seconds after hours and minutes

  Serial.print("[");
  if (hours < 10){                                //Add leading zero if single digit
  Serial.print("0");       
  }       
  Serial.print(hours);                            //Prints hours - will be a two digit value, otherwise there will be a leading zero
  Serial.print(":");
  if (minutes < 10){
  Serial.print("0");                              //Add leading zero if single digit
  }     
  Serial.print(minutes);                          //Prints minutes - will be a two digit value, otherwise there will be a leading zero
  Serial.print(":");

  if (seconds < 10){                              //Add leading zero if single digit
  Serial.print("0");
  } 
  Serial.print(seconds);                          //Prints seconds - will be a two digit value, otherwise there will be a leading zero
  Serial.print("] ");

  Serial.print("Avg: ");
  Serial.print(avgWind_mph);                      //Prints average wind speed (MPH, m/s) and average anemometer voltage 
  Serial.print(" mph | ");
  Serial.print(avgWind_mps);
  Serial.print(" m/s | ");
  Serial.print(avgWind);
  Serial.print(" V | ");
  Serial.print("Max Gust: ");
  Serial.print(maxGust_mph);
  Serial.println(" mph");
}

  windSum = 0;                                     //Reset averaging variables for next window
  windCount = 0;
  avgStartTime += avgWindow;                       //Advances the start time by one averaging window to maintain consistent intervals (prevents timing drift)
}

//FLAG DRAW (SELECT FLAG BASED ON TOGGLE)
  if (showUSA) {
    drawFlag(usa);                               //Display USA flag
  } else {
    drawFlag(guatemala);                         //Display Guatemala flag
  }
}
