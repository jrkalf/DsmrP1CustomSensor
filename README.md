# DEPRECATED -- REPLACED, SEE BELOW!

## DsmrP1CustomSensor

I copied the code from NLdroid from Github. 
This is his first custom component for EspHome. Which I copied and adjusted to my specific needs.
It can be used to read DSMR data from the P1 port of dutch smart meters. My specific needs are DSMR 2.2 as I have a very old (2013) Landis + Gyr E350 ZCF1120 meter.

The work is based on this project: https://github.com/nldroid/DsmrP1CustomSensor which again is based on https://github.com/fliphess/esp8266_p1meter

You can also find there how to create a cable
Just add the .h file in your config folder and see the .yaml file for usage

# ESPhome DSMR support

Nowadays DSMR support is built in to ESPhome by default.

My configuration:

Keep in mind the Landis + Gyr 350 ZCF120 has an inverted pin on the uart.
Uart is 9600 baud 7E1.

Increase the max_telegram_length or you'll get a timeout message on receiving the message.
Use request interval to moderate the number of requests. There's no need to poll every 10 seconds. Every minute is acurate enough.

```
# Custom uart settings for DSMR v2.2
uart:
  debug:
  baud_rate: 9600
  data_bits: 7
  parity: EVEN
  stop_bits: 1
  rx_pin:
    number: D7
    inverted: true
  #rx_buffer_size: 3000

dsmr:
  crc_check: false
  max_telegram_length: 5000
  request_pin: D0
  request_interval: 60s

sensor:
  - platform: dsmr
    energy_delivered_tariff1:
      name: dsmr_energy_delivered_tariff1
    energy_delivered_tariff2:
      name: dsmr_energy_delivered_tariff2
    energy_returned_tariff1:
      name: dsmr_energy_returned_tariff1
    energy_returned_tariff2:
      name: dsmr_energy_returned_tariff2
      

text_sensor:
  - platform: dsmr
    identification:
      name: "dsmr_identification"
    p1_version:
      name: "dsmr_p1_version"
```
