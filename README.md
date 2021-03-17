# ESP-EasyMeasure
An esp32 project that provides a quickly accessible temperature and humidity measurement over a webserver running on an esp32.
## Description
The program includes a logging module, saving the measured data to a logfile (.csv). The interval between measurements can be changed as well as the logfile name. The esp can run in one of two modes. The big advantage of this project is, that it can run autonomously without an internet connection. The time it get's from the connected RTC-Module. Data it saves in the internal SPIFFS file system. With this advantage it can be deployed anywhere by connecting it to a power bank or battery.
## Modes
### Intervall Measurement
In the first mode, an interval measurement is taken. Upfront it is possible to name the file, to which the measurements per intervall should be saved. Also the delay between measurements can be defined upfront. The esp32 goes to sleep between measurements to save energy. It can be woken up by an external input via a button connected to PIN 23. This Pin can be changed in the `ESPEasyMeasure.c++` File.
After initializing the intervall measurement, you can see the already measured values in a table. The table is provided by derekeder.github.io/csv-to-html-table/ with his amazingly simple html table to display csv values.
### Active Measurement
In the second mode, an active measurement is taken every 5 seconds (default). The time can be changed in the ./data/aktiv.html file. In the script tag you'll find the constant variable `update_time` to change the time between measurements. Be careful to not set it too low, otherwise the DHT-Sensor's readings might be corrupted. The measured temperature and humidity is shown on the webpage.

## Installation
You need the esp32-libraries provided by Expressif. The easiest install is by adding a single line in the Arduino IDE. (https://www.youtube.com/watch?v=mBaS3YnqDaU&ab_channel=RuiSantos)
Furthermore, you need the following libraries:
ESPAsyncWebServer: https://github.com/me-no-dev/ESPAsyncWebServer.git
Adafruit_Sensor: https://github.com/adafruit/Adafruit_Sensor.git
DHT: https://github.com/adafruit/DHT-sensor-library
ErriezDS1302: https://github.com/Erriez/ErriezDS1302.git

The first library is an amazing server library for the esp32. Even if you only have experience with javascript/nodejs/express, the workings of this library should seem familiar to you.
The next two libraries are taking care of the temperature sensor.
The last takes care of the connected RTC-Module.

After you have installed all the dependencies, clone this repository onto your machine and upload the files to your esp32. You can do this with either VS Code with Arduino extension, with the esptool or with the Arduino IDE.
I used VS Code to upload the program and the Arduino IDE to upload the files in ./data to the SPIFFS file system. There is a handy tool to do this: https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/

After this, you should be able to run the application and measure temperatures and humidity anywhere and everywhere.

## Epilog

The application today (17.03.2021) is still kind of a mess, since I didn't think too much about structure and order while developing it, since I was myself still in the process of learning all these technologies consisting this project. If I have time in the future, I'd like to take care of the mess and make the program structure more clear and the installation process more simplistic. Until then, feel free to send me a pull request!