import socket

TEENSY_IP = "192.168.50.5"
BASE_STATION_IP = "192.168.50.1"
PORT = 8888
N_POLLS = 5000

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Use port 8888 for communication with Teensy
sock.bind((BASE_STATION_IP, PORT))

while True:
    data, addr = sock.recvfrom(2048)
    data = data.decode('utf-8')
    print(f"Received message from {addr}: {data}")