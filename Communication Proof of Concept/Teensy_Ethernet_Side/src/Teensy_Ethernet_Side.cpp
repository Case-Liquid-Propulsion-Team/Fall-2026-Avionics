#include <QNEthernet.h>
#include <Arduino.h>

using namespace qindesign::network;
// IP of TEENSY_IP
IPAddress TEENSY_IP(192, 168, 50, 5);
IPAddress subnet(255, 255, 255, 0);

// IP of Base Station
IPAddress BASE_STATION_IP(192, 168, 50, 1);
// use port 8888 for communications
constexpr uint16_t port = 8888;

// create the UDP communication object
EthernetUDP udp;

//predefined constants
const uint8_t numPTs = 7;
const uint8_t PTpins[numPTs] = {A0, A1, A2, A3, A4, A5, A6};

// variables
uint32_t timestamp = 0;
uint32_t packetStartTime = 0;
uint32_t packetNumber = 0; // only 24 bits in the packet 

uint8_t packet[74] = {0};

uint32_t PTsampleCount = 0;
uint32_t PTsums[numPTs] = {0};
uint16_t PTpeaks[numPTs] = {0};

void updatePressure(const uint8_t PTnum){
  uint16_t pressure = analogRead(PTpins[PTnum]);
  if(pressure > PTpeaks[PTnum]) PTpeaks[PTnum] = pressure;
  PTsums[PTnum] += pressure;
}

uint16_t calcPressureAvg(const uint8_t PTnum){
  return (float) PTsums[PTnum] / PTsampleCount;
}

void setup() {

  Serial.begin(6000000); //baud rate of 6 MHz
  while(!Serial && millis() < 4000); // wait for serial monitor to begin
  Ethernet.begin(TEENSY_IP, subnet, BASE_STATION_IP); // set up ethernet

  //Ethernet checking
  if(!Ethernet.waitForLink(4000)) {
    Serial.println("Ethernet connection failed");
    return;
  }
  else{
    Serial.println("Link up");
    Serial.println(Ethernet.localIP());
  }
}

void loop(){
  timestamp = micros();
  PTsampleCount++;
  for(uint8_t i = 0; i < numPTs; i++){
    updatePressure(i);
  }
  if(micros() - timestamp > 500){
    // packet number
    packet[0] = (packetNumber >> 16) & 0xFF; //get bits 16-23
    packet[1] = (packetNumber >> 8) & 0xFF; //get bits 8-15
    packet[2] = packetNumber & 0xFF; //get bits 0-7

    // timestamp
    packet[3] = (timestamp >> 24) & 0xFF; 
    packet[4] = (timestamp >> 16) & 0xFF;
    packet[5] = (timestamp >> 8) & 0xFF;
    packet[6] = timestamp & 0xFF;

    for(uint8_t i = 0; i < numPTs; i++){
      //peak pressure
      packet[7 + 6 * i] = PTpeaks[i] >> 8;
      packet[8 + 6 * i] = PTpeaks[i] & 0xFF;

      //time when peak pressures happened from the start of the packet measurement


      //average pressures
      uint16_t avgPressure = calcPressureAvg(i);
      packet[12 + 6 * i] = avgPressure >> 8; //get high byte of average pressure
      packet[13 + 6 * i] = avgPressure & 0xFF; // get low byte of average pressure
    }


    udp.send(BASE_STATION_IP, port, packet, 74);
    
    packetNumber++; 
  }
}