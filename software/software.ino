// Implementation of main program 

#include "Display.h"
#include "Settings.h"
#include "Sensor.h"
#include "Relays.h"

#include <TimerOne.h>

/*
-- Overview --
An inverter with some photovoltaic panels is connected with two devices: an electic pump and a boiler. The goal is to create a system that, 
in base of the power that the system can generate from the sun, turns on and off the two devices.
Because cloud can create temporary shades on the panels, the system should also not consider occlusions for short amount of times (like 10 minutes). Therefore, a timer should be setted in order to activate or deactivate the relays. 

-- Hardware used --
- An arduino pro mini 
- one 16x2 LCD Display
- three buttons 
- one photoresistor 
- four relays controlled 

-- User interface --
The program include an interface that gives the user control over the settings and the current status of all four relays:
  - The main menu gives the status of the relays (ON/OFF) with the current power (estimated) that is available to all four devices. 

  *********************
  * luce: xxxx  xxxxw *
  * on  off  off  on  *
  *********************

  - A settings menu gives the user control over multipe features like the power used by the loads, output masks, or activation/deactivatiopn timers. 
  
  *********************
  * timer attivazione *
  * > 12s             *
  *********************

  - A details menu gives more information about the current status of each single relay 

  **********************
  * carico 1           *
  * attivazione in xxs *
  **********************

-- relay control system -- 
The four relays have a fixed priority order (relay 1 has the highest priority and relay 4 the least). The system activates first the relays with highest priority and, if th epower is enough for the others, the ones with lowest priority.  

-- light estimation -- 
The estimation of the available power is performes using an exponential model (see Sensor.h)
*/


// ------ DEFINITIONS  ------

//pins connected to the display (see Display.h)
#define LCD_SHIFT_EN              4 
#define LCD_SHIFT_D7              5
#define LCD_SHIFT_SER             6
#define LCD_SHIFT_CLK             7

//display format 
#define LCD_FORMAT_ROWS           2
#define LCD_FORMAT_COLS           20
#define LCD_MAX_COLS              38

//settings EEPROM base address
#define SETTINGS_ADDRESS          0x00

//Sensor pin and constants 
#define PHOTO_PIN                 A0
#define SENSOR_ALPHA_DELTA        1
#define SENSOR_BETA_DELTA         0.1
#define SENSOR_GAMMA_DELTA        1
#define POWER_DELTA               10
//Button pins 
#define SELECT_BUTTON             2
#define MINUS_BUTTON              8
#define PLUS_BUTTON               9

//Relay pins 
#define RELAY1_PIN                1
#define RELAY2_PIN                0
#define RELAY3_PIN                11  
#define RELAY4_PIN                10

//Interface options (values are in microseconds) 
#define DISPLAY_REFRESH_RATE      100000
#define BUTTON_RESOLUTION         2000000
#define BUTTON_FAST_RESOLUTION    100000

//button status enum 
typedef enum {
  CONTROL,
  MINUS,
  PLUS,
  NONE
} button;

//current view enum 
typedef enum {
  GENERAL,
  SETTINGS_POWER,
  SETTINGS_MASK,
  SETTINGS_TIMERON,
  SETTINGS_TIMEROFF,
  SETTINGS_SENSOR
} view;

//global variables  
Display* screen;   //display object 
Settings* options; //settings object 
Sensor* light;     //sensor object 
Relays* counters;  //conters object 

view currentView;                //contains the current menu 
bool blinkStatus;                //used for blinking arrows near the relay status in the main menu 
unsigned int secondCounter;      //used for counting a second from the display interrupts from TimerOne 
unsigned int buttonCounter;      //used for managing the button polling rate 
unsigned short currentSubView;   //sub view used in the menu 
button lastButton;               //last valid button readed  

void update();
void loop() {}

