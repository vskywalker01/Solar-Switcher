//Relays counter class definition 

#ifndef __RELAYS_H__
#define __RELAYS_H__

#include "Arduino.h"

typedef enum {START,STOP,OFF,ON} counterStatus;

//This class is used for tracking the counters for a fixed number of relays. Each counter can have four different states: 
// On       -> the counter is not active and the relay status is "on" 
// Off      -> the counter is not active and the relay status is "off"
// Start    -> the counter is counting in order to set the new status as "on" 
// Stop     -> the counter is counting in otder to set the new status as "off"
//To synchronize with the main program the class provides a method updateStatus(), which represents a step of the counters.  

class Relays {
    private: 
    unsigned int number;    //number of counters 
    unsigned int* counters; //counter current values 
    counterStatus* status;  //counter status array 
    bool* changedFlags;     //flag used to signal that a counter status is changed 

    public:
    // class constructiors and destructor 
    // relays -> number of counters 
    Relays(unsigned int relays);
    ~Relays();

    /* setCount() sets a new counter value and status for a single counter 
     * relay -> counter id 
     * value -> new value to set  
     * direction -> new status of the counter 
     */ 
    void setCount(unsigned int relay, unsigned int value,counterStatus direction);
    
    /* getCount() returns the current value of the counter 
     * relay -> counter id 
     * */ 
    unsigned int getCount(unsigned int relay);
    
    /* getDirection() gets the current counter status 
     * relay -> counter id 
    */ 
    counterStatus getDirection(unsigned int relay);

    /* getchanges returns the changed flag status (sets up when a counter reaches an ON/OFF status after a countdown) */ 
    bool getChanged(unsigned int relay);

    /* clearFlag() clears the changed flag */
    void clearFlag(unsigned int relay);
    
    /* updateStatus() performs a step for all the counters tracked */ 
    void updateStatus();

};



#endif 
