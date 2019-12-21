#include "esphome.h"
#include <SoftwareSerial.h>

// * Baud rate for both hardware and software serial
#define BAUD_RATE 115200
#define SWSERIAL_BAUD_RATE 9600

// * Max telegram length
#define P1_MAXLINELENGTH 75

// * P1 Meter RX pin
#define P1_SERIAL_RX D2

// * Initiate Software Serial
SoftwareSerial p1_serial(P1_SERIAL_RX, SW_SERIAL_UNUSED_PIN, true, P1_MAXLINELENGTH); // (RX, TX, inverted, buffer)

class DsmrP1CustomSensor : public Component {
 public:
   // * Set to store received telegram
  char telegram[P1_MAXLINELENGTH];

  // * Set to store the data values read
  long CONSUMPTION_LOW_TARIF;
  long CONSUMPTION_HIGH_TARIF;
  long RETURN_LOW_TARIF;
  long RETURN_HIGH_TARIF;
  long ACTUAL_CONSUMPTION;
  long ACTUAL_RETURN;
  long INSTANT_POWER_CURRENT;
  long INSTANT_POWER_USAGE;
  long GAS_METER_M3;

  // Set to store data counters read
  long ACTUAL_TARIF;
  long SHORT_POWER_OUTAGES;
  long LONG_POWER_OUTAGES;
  long SHORT_POWER_DROPS;
  long SHORT_POWER_PEAKS;
 
  Sensor *consumption_low_tarif_sensor = new Sensor();
  Sensor *consumption_high_tarif_sensor = new Sensor();
  Sensor *return_low_tarif_sensor = new Sensor();
  Sensor *return_high_tarif_sensor = new Sensor();
  Sensor *actual_consumption_sensor = new Sensor();
  Sensor *actual_return_sensor = new Sensor();
  Sensor *instant_power_current_sensor = new Sensor();
  Sensor *instant_power_usage_sensor = new Sensor();
  Sensor *gas_meter_m3_sensor = new Sensor();
  Sensor *actual_tarif_sensor = new Sensor();
  Sensor *short_power_outages_sensor = new Sensor();
  Sensor *long_power_outages_sensor = new Sensor();
  Sensor *short_power_drops_sensor = new Sensor();
  Sensor *short_power_peaks_sensor = new Sensor();

  // DsmrP1CustomSensor() : PollingComponent(1000) { }
  DsmrP1CustomSensor() {}

  void setup() override {
	  Serial.begin(BAUD_RATE);
	
    // * Start software serial for p1 meter
    p1_serial.begin(SWSERIAL_BAUD_RATE);
  }

  void loop() override {
		
    if (p1_serial.available())
    {
      memset(telegram, 0, sizeof(telegram));

      while (p1_serial.available())
      {
        ESP.wdtDisable();

        int len = p1_serial.readBytesUntil('\n', telegram, P1_MAXLINELENGTH);
        
        // DSMR 2.2 is SERIAL 7E1 not 8N1, so a bitshift needs to take place in order to properly read data.
        for (int cnt = 0; cnt < len; cnt++)
          telegram[cnt] &= ~(1 << 7);	
        
        ESP.wdtEnable(1);

        telegram[len] = '\n';
        telegram[len + 1] = 0;
        decode_telegram(len +1);
      }
	  }  
  }
  
 private:
  bool isNumber(char *res, int len)
  {
    for (int i = 0; i < len; i++)
    {
      if (((res[i] < '0') || (res[i] > '9')) && (res[i] != '.' && res[i] != 0))
        return false;
    }
    return true;
  }
  
  int FindCharInArrayRev(char array[], char c, int len)
  {
    for (int i = len - 1; i >= 0; i--)
    {
      if (array[i] == c)
        return i;
    }
    return -1;
  }
  
  long getValue(char *buffer, int maxlen, char startchar, char endchar)
  {
    int s = FindCharInArrayRev(buffer, startchar, maxlen - 2);
    int l = FindCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;
  
    char res[16];
    memset(res, 0, sizeof(res));
  
    if (strncpy(res, buffer + s + 1, l))
    {
      if (endchar == '*')
      {
        if (isNumber(res, l))
          // * Lazy convert float to long
          return (1000 * atof(res));
      }
      else if (endchar == ')')
      {
        if (isNumber(res, l))
          return atof(res);
      }
    }
    return 0;
  }
  
