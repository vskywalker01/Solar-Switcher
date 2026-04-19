#include "Display.h"
#include "Arduino.h"

using namespace std;

Display::Display(unsigned int LCD_EN, unsigned int LCD_D7, unsigned int LCD_SER, unsigned int LCD_CLK, unsigned int ROWS, unsigned int COLS, unsigned int MAX_COLS) {
    lcd_en=LCD_EN;
    lcd_d7=LCD_D7;
    lcd_ser=LCD_SER;
    lcd_clk=LCD_CLK;
    lcd_rows=ROWS;
    lcd_cols=COLS;
    lcd_space=MAX_COLS;
    
    //screen buffer initialization with blank characters 
    if (ROWS>0) {
        screen= new char*[ROWS];
        for (int row=0;row<ROWS;row++) {
            screen[row]=new char[lcd_space+1];
        }
        
    } else {
        screen=NULL;
    }
    lcd=new ShiftLcd(lcd_en, lcd_d7, lcd_ser, lcd_clk);
    lcd->begin();
    clear();
    
}
Display::~Display() {
    if (screen!=NULL) {
        for (int row=0;row<lcd_rows;row++) {
            delete [] screen[row];
        }
        delete [] screen;
    }
    delete [] lcd;

}

void Display::clear() {
    // cleaning buffer 
    lcd->clear();
    for (int row=0;row<lcd_rows;row++) {
        for (int col=0;col<lcd_space-1;col++) {
            screen[row][col]=' ';
        }
        screen[row][lcd_space-1]='\n';
    }
}
void Display::update() {
    if (screen==NULL) return;
    if (lcd_rows<2) {
        lcd->write(screen[0]);
    } else {
        lcd->write(screen[0],screen[1]);
    }
}

void Display::write(unsigned int row,unsigned int col, String string) {
    // if the row is not valid the function returns 
    if (row>=lcd_rows) return;
    unsigned int string_pos=0;

    //the buffer is updated untile the string or the buffer terminate 
    while(string_pos<string.length() && col<lcd_space) {
        screen[row][col]=string.charAt(string_pos);
        string_pos++;
        col++;
    }
}
