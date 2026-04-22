# Solar-Switcher
This repository contians the source code and pcb design for an automatic load switcher that controls high power consumption devices in base of the energy that a photovoltaic system can currently generate.

## How it works 
The switcher contains four different relays controlled by an Arduino Pro Mini that collects periodically samples from a photo-resistor and converts them into an estimated power (the power that the inverter should generate at the time). 
In base of a fixed priority order, the controller turns on and off the relays so that the overall consumptions fits into the estimated power.

<img width="322" height="381" align="center" alt="schematic" src="https://github.com/user-attachments/assets/2def72c3-0a4f-4ce9-832c-5d227f79d7b1" />