  void decode_telegram(int len)
  {

/*
  Sample DSMR2.2 message
  /XMX5<secret>  <-- Landis + Gyr E350 ZCF120

  0-0:96.1.1(123454323454323456543)
  1-0:1.8.1(12451.666*kWh)
  1-0:1.8.2(09539.696*kWh)
  1-0:2.8.1(03008.444*kWh)
  1-0:2.8.2(07401.746*kWh)
  0-0:96.14.0(0002)
  1-0:1.7.0(0000.61*kW)
  1-0:2.7.0(0000.00*kW)
  0-0:96.13.1()
  0-0:96.13.0()
  !
*/
    // 1-0:1.8.1(000992.992*kWh)
    // 1-0:1.8.1 = Elektra verbruik laag tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.8.1", strlen("1-0:1.8.1")) == 0)
    {
      CONSUMPTION_LOW_TARIF = getValue(telegram, len, '(', '*');
      consumption_low_tarif_sensor->publish_state(CONSUMPTION_LOW_TARIF);
    }
  
    // 1-0:1.8.2(000560.157*kWh)
    // 1-0:1.8.2 = Elektra verbruik hoog tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.8.2", strlen("1-0:1.8.2")) == 0)
    {
      CONSUMPTION_HIGH_TARIF = getValue(telegram, len, '(', '*');
      consumption_high_tarif_sensor->publish_state(CONSUMPTION_HIGH_TARIF);
    }

    // 1-0:2.8.1(000992.992*kWh)
    // 1-0:2.8.1 = Elektra teruglevering laag tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:2.8.1", strlen("1-0:2.8.1")) == 0)
    {
      RETURN_LOW_TARIF = getValue(telegram, len, '(', '*');
      return_low_tarif_sensor->publish_state(RETURN_LOW_TARIF);
    }
  
    // 1-0:1.8.2(000560.157*kWh)
    // 1-0:1.8.2 = Elektra teruglevering hoog tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:2.8.2", strlen("1-0:2.8.2")) == 0)
    {
      RETURN_HIGH_TARIF = getValue(telegram, len, '(', '*');
      return_high_tarif_sensor->publish_state(RETURN_HIGH_TARIF);
    }

    // 1-0:1.7.0(00.424*kW) Actueel verbruik
    // 1-0:1.7.x = Electricity consumption actual usage (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.7.0", strlen("1-0:1.7.0")) == 0)
    {
      ACTUAL_CONSUMPTION = getValue(telegram, len, '(', '*');
      actual_consumption_sensor->publish_state(ACTUAL_CONSUMPTION);
    }

    // 1-0:2.7.0(0000.00*kW)
    // 1-0:2.7.0(0000.00*kW) Actuele teruglevering
    if (strncmp(telegram, "1-0:2.7.0", strlen("1-0:2.7.0")) == 0)
    {
      ACTUAL_RETURN = getValue(telegram, len, '(', '*');
      actual_return_sensor->publish_state(ACTUAL_RETURN);
    }
  
    // 1-0:21.7.0(00.378*kW)
    // 1-0:21.7.0 = Instantaan vermogen Elektriciteit levering
    if (strncmp(telegram, "1-0:21.7.0", strlen("1-0:21.7.0")) == 0)
    {
      INSTANT_POWER_USAGE = getValue(telegram, len, '(', '*');
      instant_power_current_sensor->publish_state(INSTANT_POWER_CURRENT);
    }
  
    // 1-0:31.7.0(002*A)
    // 1-0:31.7.0 = Instantane stroom Elektriciteit
    if (strncmp(telegram, "1-0:31.7.0", strlen("1-0:31.7.0")) == 0)
    {
      INSTANT_POWER_CURRENT = getValue(telegram, len, '(', '*');
      instant_power_usage_sensor->publish_state(INSTANT_POWER_USAGE);
    }
  
    // 0-1:24.2.1(150531200000S)(00811.923*m3)
    // 0-1:24.2.1 = Gas (DSMR v4.0) on Kaifa MA105 meter
    if (strncmp(telegram, "0-1:24.2.1", strlen("0-1:24.2.1")) == 0)
    {
      GAS_METER_M3 = getValue(telegram, len, '(', '*');
      gas_meter_m3_sensor->publish_state(GAS_METER_M3);
    }
  
    // 0-0:96.14.0(0001)
    // 0-0:96.14.0 = Actual Tarif
    if (strncmp(telegram, "0-0:96.14.0", strlen("0-0:96.14.0")) == 0)
    {
      ACTUAL_TARIF = getValue(telegram, len, '(', ')');
      actual_tarif_sensor->publish_state(ACTUAL_TARIF);
    }
  
    // 0-0:96.7.21(00003)
    // 0-0:96.7.21 = Aantal onderbrekingen Elektriciteit
    if (strncmp(telegram, "0-0:96.7.21", strlen("0-0:96.7.21")) == 0)
    {
      SHORT_POWER_OUTAGES = getValue(telegram, len, '(', ')');
    }
  
    // 0-0:96.7.9(00001)
    // 0-0:96.7.9 = Aantal lange onderbrekingen Elektriciteit
    if (strncmp(telegram, "0-0:96.7.9", strlen("0-0:96.7.9")) == 0)
    {
      LONG_POWER_OUTAGES = getValue(telegram, len, '(', ')');
    }
  
    // 1-0:32.32.0(00000)
    // 1-0:32.32.0 = Aantal korte spanningsdalingen Elektriciteit in fase 1
    if (strncmp(telegram, "1-0:32.32.0", strlen("1-0:32.32.0")) == 0)
    {
      SHORT_POWER_DROPS = getValue(telegram, len, '(', ')');
    }
  
    // 1-0:32.36.0(00000)
    // 1-0:32.36.0 = Aantal korte spanningsstijgingen Elektriciteit in fase 1
    if (strncmp(telegram, "1-0:32.36.0", strlen("1-0:32.36.0")) == 0)
    {
      SHORT_POWER_PEAKS = getValue(telegram, len, '(', ')');
    }
  } 
  
};
