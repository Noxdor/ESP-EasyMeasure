# HermosEasyMeasure
An esp32 project, that provides a quickly accessible temperature and humidity measurement over a webserver running on an esp32. The program includes a logging module, saving the measured data to a logfile (.csv). The interval between measurements can be changed, as well as the logfile name. The esp can run in one of two modes.
In the first mode, an interval measurement is taken. The esp32 goes to sleep between measurements to save energy. It can be woken up by an external input via a button connected to PIN 23.
