#WindMeasurementSystem

Wind Measurement System using a custom PCB, anemometer, and data visualization uisng an OLED display to show data and a dynamic LED matrix that responds to wind speeds.

#Features
- Real time wind measurements in mph and m/s
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
The system reads the analog voltage of the anemometer and converts it to a wind speed using a calibrated voltage range and corresponding maximum wind speed.

An external hardware RC lowpass filter and a digital exponential moving average (EMA) filter are used to reduce noise and system jitter. This processed signal is used to:
- Display instantaneous wind data on the OLED screen
- Dynamically control the speed of the LED flag animation
- Calculate and log the average wind speed, voltage, and maximum gust over a time window to the serial monitor

#Future improvements
- Weatherproofing and display
- SD card data logging
- Expanded environmental sensing

#Video Demos

System demo and overview: https://youtu.be/OmSgg90Bh14?si=VTESv_KzRwL9rZkg

Anemometer Testing: https://youtube.com/shorts/ssbGXXnbcn4?si=7LpYPOe13QlDxpDv

#Project Report Available on my LinkedIn

https://www.linkedin.com/in/ryan-boylewpiece/

#Example CSV Output

<img width="407" height="135" alt="CSVOutputEx" src="https://github.com/user-attachments/assets/c928723d-5370-4d6b-99dd-b97a8033d1d7" />

#Example 'Readable' Output

<img width="920" height="177" alt="READABLEOutput" src="https://github.com/user-attachments/assets/dc1ce662-7db9-4af4-9f2e-3516f4b3da47" />

#Wiring Diagram

<img width="887" height="547" alt="SystemWiring" src="https://github.com/user-attachments/assets/c8cf652f-42b0-4441-825d-f7827ab45630" />

#System Images

<img width="832" height="808" alt="FinalSystemImage" src="https://github.com/user-attachments/assets/609656ed-a529-4341-a1c3-c29f273642a1" />

<img width="552" height="591" alt="SystemUpClose" src="https://github.com/user-attachments/assets/e1776af0-3d89-4645-ba4d-3122b4edfa20" />

<img width="747" height="672" alt="OLEDdisplay" src="https://github.com/user-attachments/assets/306450c0-4b0f-4e03-aae8-db8510e7b68e" />



