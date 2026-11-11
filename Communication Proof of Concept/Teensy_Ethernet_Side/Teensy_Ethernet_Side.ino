#include <QNEthernet.h>

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

uint32_t lastSent = 0;

uint8_t packet[74] = {0};

void setup() {

  Serial.begin(6000000); //baud rate of 6 MHz
  while(!Serial && millis() < 4000); // wait for serial monitor to begin
  Ethernet.begin(TEENSY_IP, subnet, BASE_STATION_IP); // set up ethernet

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
  
}