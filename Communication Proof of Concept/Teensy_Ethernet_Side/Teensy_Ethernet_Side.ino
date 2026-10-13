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

const char* msg = "Hello";

void setup() {

  Serial.begin(6000000);
  while(!Serial && millis() < 4000); // wait for serial monitor to begin
  Ethernet.begin(TEENSY_IP, subnet, BASE_STATION_IP);

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
  if(millis() - lastSent >= 1000){
    Serial.println("send");
    if(!udp.send(BASE_STATION_IP, port, msg, strlen(msg))){
      Serial.println("Message failed");
    }
    lastSent += 1000;
  }
}