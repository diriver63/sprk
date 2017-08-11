#include <Time.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// WI-FI Settings
const char ssid[] = "ATTEGJxhGs";  //  your network SSID (name)
const char pass[] = "pvrm%r=8h#?g";       // your network password

// NTP Servers:
IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov

// Time Zone offset
const int timeZone = -5;  // Central Standard Time (USA)

// UDP Settings to talk to ntp server
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// MySprinkler Settings
#define VALVES 3
int valve_time[3] = {7,5,10};
int valvepin[3] = {0,4,5}; // Pin output for Zones 0,1,and 2
unsigned long start_time = 0;
int valve_id;

enum DayOfWeek {SUNDAY=1, MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY};
typedef enum {
  STAND_BY_ALL_OFF, RUN_SINGLE_ZONE, RUN_ALL_ZONES, CYCLE_COMPLETE, ZONE_SELECT_MENU
}
SprinklerStates;

SprinklerStates state = STAND_BY_ALL_OFF;
SprinklerStates lastState;

#define ACTIVE_DAY1 MONDAY
#define ACTIVE_DAY2 WEDNESDAY
#define ACTIVE_DAY3 FRIDAY

#define ACTIVE_HOUR_AM 7
#define ACTIVE_MIN_AM 00
#define ACTIVE_HOUR_PM 20
#define ACTIVE_MIN_PM 00
 

boolean SystemStatus = true; // System On = True; System Off = False

void setup() {
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  pinMode(2, OUTPUT); // ON/OFF Indicator
  pinMode(0, OUTPUT); // Output for Zone 1
  pinMode(4, OUTPUT); // Output for Zone 2 
  pinMode(5, OUTPUT); // Output for Zone 3
  StopIrrigation();
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
}

void loop() {
  if (SystemStatus){
    digitalWrite(2, LOW);
    if (state == STAND_BY_ALL_OFF){
      valve_id = 0;
      if (isAM(getNtpTime())){
         if ((hour()== ACTIVE_HOUR_AM && minute()== ACTIVE_MIN_AM) && (weekday() == ACTIVE_DAY1 or weekday() == ACTIVE_DAY2 or weekday() == ACTIVE_DAY3)){
           state = RUN_ALL_ZONES;
           start_time = millis();
           StartIrrigation(valve_id);
         }
      }
      else if (isPM(getNtpTime())){
        if ((hour()== ACTIVE_HOUR_PM && minute() == ACTIVE_MIN_PM) && (weekday() == ACTIVE_DAY1 or weekday() == ACTIVE_DAY2 or weekday() == ACTIVE_DAY3)) {
          state = RUN_ALL_ZONES;
          start_time = millis();
          StartIrrigation(valve_id);
        }
      }
      lastState = state;
    }

    if (state == RUN_ALL_ZONES){
      Serial.print("Run all zones...");
      Serial.println(start_time);
      Serial.println(valve_id);
      while(millis() - start_time < valve_time[valve_id]*60000){
//        Serial.println(millis()-start_time);
        delay(500);
        lastState = state;  
      }
      StopIrrigation();
      valve_id++;
      if (valve_id >= VALVES){
        state = STAND_BY_ALL_OFF;  
      }
      else{
        delay(5000);
        start_time = millis();
        StartIrrigation(valve_id);  
      }
    }
  }
  else{
    valve_id = 0;
    lastState = state;
    state == STAND_BY_ALL_OFF;
  }
}

///////////////////////////////////////////////////////////////////////////////////////
time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
//  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
//      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
//  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}
///////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////
void StartIrrigation(int valve){
  int i;
  for (i = 0; i < VALVES; i++){
    if (i==valve){
      digitalWrite(valvepin[i], LOW);
      Serial.print("Start Irrigation - Zone ");
      Serial.print(i);
      Serial.println(" active");
    }
    else {
      digitalWrite(valvepin[i], HIGH);
      Serial.print("Start Irrigation - Zone ");
      Serial.print(i);
      Serial.println(" off");      
    }
  }
}
///////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////
void StopIrrigation(){
  int i;
  for (i = 0; i < VALVES; i++){
    digitalWrite(valvepin[i], HIGH);
    Serial.print("Stop Irrigation - Zone ");
    Serial.print(i);
    Serial.println(" off");     
  }
}
///////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////
// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
///////////////////////////////////////////////////////////////////////////////////////


