# Cocito-Weather-Station
IOT Weather Station with Air Quality monitoring

Our school is involved in water analysis in the local area, so we thought to build a weather station with a particulate sensor, to monitor air quality in Alba. It has to be modular and upgradable in the future, so new students will be able to work on it and keep it working if something fails. They will also learn electronics by doing that. 
It will be solar powered and connected to the school website using an ESP8266 and an IOT platform.

For the microcontroller we chose an ATmega328P-PU, rather than using a standard Arduino Uno, for less power consumption. We hope it will not run out of memory :).

The station will upload data to ThingSpeak™ using an ESP8266, mainly chosen for its low cost ad popularity, and we will public the data collected from the sensors on our school website, showing in real time weather and air quality conditions. We will probably develop an app for android devices for faster readings.

In the following years these could be implemented by using a GSM module instead of the ESP8266, so more stations could be added to the “network” and placed in fields; we could also use a Raspberry Pi as a server to store data and take track of the environmental conditions all over the years.

For the sensor, we have chosen:

Sparkfun weather meters (wind wane, anemometer and rain gauge) (https://www.sparkfun.com/products/8942)
Adafruit AM2315 (temperature and humidity sensor) (https://www.adafruit.com/product/1293)
Adafruit BMP280 (pressure) (https://www.adafruit.com/products/2651)
DFROBOT PM 2.5 laser dust sensor (air quality) (http://www.dfrobot.com/index.php?route=product/product&product_id=1272)
The station will be powered by a solar setup made by a 20W solar panel, a 12V 12Ah lead acid battery and a solar charge controller. We decided to build a custom version of “Arduino solar charge controller version 2.0” made by Debasish Dutta (http://www.instructables.com/id/ARDUINO-SOLAR-CHARGE-CONTROLLER-Version-20/) because we like DIY stuff, but if you want to replicate our project, our suggestion is to buy one of the cheap PWM ones.

The temperature and pressure sensors will be protected by a solar shield, that we decided to 3D print to reduce the cost (buying one is about 90€ in Italy). The one designed by micromet (http://www.thingiverse.com/thing:1067700) seems very good, but we will have to make some mods to fit both sensors. Once done we will upload them on Thingiverse.

The entire cost will be around 300€, an affordable price that allows us to create an enough good product to collect precise data.
