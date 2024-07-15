
/******************************************************************************
  Arduino Swag Control
*******************************************************************************/
String ProjVersion = "/*********************************************************************************\n";

// *************************************************************
//  ~~~~~~~ LIBRARIES SECTION ~~~~~~~
//
// *************************************************************

// #include "math.h"            // quadrature needs math lib
// #include "SoftReset.h"       // SoftReset lib,   https://github.com/WickedDevice/SoftReset

//#include "LedControl.h"         // MAX7219 library, http://www.wayoda.org/arduino/ledcontrol/index.html  
//#include "ClickEncoder.h"       // Rotary switch encoder lib, https://github.com/0xPIT/encoder
//#include <TimerOne.h>
#include "Servo.h"
// *************************************************************
//  ~~~~~~~ CONSTANTS DEFINITION SECTION ~~~~~~~
//
// *************************************************************
/// FUNCTIONS DECLARATION
void ProcessStatic(struct swstatic_ * AnySwitch);
void Buzz(uint8_t del,uint8_t off_del);
// *************************************************************
//  MAX7219 DISPLAY Constant setting
// *************************************************************
#define LED_INTENSITY 5
#define MAX_NUMCHIPS 2

// *************************************************************
//  PINOUT setting
// *************************************************************


#define LD_B_PIN 2            // ->>
#define LD_R_PIN 3            // ->>
#define LD_G_PIN 4            // ->>
#define SYS_PIN 5             // ->>
#define BUSY_LD_PIN 6         // ->>
#define FIRE_LD_PIN 7         // ->>
#define SW_FIRE_PIN 8         // <<-
#define SW_ST_EN_PIN 9        // <<-
#define BZ_PIN    10          // ->>
#define SERVO_PIN  11         // ->>
#define HEATER_PIN  12        // ->>
#define EN_LED 13             // ->>

#define MAX_FIRE_TIME 10000
#define HEAT_TIME 500




// *************************************************************
//  ~~~~~~~ STRUCTS & VAR  DEFINITION SECTION ~~~~~~~
//
// *************************************************************

Servo Trigger;
// *************************************************************
//  SW_MOMENTARY_TYPE definition
//  Typedef for switch data structure used in function ProcessMomentary
//  swmom_  for functions, SW_MOMENTARY_TYPE for variables
// *************************************************************
typedef struct swmom_ {
  uint8_t pin;
  unsigned long millis;
  uint8_t state;
} SW_MOMENTARY_TYPE;

// *************************************************************
//  MOMENTARY SWITCH Instance
// *************************************************************
SW_MOMENTARY_TYPE SW_FIRE = { SW_FIRE_PIN, 0, 0};
/*SW_MOMENTARY_TYPE SW_S2 = { SW_S2_PIN, 0, 0};
SW_MOMENTARY_TYPE SW_S3 = { SW_S3_PIN, 0, 0};
SW_MOMENTARY_TYPE SW_S4 = { SW_S4_PIN, 0, 0};
SW_MOMENTARY_TYPE SW_S5 = { SW_S5_PIN, 0, 0};
SW_MOMENTARY_TYPE SW_S6 = { SW_S6_PIN, 0, 0};*/

// *************************************************************
//  SW_STATIC_TYPE definition
//  Typedef for switch data structure used in function ProcessStatic
//  swstatic_  for functions, SW_STATIC_TYPE for variables
// *************************************************************
typedef struct swstatic_ {
  uint8_t pin;
  uint8_t prec_state;
  uint8_t state;
} SW_STATIC_TYPE;

// *************************************************************
//  STATIC SWITCH Instance
// *************************************************************
SW_STATIC_TYPE SW_ST_EN = { SW_ST_EN_PIN, 0, 0};



// *************************************************************
//   TIMING variables
//   variables for Millis counter
// *************************************************************
unsigned long ul_PreviousMillis;
unsigned long ul_CurrentMillis;

// *************************************************************
//   DEBOUNCE variables
//   DEBOUNCE and pushed long time values
// *************************************************************

