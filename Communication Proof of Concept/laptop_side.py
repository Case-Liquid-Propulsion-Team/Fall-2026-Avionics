import socket
import struct
import time
import numpy as np

TEENSY_IP = "192.168.1.50"
PORT = 8888
N_POLLS = 5000

# Must match the Teensy's SensorPacket struct exactly:
# < = little-endian, I = uint32, f = float (4 bytes each)
PACKET_FORMAT = "<Iffff"
PACKET_SIZE = struct.calcsize(PACKET_FORMAT)  # 20 bytes


def recv_exact(sock, n):
    """recv() can return fewer bytes than requested, especially for
    multi-byte binary payloads. This loops until exactly n bytes are
    read, or raises if the connection drops."""
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("Socket closed before full packet received")
        buf.extend(chunk)
    return bytes(buf)


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    try:
        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        s.connect((TEENSY_IP, PORT))
        print(f"Connected to Teensy at {TEENSY_IP}:{PORT}")

        times = []

        for i in range(N_POLLS):
            prev = time.perf_counter()

            s.sendall(b"\x11")  # poll trigger byte

            data = recv_exact(s, PACKET_SIZE)
            now = time.perf_counter()

            timestamp_ms, pressure_psi, temp_c, flow_lpm, valve_pos_pct = \
                struct.unpack(PACKET_FORMAT, data)

            dt = now - prev
            times.append(dt)

            print(f"t={timestamp_ms:>10} ms  "
                  f"P={pressure_psi:6.2f} psi  "
                  f"T={temp_c:6.2f} C  "
                  f"Flow={flow_lpm:6.2f} lpm  "
                  f"Valve={valve_pos_pct:6.2f} %  "
                  f"RTT={dt * 1000:.3f} ms")

        times = np.array(times) * 1000
        print("\nBytes per communication: 20")
        print(f"Number of trials: {N_POLLS}")
        print(f"\nMean RTT: {times.mean():.3f} ms")
        print(f"Min RTT:  {times.min():.3f} ms")
        print(f"Max RTT:  {times.max():.3f} ms")
        print(f"Std dev:  {times.std():.3f} ms\n\n")

    except Exception as e:
        print(f"Connection failed: {e}")