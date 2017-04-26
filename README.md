# led-matrix-clock
LED matrix clock with telegram bot

# Hardware

## Bill of materials

| Menge | Preis  | Beschreibung | Link |
| ----- | ------ | ------ | ----- |
| 1     | 0,22€  | 2.54mm THT Angled Pin Header| https://www.reichelt.de/index.html?ARTIKEL=SL%201X40W%202,54 |
| 4     | 10,66€ | 8x8 RGB LED Matrix with WS2812 LEDs | https://www.banggood.com/de/CJMCU-64-Bit-WS2812-5050-RGB-LED-Driver-Development-Board-p-981678.html |
| 1     | 0,75€  | Prototyping board |  https://www.reichelt.de/index.html?ARTIKEL=HPR%20100X100 |
| 3     | 0,15€  | Push button | https://www.reichelt.de/index.html?ARTIKEL=TASTER%203305 |
| 1     | 4,41€  | ESP8266 WLAN module | https://www.banggood.com/de/WeMos-D1-Mini-V2-NodeMcu-4M-Bytes-Lua-WIFI-Internet-Of-Things-Development-Board-Based-ESP8266-p-1115398.html |
| 1     | 1,15€  | LM75 I2C temperature sensor (I used a custom PCB from a former project) | http://www.ebay.de/itm/New-LM75A-Temperatur-Sensor-High-speed-I2C-Interface-Development-Board-Modul-/272434029114 |
| 1     |        | LDR (unknown type with approx. 5-10kR at 10Lux) | | 
| 1     | 0,10€  | Resistor 1kR for the voltage divider at the LDR | https://www.reichelt.de/index.html?ARTICLE=1315 |

## Schematics

### LDR
```
       +3.3V
      |
     .-.
 --> | |
 --> | | LDR
     '-'
      |
      +--- A0
      |
     .-.
     | |
     | | 1kR
     '-'
      |
       GND
```

### Push buttons
```
   T           T           T
  ---         ---         ---
+-o o-+     +-o o-+     +-o o-+
|     |     |     |     |     |
|G    |     |G    |     |G    |
|N    |D    |N    |D    |N    |D
|D    |7    |D    |6    |D    |3
```

### Connector for the LED matrix
```
DOUT  --  D5
+5V
GND
```

### Connector for the LM75
```
SCL  --  D1
SDA  --  D2
```

