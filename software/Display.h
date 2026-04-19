//Display controller class 

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "Arduino.h"
#include <ShiftLcd.h>

using namespace std;

//This class is used as an interface to handle the communication between the main program and the display, which is connected using a 74LS595 register and controlled with the ShiftLcd class 

class Display {
    private: 
    // Definition for the shift register/display connections 
    unsigned int lcd_en;
    unsigned int lcd_d7;
    unsigned int lcd_ser;
    unsigned int lcd_clk;
    unsigned int lcd_rows;
    unsigned int lcd_cols;
    unsigned int lcd_space;

    // array used for buffering the text on the screen 
    char** screen;

    // ShiftLcd class 
    ShiftLcd* lcd;

    public:
    /* class constructior and destructor
     * Need the pin connected to the display / shift register and the size fo the display in ROWS x COLS 
     */ 
    Display(unsigned int LCD_EN, unsigned int LCD_D7, unsigned int LCD_SER, unsigned int LCD_CLK, unsigned int ROWS, unsigned int COLS, unsigned int MAX_COLS);
    ~Display();
    
    /* clear() resets the display output */ 
    void clear();

    /* update() sends the characters contained int he screen buffer to the display itself */ 
    void update();

    /* write() is used for modifying the screen buffer with a specific string in given screen coordinates:
     * row -> row position for the first character 
     * col -> column opsition for the first character 
     * string -> tring to write in the buffer 
    */
    void write(unsigned int row,unsigned int col, String string);

};


#endif 
