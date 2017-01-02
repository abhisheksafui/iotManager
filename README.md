# iotManager

A Mosquitto based simple IOT device manager and scheduler. It can be run on any linux platform.
Supports both local control(in case of no internet), or remote control through internet.

Divided into two processes: 
iotserver: Discovers new iot devices as an when plugged in. Brodcasts notification events to subscribers.
           Queues SET GET requests for iot devices. Exchanges hello messages with iot devices. 
           Supports iot switches, Sensors. More devices to be added. 
           
iotscheduler: saves time based operation requests from user. schedules time based events for iot devices.


Hardware needs to follow the JSON based message interface between IOT DEVICE and IOT MANAGER, to join the network.

Android App needs to follow a JSON based message interface between MOBILE DEVICE and IOT MANAGER. 
