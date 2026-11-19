#include <QNEthernet.h>
#include <Arduino.h>

using namespace qindesign::network;
// IP of TEENSY_IP
IPAddress TEENSY_IP(192, 168, 50, 5);
IPAddress SUBNET(255, 255, 255, 0);

// IP of Base Station
IPAddress BASE_STATION_IP(192, 168, 50, 1);

// use PORT 8888 for communications
constexpr uint16_t PORT = 8888;
constexpr uint8_t PACKET_SIZE = 77;
constexpr uint16_t PACKET_INTERVAL_US = 500;

// create the UDP communication object
EthernetUDP udp;

//predefined constants
constexpr uint8_t NUM_PTS = 7;
constexpr uint8_t PT_PINS[NUM_PTS] = {A0, A1, A2, A3, A4, A5, A6};

constexpr uint8_t NUM_TEMPS = 5;
constexpr uint8_t TEMP_PINS[NUM_TEMPS] = {-1, -1, -1, -1, -1};

constexpr uint8_t NUM_LOAD_CELLS = 3;
constexpr uint8_t LOAD_CELL_PINS[NUM_LOAD_CELLS] = {-1, -1, -1};

// variables
uint8_t packet[PACKET_SIZE] = {0};

uint32_t timestamp = 0;
uint32_t packetStartTime = 0;

uint32_t packetNumber = 0; // only 24 bits in the packet 
uint16_t loadCellPacketNumber = 0;
uint16_t temperaturePacketNumber = 0;

uint32_t ptSampleCount = 0;
uint32_t ptSums[NUM_PTS] = {0};
uint16_t ptPeaks[NUM_PTS] = {0};
uint16_t ptPeakTimes[NUM_PTS] = {0};


// write data to packet methods
void writeUint8(uint8_t &index, uint8_t value){
  packet[index++] = static_cast<uint8_t>(value);
}

void writeUint12(uint8_t &index, uint16_t value){
  value &= 0x0FFF;

  // Upper four bits of the first byte remain zero.
  packet[index++] = static_cast<uint8_t>(value >> 8);
  packet[index++] = static_cast<uint8_t>(value);
}

void writeUint16(uint8_t &index, uint16_t value){
  packet[index++] = static_cast<uint8_t>(value >> 8);
  packet[index++] = static_cast<uint8_t>(value);
}

void writeUint24(uint8_t &index, uint32_t value){
  packet[index++] = static_cast<uint8_t>(value >> 16);
  packet[index++] = static_cast<uint8_t>(value >> 8);
  packet[index++] = static_cast<uint8_t>(value);
}

void writeUint32(uint16_t &index, uint32_t value) {
  packet[index++] = static_cast<uint8_t>(value >> 24);
  packet[index++] = static_cast<uint8_t>(value >> 16);
  packet[index++] = static_cast<uint8_t>(value >> 8);
  packet[index++] = static_cast<uint8_t>(value);
}

uint16_t readPressure(const uint8_t ptNum) {
  const int pin = PT_PINS[ptNum];

  if (pin < 0) return 0;

  return static_cast<uint16_t>(analogRead(pin)) & 0x0FFF;
}

uint32_t readLoadCell(const uint8_t pin) {
  if (pin < 0) return 0;

  // Placeholder until the load-cell ADC interface is selected.
  return static_cast<uint32_t>(analogRead(pin)) & 0x00FFFFFF;
}

int16_t readTemperature(const uint8_t pin) {
  if (pin < 0) return 0;

  // Placeholder until temperature encoding and conversion are finalized.
  return 0;
}

void updatePressure(const uint8_t ptNumber, const uint32_t elapsedUs) {
  const uint16_t pressure = readPressure(ptNumber);

  ptSums[ptNumber] += pressure;

  if (pressure > ptPeaks[ptNumber]) {
    ptPeaks[ptNumber] = pressure;
    ptPeakTimes[ptNumber] = static_cast<uint16_t>(
      elapsedUs > UINT16_MAX ? UINT16_MAX : elapsedUs
    ); // error checking in case our sampling time increases dramatically
  }
}

uint16_t calculatePressureAverage(const uint8_t ptNumber) {
  if (ptSampleCount == 0) return 0;

  return static_cast<uint16_t>(ptSums[ptNumber] / ptSampleCount);
}

void writeTemperaturePlaceholder(uint8_t &index, const int32_t temperature) {
  (void)temperature;

  // Temperature encoding is not finalized yet.
  packet[index++] = 0;
  packet[index++] = 0;
  packet[index++] = 0;
}


void buildPacket(const uint32_t packetTime) {
  uint8_t index = 0;

  // Bytes 1-3: packet number
  writeUint24(index, packetNumber++);

  // Bytes 4-7: packet time
  writeUint32(index, packetTime);

  // Bytes 8-49: pressure-transducer data
  for (uint8_t i = 0; i < NUM_PTS; ++i) {
    writeUint12(index, ptPeaks[i]);
    writeUint16(index, ptPeakTimes[i]);
    writeUint12(index, calculatePressureAverage(i));
  }

  // Bytes 50-51: load-cell packet number
  writeUint16(index, loadCellPacketNumber++);

  // Bytes 52-54: thrust load cell
  writeUint24(index, readLoadCell(THRUST_LOAD_CELL_PIN));

  // Bytes 55-57: oxidizer load cell
  writeUint24(index, readLoadCell(OXIDIZER_LOAD_CELL_PIN));

  // Bytes 58-59: temperature packet number
  writeUint16(index, temperaturePacketNumber++);

  // Bytes 60-74: five temperature values
  for (uint8_t i = 0; i < NUM_TEMP; ++i) {
    writeTemperaturePlaceholder(index, readTemperature(TEMP_PINS[i]));
  }
}

void setup() {

  Serial.begin(6000000); //baud rate of 6 MHz
  while(!Serial && millis() < 4000); // wait for serial monitor to begin
  Ethernet.begin(TEENSY_IP, SUBNET, BASE_STATION_IP); // set up ethernet

  //Ethernet checking
  if(!Ethernet.waitForLink(4000)) {
    Serial.println("Ethernet connection failed");
    return;
  }
  else{
    Serial.println("Link up");
    Serial.println(Ethernet.localIP());
  }

  //SPI setup
  SPI.begin();

  // pin setup
  for(int i = 0; i < NUM_PTS; i++){
    pinMode(PT_PINS[i], INPUT_DISABLE);
  }
  
  // SPI setup for load cells and temps
  for(int i = 0; i < NUM_TEMPS; i++){
    pinMode(TEMP_PINS, OUTPUT);
  }

  for(int i = 0; i < NUM_LOAD_CELLS; i++){
    pinMode(TEMP_PINS, OUTPUT);
  }
}

void loop(){
  timestamp = micros();
  ptSampleCount++;
  for(uint8_t i = 0; i < NUM_PTS; i++){
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

    for(uint8_t i = 0; i < NUM_PTS; i++){
      //peak pressure
      packet[7 + 6 * i] = ptPeaks[i] >> 8;
      packet[8 + 6 * i] = ptPeaks[i] & 0xFF;

      //time when peak pressures happened from the start of the packet measurement


      //average pressures
      uint16_t avgPressure = calcPressureAvg(i);
      packet[12 + 6 * i] = avgPressure >> 8; //get high byte of average pressure
      packet[13 + 6 * i] = avgPressure & 0xFF; // get low byte of average pressure
    }


    udp.send(BASE_STATION_IP, PORT, packet, 74);
    
    packetNumber++; 
  }
}