long               debouncesw = 50;             // switch debounce time in milliseconds (1000 millis per second)
const unsigned int PUSH_LONG_LEVEL1  = 1;    // Encoder Switch pressed LONG time value (5000 = five seconds)
unsigned long      millisSinceLaunch = 1;       // previous reading of millis, seed with 1 to init, set to 0 if don't want to use
unsigned long      delaytime = 250;
unsigned long      shortdel = 50;
unsigned long      mydelay = 100;




// *************************************************************
//   GENERIC variables
//   variables for flags and states
//   variables for master and avionics switches or states, test state, lights, etc
// *************************************************************
boolean gbMasterVolts = false;

typedef enum 
{
    DISABLED=0,
    ENABLED=1
} EN_STATE_TYPE;

typedef enum 
{
    IDLE,
    ACCEPTING,  //switch debounced --wait long press
    ENGAGED,    //switch state is 3 --before was 2 OPEN HEATER DRAIN 
    HEATING,    //switch state is 3-- wait time to heat 
    FIRING,     //switch state is 3-- HEATER IS OK--TIGGER

} Ignition_state;

typedef struct  
{
    Ignition_state state;
    Ignition_state prec_state;
    unsigned long heat_start_time;
    unsigned long heat_time;
    unsigned long max_fire_time;
    unsigned long fire_start_time;
    SW_STATIC_TYPE* enable_sw;
    bool enable_led;
    SW_MOMENTARY_TYPE* fire_sw;

} IGNITER_TYPE;

IGNITER_TYPE IGNITER_1;
//


// *************************************************************
//   END PREPROCESSOR CODE
// *************************************************************







// *************************************************************
//
//  ####  ###### ##### #    # #####
//  #     #        #   #    # #    #
//  ####  #####    #   #    # #####
//      # #        #   #    # #
//  ####  ######   #    ####  #
//
//  ~~~~~~~~~~~~~~~~~~   SETUP SECTION   ~~~~~~~~~~~~~~~~~~~~~~
// *************************************************************
void setup() 
{


  Buzz(350,100);
  Buzz(350,100);
  Buzz(350,100);


  // Switches, set the pins for pullup mode  READ THIS!!! --> https://www.arduino.cc/en/Tutorial/DigitalPins
 
  /*pinMode(SW_S2.pin, INPUT_PULLUP);
  pinMode(SW_S3.pin, INPUT_PULLUP);
  pinMode(SW_S4.pin, INPUT_PULLUP);
  pinMode(SW_S5.pin, INPUT_PULLUP);
  pinMode(SW_S6.pin, INPUT_PULLUP);
  */
  
  pinMode(SW_FIRE.pin, INPUT_PULLUP);
  pinMode(SW_ST_EN.pin, INPUT_PULLUP);

  pinMode(HEATER_PIN, OUTPUT);  /*HEAT Element pin defines as output*/
  pinMode(BZ_PIN, OUTPUT);
  pinMode(LD_R_PIN, OUTPUT);    /*Red pin defined as output*/
  pinMode(LD_G_PIN, OUTPUT);   /*Green pin defined as output*/
  pinMode(LD_B_PIN, OUTPUT);  /*Blue pin defined as output*/
  pinMode(EN_LED, OUTPUT);
  Trigger.attach(SERVO_PIN);

  //pinMode(SERVO_PIN, OUTPUT);

  // ********* Serial communications to PC
  Serial.begin(115200);  // output


  InitStatic(&SW_ST_EN);
  
  InitIgnition(&IGNITER_1,&SW_ST_EN,&SW_FIRE);
  //IGNITER_1.enable_sw=&SW_ST_EN;
  // end of SETUP Light up the LED on pin 13
  
  //digitalWrite(EN_LED, HIGH);           // turn the ENABLE  LED on 

  FireTrigger(DISABLED);
  HeatingElement(DISABLED);
} // end of SETUP
// *************************************************************
//   END SETUP CODE
// *************************************************************