void setup() {
  noInterrupts(); 

  //Pin mode init 
  pinMode(SELECT_BUTTON, INPUT);
  pinMode(MINUS_BUTTON, INPUT);
  pinMode(PLUS_BUTTON, INPUT);
  pinMode(PHOTO_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  //first deactivation of the relays 
  digitalWrite(RELAY1_PIN, true);
  digitalWrite(RELAY2_PIN, true);
  digitalWrite(RELAY3_PIN, true);
  digitalWrite(RELAY4_PIN, true);
  lastButton=NONE;
  currentView=GENERAL;
  blinkStatus=false;

  //the second counter is setted to match exactly one second in base of the display interrupts 
  secondCounter=1000000/DISPLAY_REFRESH_RATE;
  buttonCounter=BUTTON_RESOLUTION/DISPLAY_REFRESH_RATE;

  screen=new Display(LCD_SHIFT_EN, LCD_SHIFT_D7, LCD_SHIFT_SER, LCD_SHIFT_CLK,LCD_FORMAT_ROWS,LCD_FORMAT_COLS,LCD_MAX_COLS);
  options=new Settings();
  counters=new Relays(4);
  
  if(!options->loadSaved(SETTINGS_ADDRESS)) {
    //If the settings saved in the EEPROM are not valid, prints an error on the display 
    screen->clear();
    screen->write(0,0,"Impostazioni non");
    screen->write(1,0,"valide");
    screen->update();
    interrupts();
    delay(2000);
    noInterrupts();
    screen->clear();
  }
  light=new Sensor(PHOTO_PIN,options->getAlpha(),options->getBeta(),options->getGamma());
 
  //interrupt intialization (display refresh rate)
  Timer1.initialize((DISPLAY_REFRESH_RATE/2));
  Timer1.attachInterrupt(update);

  interrupts(); 
}

//timerOne interrupt (display refresh and control system)
void update() {
  //gets the current available light 
  light->readSample();
  unsigned int currentPower=light->getCurrentPower();
  unsigned int currentLight=light->getCurrentValue(); 
  //If one second is elapsed, check the counter status for updating the relays, otherwise the second counter is decremented. 
  if (secondCounter>0) {
    secondCounter--;
  } else {
    //secondCounter reset 
    secondCounter=1000000/DISPLAY_REFRESH_RATE;
    
    //blink arrow update and counter step 
    blinkStatus=!blinkStatus;
    counters->updateStatus();

    //the currentPower value is substracted with the power of the loads to estimate which relay should be modified 
    //(the priority is managed by modifying the leftPowr variable before considering the next relay)
    int leftPower=currentPower;
    for (int l=0;l<LOADS_NUMBER;l++) {
      //if the relay is not masked, leftPower is substracted with the power used by the load
      if (!options->getMask(l)) {
        leftPower-=options->getPower(l);
        switch (counters->getDirection(l)) {
          case STOP:
            //if relay is turning off but there is enough light -> the relay is maintained on 
            if (leftPower>0) counters->setCount(l,0,ON);
          break;
          case START:
            //if the relay is turning on but there is not enough light -> the relay is maintained off 
            if (leftPower<0) counters->setCount(l,0,OFF);
          break;
          case ON:
            //if the relay is on but there is not enough light -> the relay counter is started to deactivate the relay 
            if (leftPower<0) counters->setCount(l,options->getTimerOff(),STOP);
          break;
          case OFF:
            //if the relay is off but there is enough light -> the relay counter is started to activate the relay 
            if (leftPower>0) counters->setCount(l,options->getTimerOn(),START);
          break;
        }
      } else {
        //if the device is masked, its priority is ignored and the relay remains off.
        counters->setCount(l,0,OFF);
      }
      counters->clearFlag(l);
    }
    
    // actual pin update 
    if (counters->getDirection(0)==START || counters->getDirection(0)==OFF) digitalWrite(RELAY1_PIN,true);
    else digitalWrite(RELAY1_PIN,false);
    if (counters->getDirection(1)==START || counters->getDirection(1)==OFF) digitalWrite(RELAY2_PIN,true);
    else digitalWrite(RELAY2_PIN,false);
    if (counters->getDirection(2)==START || counters->getDirection(2)==OFF) digitalWrite(RELAY3_PIN,true);
    else digitalWrite(RELAY3_PIN,false);
    if (counters->getDirection(3)==START || counters->getDirection(3)==OFF) digitalWrite(RELAY4_PIN,true);
    else digitalWrite(RELAY4_PIN,false);

  }

  /* button debouncing system with dynamic resolution */ 

  button detectedButton=NONE;
  button pressedButton=NONE;
  if (!digitalRead(PLUS_BUTTON)) pressedButton=PLUS;
  if (!digitalRead(MINUS_BUTTON)) pressedButton=MINUS;
  if (!digitalRead(SELECT_BUTTON)) pressedButton=CONTROL;
  
  //if the button is pressed and the button counter is off, the button counter is setted. 
  if (buttonCounter==0 || pressedButton != lastButton) {
    //if the last valid button is equal to the pressed one, the polling rate is increased 
    if (lastButton!=pressedButton) buttonCounter=BUTTON_RESOLUTION/DISPLAY_REFRESH_RATE;
    else buttonCounter=BUTTON_FAST_RESOLUTION/DISPLAY_REFRESH_RATE;
    lastButton=pressedButton;
  } else {
    //if the button counter is not zero, the variable is decremented. 
    pressedButton=NONE; 
    buttonCounter--;
  }
  
  /* interface management */ 
  switch (currentView) {
    case GENERAL:
      //If PLUS button is pressed -> go to the next detail page  
      if (pressedButton==PLUS) currentSubView++;

      //If CONTROL button is pressed -> go to the settings page 
      if (pressedButton==CONTROL) {
        if (currentSubView==0) {  
          currentView=SETTINGS_POWER;
        } else {
          //If it is pressed in a detail page -> return to the general view 
          currentSubView=0;
        }
      }
      if (currentSubView==0) {
        //Printing power and light values on the first row 
        screen->write(0,0,"Sole:                ");
        screen->write(0,6,String(currentLight)+"     ");
        screen->write(0,11,String((char) 0b00010000)+" ");
        screen->write(0,13,String(currentPower)+"W   ");

        //Printing relay status on the second row 
        for (unsigned int l=0;l<LOADS_NUMBER ;l++) {
          switch (counters->getDirection(l)) {
            case ON:
              screen->write(1,(l*5),"ON   ");
            break;
            case OFF:
              screen->write(1,(l*5),"OFF  ");
            break;
            case STOP:
              screen->write(1,(l*5),"ON   ");
              if(blinkStatus) {
                screen->write(1,2+(l*5),String((char) 0b00011111));
              } else {
                screen->write(1,2+(l*5),String("  "));
              }
            break;
            case START:
              screen->write(1,(l*5),"OFF  ");
              if(blinkStatus) {
                screen->write(1,3+(l*5),String((char) 0b00011110));
              } else {
                screen->write(1,3+(l*5),String("  "));
              }
            break;
          }
        }

      } else {
        //Details views shows a single relay status. 
        if (currentSubView>LOADS_NUMBER) {
          currentSubView=0;
          currentView=GENERAL;
        } else {

          //Printing the current relay Id and status in the first row and the countdown in the second row (if started) 
          screen->write(0,0,"Porta "+String(currentSubView)+":            ");

          
          switch(counters->getDirection(currentSubView-1)) {
            case ON:
              screen->write(0,10,"ON ");
              screen->write(1,0,"                    ");
              break;
            case STOP:
              screen->write(0,10,"ON ");
              screen->write(1,0,"Spegnimento in "+String(counters->getCount(currentSubView-1))+"    ");
            break;
            
            case OFF:
              screen->write(0,10,"OFF");
              //If the relay is masked, prints "masked in the second row"
              if (options->getMask(currentSubView-1)) {
                screen->write(1,0,"Mascherato          ");
              } else {
                screen->write(1,0,"                    ");
              }
              break;
            case START:
              screen->write(0,10,"OFF");
              screen->write(1,0,"Accensione in "+String(counters->getCount(currentSubView-1))+"    ");
            break;
          }
        }
      }
    break;
    case SETTINGS_MASK:
      switch (pressedButton) {
        case PLUS:
          if (currentSubView==0) currentSubView=1;
          else if (currentSubView<=LOADS_NUMBER) options->setMask((currentSubView-1),true);
        break;

        case MINUS:
          if (currentSubView==0) currentSubView=1;
          else if (currentSubView<=LOADS_NUMBER) options->setMask((currentSubView-1),false);
        break;

        case CONTROL:
          if (currentSubView>0) {
            if (currentSubView>=LOADS_NUMBER) {
              currentSubView=0;
              currentView=SETTINGS_SENSOR;
            }
            else {
              currentSubView++;
            }
          } else {
            currentSubView=0;
            currentView=SETTINGS_SENSOR;
          }
        break;
      }
      if (currentSubView==0) {
        screen->write(0,0,"Maschere            ");
        screen->write(1,0,"                    ");
      } else {
        screen->write(0,0,"Maschera carico "+String(currentSubView)+"     ");
        screen->write(1,0,String((char) 0b00111110)+"                   ");
        if (options->getMask(currentSubView-1)) {
          screen->write(1,2,String("ON "));
        } else {
          screen->write(1,2,String("OFF "));
        }
        
      }
    break;

    case SETTINGS_POWER:
      switch (pressedButton) {
        case PLUS:
          if (currentSubView==0) currentSubView=1;
          else if (currentSubView<=LOADS_NUMBER) options->setPower((currentSubView-1),options->getPower(currentSubView-1)+POWER_DELTA);
        break;

        case MINUS:
          if (currentSubView==0) currentSubView=1;
          else if (currentSubView<=LOADS_NUMBER) options->setPower((currentSubView-1),options->getPower(currentSubView-1)-POWER_DELTA);
        break;

        case CONTROL:
          if (currentSubView>0) {
            if (currentSubView>=LOADS_NUMBER) {
              currentSubView=0;
              currentView=SETTINGS_MASK;
            }
            else {
              currentSubView++;
            }
          } else {
            currentSubView=0;
            currentView=SETTINGS_MASK;
          }
        break;
      }
      if (currentSubView==0) {
        screen->write(0,0,"Potenza carichi     ");
        screen->write(1,0,"                    ");
      } else {
        screen->write(0,0,"Potenza carico "+String(currentSubView)+"     ");
        screen->write(1,0,String((char) 0b00111110)+"                   ");
        screen->write(1,2,String(options->getPower(currentSubView-1))+"    ");
      }
    break;

    case SETTINGS_TIMERON:
      switch (pressedButton) {
        case PLUS:
          options->setTimerOn(options->getTimerOn()+1);
        break;

        case MINUS:
          options->setTimerOn(options->getTimerOn()-1);
        break;

        case CONTROL:
          currentView=SETTINGS_TIMEROFF;
        break;
      }
      screen->write(0,0,"Timer allaccio      ");
      screen->write(1,0,String((char) 0b00111110)+"                   ");
      screen->write(1,2,String(options->getTimerOn())+"    ");
    
    break;

    case SETTINGS_TIMEROFF:
      switch (pressedButton) {
        case PLUS:
          options->setTimerOff(options->getTimerOff()+1);
        break;

        case MINUS:
          options->setTimerOff(options->getTimerOff()-1);
        break;

        case CONTROL:
          screen->clear();
          screen->update();
          currentView=GENERAL;
          currentSubView=0;
          options->store(SETTINGS_ADDRESS);
        break;
      }
      screen->write(0,0,"Timer distacco      ");
      screen->write(1,0,String((char) 0b00111110)+"                   ");
      screen->write(1,2,String(options->getTimerOff())+"    ");
    
    break;
    case SETTINGS_SENSOR:
      switch (pressedButton) {
        case PLUS:
          switch (currentSubView) {  
            case 0: 
                currentSubView=1; 
            break; 
            case 1: 
                options->setAlpha(options->getAlpha()+SENSOR_ALPHA_DELTA);
                light->setAlpha(options->getAlpha());
            break;
            case 2: 
                options->setBeta(options->getBeta()+SENSOR_BETA_DELTA);
                light->setBeta(options->getBeta());
            
            case 3: 
                options->setGamma(options->getGamma()+SENSOR_GAMMA_DELTA);
                light->setGamma(options->getGamma());


            default: 
            break;
          }
        break;

        case MINUS:
          switch (currentSubView) {  
            case 0: 
                currentSubView=1; 
            break; 
            case 1: 
                options->setAlpha(options->getAlpha()-SENSOR_ALPHA_DELTA);
                light->setAlpha(options->getAlpha());
            break;
            case 2: 
                options->setBeta(options->getBeta()-SENSOR_BETA_DELTA);
                light->setBeta(options->getBeta());
            
            case 3: 
                options->setGamma(options->getGamma()-SENSOR_GAMMA_DELTA);
                light->setGamma(options->getGamma());

            default: 
            break;
          }  
        break;

        case CONTROL:
          if (currentSubView>0) {
            if (currentSubView>=3) {
              currentSubView=0;
              currentView=SETTINGS_TIMERON;
            }
            else {
              currentSubView++;
            }
          } else {
            currentSubView=0;
            currentView=SETTINGS_TIMERON;
          }
        break;
      }
      switch (currentSubView) {
        case 0: 
          screen->write(0,0,"Calibrazione sensore");
          screen->write(1,0,"                    ");
        break; 

        case 1:
          screen->write(0,0,"Alpha               ");
          screen->write(1,0,String((char) 0b00111110)+"                   ");
          screen->write(1,2,String(options->getAlpha()));
        break; 

        case 2: 
          screen->write(0,0,"Beta                ");
          screen->write(1,0,String((char) 0b00111110)+"                   ");
          screen->write(1,2,String(options->getBeta()));
        break;
        
        case 3: 
          screen->write(0,0,"Gamma                ");
          screen->write(1,0,String((char) 0b00111110)+"                   ");
          screen->write(1,2,String(options->getGamma()));
        default: 
        break;
      }
    break;

    default:
      currentView=GENERAL;
    break;
  }
  screen->update();
}
