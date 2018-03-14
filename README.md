#last updated on 14-03-2018 
# people-tracking-system
People and asset tracking system.
 
Hardware support : 
   1. Happy Bubble Presence Detectors 
   2. Raspberry Pi 3
   3. ESP32 kit with Wifi and Bluetooth
   
ESP32 and HBPD used for bluetooth ibeacons tracking, hey detectos location advertisements from beacons
collect the information from ibeacon advertisements and form packet in json data format and sent it over MQTT to 
Raspberry Pi Server.
Raspberry Pi Server for people tracking is advance system which get the information on MQTT and process it, 
Get the important information and predict the location of beacons with advance algorithms like trilateration, 
kalmann filtering, fingerprinting.   