// *************************************************************
//
//  #       ####   ####  #####
//  #      #    # #    # #    #
//  #      #    # #    # #####
//  #      #    # #    # #
//  ######  ####   ####  #
//
//  ~~~~~~~~~~~~~~~~~~   LOOP SECTION   ~~~~~~~~~~~~~~~~~~~~~~
// *************************************************************
void loop() 
{
  //TIME SNAPSHOTS FOR LOOP
  ul_CurrentMillis = millis();
  
 
  // *************************************************************
  // READ and PARSE SERIAL INPUT (version 2)
  // Jim's program (Link2FS) sends commands batched up /A1/F1/J1/K1 (no NL separator)
  // added IsSerialCommand routine to fetch them one at a time up to separator like / or =
  // leaving rest on serialbuffer for next loop to process
  // *************************************************************


  // *************************************************************
  // Respond to switch button presses
  // processing 2 mom sw in radio panel
  // *************************************************************

  // Check using pointers instead of big chunks o code
  ProcessMomentary(&SW_FIRE);          // FIRE BUTTON LONG PRESS
  //ProcessMomentary(&SW_S2);          // COM1 BTN
  //ProcessMomentary(&SW_S3);          // COM2 BTN
  //ProcessMomentary(&SW_S4);          // NAV1 BTN
  //ProcessMomentary(&SW_S5);         //  NAV2 BTN
  //ProcessMomentary(&SW_S6);         //  toggle freq
  ProcessStatic(&SW_ST_EN);
  // ***************************************************
  // Switch action, state==2 means pressed and debounced, ready to read
  // ***************************************************


  // SEND CMD AND CHANGE TO READY STATE  Act on regular button presses

  //Serial.println(SW_ST_EN.state );
 // ENABLE  Button
  if (SW_ST_EN.state !=SW_ST_EN.prec_state) {
    Serial.println("CHANGED" );
    if(SW_ST_EN.state==HIGH)
    {
      Serial.println("ENABLE BTN-->ENABLED");
    }
    else
    {
      Serial.println("ENABLE BTN-->DISABLED");
    }
  

  }

  
    
  
  ProcessIgnition(&IGNITER_1);
}


  // SEND CMD AND CHANGE TO READY STATE  Act on rotary encoder inc/dec

// *************************************************************
//   END LOOP CODE
// *************************************************************




// *************************************************************
//
//  ###### #    # #    #  ####  ##### #  ####  #    #  ####
//  #      #    # ##   # #    #   #   # #    # ##   # #
//  #####  #    # # #  # #        #   # #    # # #  #  ####
//  #      #    # #  # # #        #   # #    # #  # #      #
//  #      #    # #   ## #    #   #   # #    # #   ## #    #
//  #       ####  #    #  ####    #   #  ####  #    #  ####
//
//  ~~~~~~~~~~~~~~~~~~   FUNCTION SECTION   ~~~~~~~~~~~~~~~~~~~~~~
// *************************************************************

void RGB_output(int redLight, int greenLight, int blueLight)
  {
    analogWrite(LD_R_PIN, redLight); //write analog values to RGB
    analogWrite(LD_G_PIN, greenLight);
    analogWrite(LD_B_PIN, blueLight);
}

void Buzz(uint8_t del,uint8_t off_del)
{
  tone(BZ_PIN,1500);
  delay(del);
  noTone(BZ_PIN);
  delay(off_del);
}

// *************************************************************
//   FireTrigger(EN_STATE_TYPE)
//   
// *************************************************************
void FireTrigger( EN_STATE_TYPE en) 
{
  switch(en)  
  {
    case 0:
          Trigger.write(0);
          break;
    case 1:
          /*tone(BZ_PIN,800);
          delay(50);
          noTone(BZ_PIN);
          delay(50);
          tone(BZ_PIN,2000);
          delay(150);
          noTone(BZ_PIN);
          delay(50);
          tone(BZ_PIN,2000);
          delay(150);
          noTone(BZ_PIN);
          delay(50);
          tone(BZ_PIN,2000);
          delay(150);
          noTone(BZ_PIN);
          delay(50);*/
          Trigger.write(90);
          break;
    default:
          break;
      
  }
  delay(15); 
}

