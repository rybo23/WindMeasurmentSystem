# WindMeasurmentSystem
Wind Measurement System using a custom PCB, anemometer, and data visualization uisng an OLED display to show data and a dynamic LED matrix that respnds to wind speeds.


#Features
- Real time wind measurments in mph and m/s
- Maximum gust detection
- OLED display to show instantaneous wind speed, anemometer voltage, and maximum gust
- CSV and Readable serial monitor output modes printing the average wing speed (mph and m/s), anemometer voltage, and max gust over a user defined time window
- Easy to adjust parameters for system calibration and flag animation preferences 

#Hardware
- Custom PCB based on an IEEE reference design (modified, assembled, and integrated into the system)
- On PCB buttons and LED Matrix
- JL-FS2 Anemometer (analog output)
- SPI OLED display (SH1106)
- RC lowpass filter (for anemometer signal noise reduction)
- 12V, 2A Power Adapter
- DC Barrel Jack

#Controls
- SW1
  - Short press: Toggle flag (USA / Guatemala)
  - Long press: Max gust detection reset
- SW2
  - Short press: Increase animation speed (w/ wrap around)
- SW3
  - Short press: Increase brightness (w/ wrap around)
  - Long press: Toggle serial output (CSV / Readable)
 
#System Overview 
The system reads the analog voltage of the anemmeter and converts it to a wind speed using a calibrated voltage range and corresponding maximum wind speed.

An external hardware RC lowpass filter and a digital exponential moving average (EMA) filter are used to reduce noice and system jitter. This processed signal is used to:
- Display instantaneous wind data on the OLED screen
- Dynamically control the speed of the LED flag animation
- Calculate and log the average wind speed, voltage, and maximum gust over a time window to the serial monitor


#Future improvements
- Weatherproofing and display
- SD card data logging
- Expanded envormental sensing



  





