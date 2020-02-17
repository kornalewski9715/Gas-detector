# Gas-detector
Portable gas detector with WiFi and BT moduls and using MQTT protocol.

Used elements:

ESP8266 + NodeMCUv3 (microcontroller with wifi module), 
HC-05 (Bluetooth module), 
MQ-07 (Gas sensor), 
Buzzer, 
6xAA battery basket, 
L7805CV 

File MQ-7 data for CO contain data shows dependence between data read out at the input and score in ppm scale.
There has been created a trend function, which converts input data to ppm scale.
The spreadsheet has been created in LibreOfffice Calc.

Function: y=96.65730337850716*x^(-1.54022636537346)

y-ppm scale   
x- RS/RO