// *************************************************************
//   HeatingElement(EN_STATE_TYPE)
//   
// *************************************************************
void HeatingElement( EN_STATE_TYPE en) 
{
  switch(en)  
  {
    case 0:
         /* tone(BZ_PIN,2000);
          delay(50);
          noTone(BZ_PIN);
          delay(50);
          tone(BZ_PIN,800);
          delay(50);
          noTone(BZ_PIN);
          delay(100);*/
          digitalWrite(HEATER_PIN,LOW);
          break;
    case 1:
          tone(BZ_PIN,800);
          delay(50);
          noTone(BZ_PIN);
          delay(50);
          tone(BZ_PIN,2000);
          delay(100);
          noTone(BZ_PIN);
          delay(50);
          digitalWrite(HEATER_PIN,HIGH);
          break;
    default:
          break;
      
  }
  
}



// *************************************************************
//   ProcessMomentary(pointer to swmom_ struct)
//   Processes the specified switch state of momentary buttons
//   Read the state of the switch to get results
//   - state 0, switch not pressed
//   - state 2, switch pressed and debounced (short press)
//   - state 4, switch pressed and held for PUSH_LONG_LEVEL1 (eg 3 seconds, 5 seconds)
//   - state 6, switch pressed and held for 2x PUSH_LONG_LEVEL1
// *************************************************************
void ProcessMomentary(struct swmom_ * AnySwitch)
{
  //Serial.print("Switch value");
  //Serial.println(digitalRead(AnySwitch->pin));
  //Serial.print("Switch state");
  //Serial.println(AnySwitch->state);
  if (digitalRead(AnySwitch->pin) == LOW) {       // switch pressed
    switch (AnySwitch->state) {
      case 0: /* first instance of press */ AnySwitch->millis = ul_CurrentMillis; AnySwitch->state = 1; break;
      case 1: /* debouncing counts */       
        if (ul_CurrentMillis - AnySwitch->millis > debouncesw) 
          AnySwitch->state = 2;
        break;
      case 2: /* DEBOUNCED, read valid */  
        Serial.println("DEBOUNCED STATE 2");
        if (ul_CurrentMillis - AnySwitch->millis > PUSH_LONG_LEVEL1) 
          AnySwitch->state = 3;
        break;
      case 3: /* LONG PRESS, read valid */ 
        Serial.println("LONG PRESS STATE 3");
        break;

      case 5: /* For LONG * 2 press  _not in this code_*     if (ul_CurrentMillis - AnySwitch->millis > (PUSH_LONG_LEVEL1 * 2)) {
          AnySwitch->state = 6;*/
          break;
        
      case 6: /* LONG PRESS * 2, read  */   break;
      case 7: /* LONG PRESS * 2, hold  */   break;
    } // end switch
  } else {    // button went high, reset the state machine
    AnySwitch->state = 0;
  }
}




// *************************************************************
//   ProcessStatic(pointer to swmom_ struct)
//   Processes the specified switch state of static switch
//   Send serial value on state change
// *************************************************************
void ProcessStatic(struct swstatic_ * AnySwitch)
{
   AnySwitch->prec_state = AnySwitch->state;
  AnySwitch->state = digitalRead(AnySwitch->pin);
 
 
  if (AnySwitch->state != AnySwitch->prec_state) {
    if ( AnySwitch->state  == LOW) Serial.println("LOW");
    else Serial.println("HIGH");
  }

}

// *************************************************************
//   InitStatic(pointer to swmom_ struct)
//   Read state of static switch and send consistent value
//   MAYbe Better call this during setup ending (not before)
// *************************************************************
void InitStatic(struct swstatic_ * AnySwitch)
{
  
  AnySwitch->state = digitalRead(AnySwitch->pin);
  AnySwitch->prec_state = AnySwitch->state;
  if ((AnySwitch->state == LOW))
    Serial.println("LOW");
  else Serial.println("HIGH");
}










// *************************************************************
//   InitIgnition(pointer to IGNITER_TYPE struct)
//   Init the igniter state
//   Send serial value on state change
// *************************************************************
void InitIgnition(IGNITER_TYPE * AnyIgniter,SW_STATIC_TYPE* en_sw , SW_MOMENTARY_TYPE* fire_sw)
{
  Serial.println("RESETTING IGNITER"); 
  AnyIgniter->enable_sw=en_sw;
  AnyIgniter->enable_led=en_sw->state;
  digitalWrite(EN_LED,AnyIgniter->enable_led);
  AnyIgniter->fire_sw=fire_sw;

  AnyIgniter->prec_state = 0; 
  AnyIgniter->state=0;

  AnyIgniter->fire_start_time=0;
  AnyIgniter->max_fire_time=MAX_FIRE_TIME;
  AnyIgniter->heat_start_time=0;
  AnyIgniter->heat_time=HEAT_TIME;
 


  
}



