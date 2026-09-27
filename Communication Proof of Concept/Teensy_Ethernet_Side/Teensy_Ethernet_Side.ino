// Teensy 4.1 low-latency TCP sensor server for avionics liquid test stand
// Requires the QNEthernet library (Library Manager -> search "QNEthernet")
//
// Protocol: PC sends any single byte as a "poll" trigger.
// Teensy replies immediately with a 20-byte packed binary struct:
//   uint32_t timestamp_ms   (4 bytes)
//   float    pressure_psi   (4 bytes)
//   float    temp_c         (4 bytes)
//   float    flow_lpm       (4 bytes)
//   float    valve_pos_pct  (4 bytes)
// Total: 20 bytes, matches Python struct format "<Iffff"
//
// IMPORTANT: __attribute__((packed)) is required so the compiler doesn't
// insert padding between fields -- without it, this struct could end up
// larger than 20 bytes and misaligned with what Python expects.

#include <QNEthernet.h>

using namespace qindesign::network;

IPAddress ip(192, 168, 1, 50);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 1, 1);

constexpr uint16_t PORT = 8888;

EthernetServer server(PORT);
EthernetClient client;

struct __attribute__((packed)) SensorPacket {
  uint32_t timestamp_ms;
  float pressure_psi;
  float temp_c;
  float flow_lpm;
  float valve_pos_pct;
};

static_assert(sizeof(SensorPacket) == 20, "SensorPacket must be exactly 20 bytes");

void setup() {
  Ethernet.begin(ip, subnet, gateway);
  server.begin();
}

// Replace this with your actual sensor reads (ADC, I2C, etc.)
SensorPacket readSensors() {
  SensorPacket pkt;
  pkt.timestamp_ms = millis();
  pkt.pressure_psi   = analogRead(A0) * (100.0f / 1023.0f);  // placeholder scaling
  pkt.temp_c         = analogRead(A1) * (0.1f);               // placeholder scaling
  pkt.flow_lpm       = analogRead(A2) * (0.05f);               // placeholder scaling
  pkt.valve_pos_pct  = analogRead(A3) * (100.0f / 1023.0f);   // placeholder scaling
  return pkt;
}

void loop() {
  if (!client || !client.connected()) {
    EthernetClient newClient = server.accept();
    if (newClient) {
      client = newClient;
      client.setNoDelay(true);  // disable Nagle's algorithm - critical for latency
    }
  }

  if (client && client.connected()) {
    int avail = client.available();
    if (avail > 0) {
      uint8_t trigger[64];
      client.read(trigger, avail > 64 ? 64 : avail);  // drain the poll byte(s)

      SensorPacket pkt = readSensors();
      client.write(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt));
      client.flush();
    }
  }
}
