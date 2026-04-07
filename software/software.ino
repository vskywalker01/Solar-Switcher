#include "Display.h"
#include "Settings.h"
#include "Sensor.h"
#include "Relays.h"

#include <TimerOne.h>
#include <Thread.h>
#include <ThreadController.h>

#define LCD_SHIFT_EN              4
#define LCD_SHIFT_D7              5
#define LCD_SHIFT_SER             6
#define LCD_SHIFT_CLK             7

#define LCD_FORMAT_ROWS           2
#define LCD_FORMAT_COLS           20
#define LCD_MAX_COLS              38
#define SETTINGS_ADDRESS          10

#define PHOTO_PIN                 A0
#define PHOTO_BASEVALUE           0
#define PHOTO_INCREMENT           10

#define SELECT_BUTTON             2
#define MINUS_BUTTON              8
#define PLUS_BUTTON               9

#define RELAY1_PIN                0
#define RELAY2_PIN                1
#define RELAY3_PIN                11  
#define RELAY4_PIN                10

#define BUZZER_PIN                12
#define BUZZER_FREQUENCY          3500

#define DISPLAY_REFRESH_RATE      100000
#define BUTTON_RESOLUTION         (2000/50)
#define BUTTON_FAST_RESOLUTION    1

#define BUTTON_PRESSED_TONE       200
#define ALARM_TONE                5000

typedef enum {CONTROL,MINUS,PLUS,NONE} button;
typedef enum {
  GENERAL,
  SETTINGS_POWER,
  SETTINGS_MASK,
  SETTINGS_TIMERON,
  SETTINGS_TIMEROFF,
  SETTINGS_BUZZER
} view;

Display* screen;
Settings* options;
Sensor* light;
Relays* counters;

view currentView;
bool blinkStatus;
bool changes;
unsigned int secondCounter;
unsigned int buttonCounter;
unsigned short currentSubView;
button lastButton;

void buttonThreadCallback(); 
void outputThreadCallback();
void threadsCallback(); 


ThreadController threads = ThreadController();
Thread buttonThread = Thread(); 
Thread outputThread = Thread();



void setup() {
  noInterrupts(); 

  pinMode(SELECT_BUTTON, INPUT);
  pinMode(MINUS_BUTTON, INPUT);
  pinMode(PLUS_BUTTON, INPUT);
  pinMode(PHOTO_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY1_PIN, true);
  digitalWrite(RELAY2_PIN, true);
  digitalWrite(RELAY3_PIN, true);
  digitalWrite(RELAY4_PIN, true);
  lastButton=NONE;
  currentView=GENERAL;
  currentSubView=0;
  blinkStatus=false;
  //secondCounter=1000000/DISPLAY_REFRESH_RATE;
  buttonCounter=BUTTON_RESOLUTION;///DISPLAY_REFRESH_RATE;
  changes=false;

  screen=new Display(LCD_SHIFT_EN, LCD_SHIFT_D7, LCD_SHIFT_SER, LCD_SHIFT_CLK,LCD_FORMAT_ROWS,LCD_FORMAT_COLS,LCD_MAX_COLS);
  options=new Settings();
  light=new Sensor(PHOTO_PIN,PHOTO_BASEVALUE,PHOTO_INCREMENT);
  counters=new Relays(4);
  
  if(!options->loadSaved(SETTINGS_ADDRESS)) {
    screen->clear();
    screen->write(0,0,"Impostazioni non");
    screen->write(1,0,"valide");
    screen->update();
    interrupts();
    delay(2000);
    noInterrupts();
    screen->clear();
  }
  
  buttonThread.onRun(buttonThreadCallback);
  outputThread.onRun(outputThreadCallback);
  buttonThread.setInterval(BUTTON_FAST_RESOLUTION);
  outputThread.setInterval(1000);
  threads.add(&buttonThread);
  threads.add(&outputThread);

  Timer1.initialize(50000);
  Timer1.attachInterrupt(threadsCallback);
  interrupts(); 
}