// *************************************************************
//   ProcessIgnition(pointer to IGNITER_TYPE struct)
//   Processes the igniter state
//   Send serial value on state change
/*{
    IDLE,
    ACCEPTING,  //switch debounced --wait long press
    ENGAGED,    //switch state is 3 --before was 2 OPEN HEATER DRAIN 
    HEATING,    //switch state is 3-- wait time to heat 
    FIRING,     //switch state is 3-- HEATER IS OK--TIGGER

} Ignition_state;*/
// *************************************************************
void ProcessIgnition(IGNITER_TYPE * AnyIgniter)
{
  if (AnyIgniter->enable_sw->state == HIGH)
  {
       
      if (AnyIgniter->enable_sw->state != AnyIgniter->enable_sw->prec_state )
      {
          Serial.println("IGNITER ENABLED");
         
          Buzz(50,100);
          
          Buzz(50,100);
          
          Buzz(50,100);

          AnyIgniter->enable_led=HIGH;
          digitalWrite(EN_LED,AnyIgniter->enable_led);
        //RESETTING AFTER ENABLING
        AnyIgniter->prec_state=IDLE;
        AnyIgniter->state=IDLE;

      }
     /* else
      {
        //TOGGLE EN LED
        if (ul_CurrentMillis%700==0)
            {
              AnyIgniter->enable_led=!AnyIgniter->enable_led;
              digitalWrite(EN_LED,AnyIgniter->enable_led);
            }

      }*/

      AnyIgniter->prec_state=AnyIgniter->state;
      //PROCESS STATE
      switch (AnyIgniter->state)
      {
        case IDLE:
           {
            RGB_output(0,255,0); //GREEN
            

            if (AnyIgniter->fire_sw->state==0)
              {
                AnyIgniter->state=0;
              }
            else if (AnyIgniter->fire_sw->state==2)
              {
                AnyIgniter->state=ACCEPTING;
              }
           }
        break;
        case ACCEPTING:
        {

            if (AnyIgniter->fire_sw->state==0)
              {
                AnyIgniter->state=0;
              }
             else if (AnyIgniter->fire_sw->state==3)
              {
                AnyIgniter->state=ENGAGED;
              }
        }
        break;
        case ENGAGED: //start heating open drain - take start millis
        {
            RGB_output(255,255,0);
            if (AnyIgniter->fire_sw->state==0)
              {
                AnyIgniter->state=0;
              }
             else if (AnyIgniter->fire_sw->state==3)
              {
                //OPEN DRAIN
                AnyIgniter->heat_start_time=ul_CurrentMillis;
                AnyIgniter->state=HEATING;
                
                HeatingElement(ENABLED);
              }else
              {
                //CLOSE DRAIN
                 AnyIgniter->state=0;
                 HeatingElement(DISABLED);
              }
        }
        break;
        case HEATING:
        {
           
             if (AnyIgniter->fire_sw->state==0)
              {
                //CLOSE DRAIN
                AnyIgniter->state=0;
                HeatingElement(DISABLED);
              }
             else if (AnyIgniter->fire_sw->state==3 )
              {
                if(ul_CurrentMillis-AnyIgniter->heat_start_time<AnyIgniter->heat_time)
                  {
                    AnyIgniter->state=HEATING;  //keep heating
                  }else
                  {
                    
                    AnyIgniter->fire_start_time=ul_CurrentMillis; //sample fire timing
                    AnyIgniter->state=FIRING;  //open TRGGER
                    FireTrigger(ENABLED);
                
                  }
              }else
              {
                HeatingElement(DISABLED);
                AnyIgniter->state=0;
              }
        }
        break;
        case FIRING:
        {
            RGB_output(255,0,0);
            if (AnyIgniter->fire_sw->state==0)
              {
                AnyIgniter->state=0;
                FireTrigger(DISABLED);
                HeatingElement(DISABLED);
              }
            else if  (AnyIgniter->fire_sw->state=3)
              {
                if (ul_CurrentMillis-AnyIgniter->fire_start_time<AnyIgniter->max_fire_time) 
                {
                  //KEEP FIRING
                  AnyIgniter->state=FIRING;
                }
                else
                {
                  //AUTO DISABLING
                  AnyIgniter->state=0;
                  FireTrigger(DISABLED);
                  HeatingElement(DISABLED);
                  Buzz(200,300);
                  Buzz(300,10);
                }
              }
              else
              {
                FireTrigger(DISABLED);
                HeatingElement(DISABLED);
                AnyIgniter->state=0;
              }
        }
        break;
      }

  }else
  {
     if (AnyIgniter->enable_sw->state != AnyIgniter->enable_sw->prec_state )
      {
           //DISBLED
          //Led BUTTON OFF*/
        Serial.println("IGNITER DISABLED");
        Buzz(200,300);
        Buzz(300,10);
        AnyIgniter->enable_led=LOW;
        digitalWrite(EN_LED,AnyIgniter->enable_led);
        FireTrigger(DISABLED);
        HeatingElement(DISABLED);
      }
     
  }
  if (AnyIgniter->state != AnyIgniter->prec_state) {
    if ( AnyIgniter->state  == LOW) Serial.println("A01");
    else Serial.println("A02");
  }
  AnyIgniter->prec_state = AnyIgniter->state;
}




