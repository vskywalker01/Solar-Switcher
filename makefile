PRJ = ./software 
DEV = arduino:avr
BRD = pro
PORT = /dev/ttyUSB0 
BAUD = 115200 

compile: 
	arduino-cli core install $(DEV)
	arduino-cli lib install ShiftLcd
	arduino-cli lib install TimerOne
	arduino-cli compile --fqbn $(DEV):$(BRD) $(PRJ)

run: compile
	arduino-cli upload -p $(PORT) --fqbn $(DEV):$(BRD) $(PRJ)