void threadsCallback() {
  threads.run();
}

void buttonThreadCallback() {
  button detectedButton=NONE;
  button pressedButton=NONE;
  if (!digitalRead(PLUS_BUTTON)) pressedButton=PLUS;
  if (!digitalRead(MINUS_BUTTON)) pressedButton=MINUS;
  if (!digitalRead(SELECT_BUTTON)) pressedButton=CONTROL;
  
  if (buttonCounter==0 || pressedButton != lastButton) {
    if (lastButton!=pressedButton) buttonCounter=BUTTON_RESOLUTION;
    lastButton=pressedButton;
  } else {
    pressedButton=NONE; 
    buttonCounter--;
  }
  switch (currentView) {
    case GENERAL:
      if (pressedButton==PLUS) currentSubView++;
      if (pressedButton==CONTROL) {
        if (currentSubView==0) {  
          currentView=SETTINGS_POWER;
        } else {
          currentSubView=0;
        }
      }
    break;
    case SETTINGS_MASK:
      switch (pressedButton) {
        case PLUS:
        case MINUS: 
          if (currentSubView==0) currentSubView=1;
          else if (currentSubView<=LOADS_NUMBER) options->setMask((currentSubView-1),true);
        break;

        case CONTROL:
          if (currentSubView>0) {
            if (currentSubView>=LOADS_NUMBER) {
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
    break;

    case SETTINGS_POWER:
      switch (pressedButton) {
        case PLUS:
          if (currentSubView==0) currentSubView=1;
          else if (currentSubView<=LOADS_NUMBER) options->setPower((currentSubView-1),options->getPower(currentSubView-1)+1);
        break;

        case MINUS:
          if (currentSubView==0) currentSubView=1;
          else if (currentSubView<=LOADS_NUMBER) options->setPower((currentSubView-1),options->getPower(currentSubView-1)-1);
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
          currentView=SETTINGS_BUZZER;
        break;
      }
    break;

    case SETTINGS_BUZZER:
      switch (pressedButton) {
        case PLUS:
          options->setBuzzer(true);
        break;

        case MINUS:
          options->setBuzzer(false);
        break;

        case CONTROL:
          screen->clear();
          screen->update();
          currentView=GENERAL;
          currentSubView=0;
          options->store(SETTINGS_ADDRESS);
        break;
      }
    break;

    default:
      currentView=GENERAL;
    break;
  }

}

void outputThreadCallback() {
    blinkStatus=!blinkStatus;
    counters->updateStatus();

    int leftPower=light->getCurrentPower();
    for (int l=0;l<LOADS_NUMBER;l++) {
      if (!options->getMask(l)) {
        leftPower-=options->getPower(l);
        switch (counters->getDirection(l)) {
          case STOP:
            if (leftPower>0) counters->setCount(l,0,ON);
          break;
          case START:
            if (leftPower<0) counters->setCount(l,0,OFF);
          break;
          case ON:
            if (leftPower<0) counters->setCount(l,options->getTimerOff(),STOP);
            changes=changes || counters->getChanged(l);
          break;
          case OFF:
            if (leftPower>0) counters->setCount(l,options->getTimerOn(),START);
            changes=changes || counters->getChanged(l);
          break;
        }
      } else {
        counters->setCount(l,0,OFF);
      }
      counters->clearFlag(l);
    }

    if (counters->getDirection(0)==START || counters->getDirection(0)==OFF) digitalWrite(RELAY1_PIN,true);
    else digitalWrite(RELAY1_PIN,false);
    if (counters->getDirection(1)==START || counters->getDirection(1)==OFF) digitalWrite(RELAY2_PIN,true);
    else digitalWrite(RELAY2_PIN,false);
    if (counters->getDirection(2)==START || counters->getDirection(2)==OFF) digitalWrite(RELAY3_PIN,true);
    else digitalWrite(RELAY3_PIN,false);
    if (counters->getDirection(3)==START || counters->getDirection(3)==OFF) digitalWrite(RELAY4_PIN,true);
    else digitalWrite(RELAY4_PIN,false);
}

void loop() {
  if (changes && options->getBuzzer()) {
    noInterrupts(); 
    tone(BUZZER_PIN,BUZZER_FREQUENCY,5000);
    changes=false;
    Timer1.initialize(50000);
    Timer1.attachInterrupt(threadsCallback);
    interrupts(); 
  }

  switch (currentView) {
    case GENERAL:
       if (currentSubView==0) {
        screen->write(0,0,"Potenza:  ");
        screen->write(0,10,String(light->getCurrentPower())+"W    ");
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
        if (currentSubView>LOADS_NUMBER) {
          currentSubView=0;
          currentView=GENERAL;
        } else {
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
      screen->write(0,0,"Timer allaccio      ");
      screen->write(1,0,String((char) 0b00111110)+"                   ");
      screen->write(1,2,String(options->getTimerOn())+"    ");
    
    break;

    case SETTINGS_TIMEROFF:
      screen->write(0,0,"Timer distacco      ");
      screen->write(1,0,String((char) 0b00111110)+"                   ");
      screen->write(1,2,String(options->getTimerOff())+"    ");
    
    break;

    case SETTINGS_BUZZER:
      screen->write(0,0,"Avvisi sonori       ");
      screen->write(1,0,String((char) 0b00111110)+"                   ");
      if (options->getBuzzer()) {
        screen->write(1,2,"OFF                ");
      } else {
        screen->write(1,2,"ON                ");
      }
    
    break;
  }
  screen->update();
}

/*
void update();

view currentView;
bool blinkStatus;
unsigned int secondCounter;
unsigned int buttonCounter;
unsigned short currentSubView;
button lastButton;

void setup() {
  noInterrupts(); 

  pinMode(SELECT_BUTTON, INPUT);
  pinMode(MINUS_BUTTON, INPUT);
  pinMode(PLUS_BUTTON, INPUT);
  pinMode(PHOTO_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY1_PIN, true);
  digitalWrite(RELAY2_PIN, true);
  digitalWrite(RELAY3_PIN, true);
  digitalWrite(RELAY4_PIN, true);
  lastButton=NONE;
  currentView=GENERAL;
  blinkStatus=false;
  secondCounter=1000000/DISPLAY_REFRESH_RATE;
  buttonCounter=BUTTON_RESOLUTION/DISPLAY_REFRESH_RATE;

  screen=new Display(LCD_SHIFT_EN, LCD_SHIFT_D7, LCD_SHIFT_SER, LCD_SHIFT_CLK,LCD_FORMAT_ROWS,LCD_FORMAT_COLS,LCD_MAX_COLS);
  options=new Settings();
  light=new Sensor(PHOTO_PIN,PHOTO_BASEVALUE,PHOTO_INCREMENT);
  counters=new Relays(4);
  
  if(!options->loadSaved(SETTINGS_ADDRESS)) {
    screen->clear();
    screen->write(0,0,"Impostazioni non");
    screen->write(1,0,"valide");
    screen->update();
    interrupts();
    delay(2000);
    noInterrupts();
    screen->clear();
  }
  
  Timer1.initialize((DISPLAY_REFRESH_RATE/2));
  Timer1.attachInterrupt(update);

  interrupts(); 
}
bool changes;
void loop() {

}

void update() {
  unsigned int currentPower=light->getCurrentPower();
  
  if (secondCounter>0) {
    secondCounter--;
  } else {
    secondCounter=1000000/DISPLAY_REFRESH_RATE;
    blinkStatus=!blinkStatus;
    counters->updateStatus();

    int leftPower=currentPower;
    for (int l=0;l<LOADS_NUMBER;l++) {
      if (!options->getMask(l)) {
        leftPower-=options->getPower(l);
        switch (counters->getDirection(l)) {
          case STOP:
            if (leftPower>0) counters->setCount(l,0,ON);
          break;
          case START:
            if (leftPower<0) counters->setCount(l,0,OFF);
          break;
          case ON:
            if (leftPower<0) counters->setCount(l,options->getTimerOff(),STOP);
            changes=changes || counters->getChanged(l);
          break;
          case OFF:
            if (leftPower>0) counters->setCount(l,options->getTimerOn(),START);
            changes=changes || counters->getChanged(l);
          break;
        }
      } else {
        counters->setCount(l,0,OFF);
      }
      counters->clearFlag(l);
    }

    if (counters->getDirection(0)==START || counters->getDirection(0)==OFF) digitalWrite(RELAY1_PIN,true);
    else digitalWrite(RELAY1_PIN,false);
    if (counters->getDirection(1)==START || counters->getDirection(1)==OFF) digitalWrite(RELAY2_PIN,true);
    else digitalWrite(RELAY2_PIN,false);
    if (counters->getDirection(2)==START || counters->getDirection(2)==OFF) digitalWrite(RELAY3_PIN,true);
    else digitalWrite(RELAY3_PIN,false);
    if (counters->getDirection(3)==START || counters->getDirection(3)==OFF) digitalWrite(RELAY4_PIN,true);
    else digitalWrite(RELAY4_PIN,false);

  }

  button detectedButton=NONE;
  button pressedButton=NONE;
  if (!digitalRead(PLUS_BUTTON)) pressedButton=PLUS;
  if (!digitalRead(MINUS_BUTTON)) pressedButton=MINUS;
  if (!digitalRead(SELECT_BUTTON)) pressedButton=CONTROL;
  
  if (buttonCounter==0 || pressedButton != lastButton) {
    if (lastButton!=pressedButton) buttonCounter=BUTTON_RESOLUTION/DISPLAY_REFRESH_RATE;
    else buttonCounter=BUTTON_FAST_RESOLUTION/DISPLAY_REFRESH_RATE;
    lastButton=pressedButton;
  } else {
    pressedButton=NONE; 
    buttonCounter--;
  }
   
  switch (currentView) {
    case GENERAL:
      if (pressedButton==PLUS) currentSubView++;
      if (pressedButton==CONTROL) {
        if (currentSubView==0) {  
          currentView=SETTINGS_POWER;
        } else {
          currentSubView=0;
        }
      }
      if (currentSubView==0) {
        screen->write(0,0,"Potenza:  ");
        screen->write(0,10,String(light->getCurrentPower())+"W    ");
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
        if (currentSubView>LOADS_NUMBER) {
          currentSubView=0;
          currentView=GENERAL;
        } else {
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
          else if (currentSubView<=LOADS_NUMBER) options->setPower((currentSubView-1),options->getPower(currentSubView-1)+1);
        break;

        case MINUS:
          if (currentSubView==0) currentSubView=1;
          else if (currentSubView<=LOADS_NUMBER) options->setPower((currentSubView-1),options->getPower(currentSubView-1)-1);
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
          currentView=SETTINGS_BUZZER;
        break;
      }
      screen->write(0,0,"Timer distacco      ");
      screen->write(1,0,String((char) 0b00111110)+"                   ");
      screen->write(1,2,String(options->getTimerOff())+"    ");
    
    break;

    case SETTINGS_BUZZER:
      switch (pressedButton) {
        case PLUS:
          options->setBuzzer(true);
        break;

        case MINUS:
          options->setBuzzer(false);
        break;

        case CONTROL:
          screen->clear();
          screen->update();
          currentView=GENERAL;
          currentSubView=0;
          options->store(SETTINGS_ADDRESS);
        break;
      }
      screen->write(0,0,"Avvisi sonori       ");
      screen->write(1,0,String((char) 0b00111110)+"                   ");
      if (options->getBuzzer()) {
        screen->write(1,2,"OFF                ");
      } else {
        screen->write(1,2,"ON                ");
      }
    
    break;

    default:
      currentView=GENERAL;
    break;
  }
  screen->update();
}*/