// *************************************************************
//   PrintNumber(num)
//   Convert integer
//
// *************************************************************
void PrintNumber(int v) {
  int ones;
  int tens;
  int hundreds;
  boolean negative;

  if (v < 0) {
    negative = true;
    ////lc.setChar(0, 3, '-', false);
    v = v * -1;
  }
  else {
    //print a blank in the sign column
    ////lc.setRow(0, 3, B00000000);
  }
  ones = v % 10;
  v = v / 10;
  tens = v % 10;
  v = v / 10;
  hundreds = v;

  //Now print the number digit by digit
  ////lc.setDigit(0, 2, (byte)hundreds, false);
  ////lc.setDigit(0, 1, (byte)tens, false);
  ////lc.setDigit(0, 0, (byte)ones, false);
}

// *************************************************************
//   PrintFreq(addr, frq)  //specific for freqency string handling
//   Loop for display all characters from frequency string, incoming from COM1 or 2
// *************************************************************
void PrintFreq(int lcdAddress, String freq) {
  ////lc.clearDisplay(lcdAddress);

  int iLen = freq.length();
  int i = 0;
  for (int j = 0; j < iLen; j++) {

    if (freq[i + 1] == '.') {
      ////lc.setChar(lcdAddress, iLen - 2 - j, freq[i], 1);
      i++;
    }
    else
      ////lc.setChar(lcdAddress, iLen - 2 - j, freq[i], 0);
    i++;
  }
}  //end f


// *************************************************************
//   RefreshDisplay(state vector)
//   handle all lcd refresh
// *************************************************************
/*void RefreshDisplay(uint8_t *radioStateVectorPtr)  {
  switch (radioStateVector[0]) {
    case 1: //if COM1 selected, print com1
      PrintFreq(0, freqComOneActiveString);
      PrintFreq(1, freqComOneSbString);
      break;
    case 2: //if COM2 selected, print com2
      PrintFreq(0, freqComTwoActiveString);
      PrintFreq(1, freqComTwoSbString);
      break;
    case 3: //if NAV1 selected, print NAV1
      PrintFreq(0, freqNavOneActiveString);
      PrintFreq(1, freqNavOneSbString);
      break;
    case 4: //if NAV2 selected, print NAV2
      PrintFreq(0, freqNavTwoActiveString);
      PrintFreq(1, freqNavTwoSbString);
      break;
    default:
      break;
  }
}  //end f

*/


// *************************************************************
//   ModeSelector(state, state vector ptr)
//   Change Radio selection on request
// *************************************************************
void ModeSelector(int state, uint8_t *radioStateVectorPtr) {

  radioStateVectorPtr[0] = state;
  //led status refresh
}


