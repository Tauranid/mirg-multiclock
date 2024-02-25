#!/usr/bin/env python3

import scrollphathd
from dataclasses import dataclass
import socket
import os
from time import sleep

row_clks = 0

@dataclass
class display_clock:
    bit_idx: int
    grid_x: int
    div: int

dc1 = display_clock(0, 8, 1)
dc2 = display_clock(1, 7, 2)
dc4 = display_clock(2, 6, 4)
dc8 = display_clock(3, 5, 8)
dc16 = display_clock(4, 4, 16)

dcs = [dc1, dc2, dc4, dc8, dc16]

socket_path = "/tmp/mirg_display_server.s"

try:
    os.unlink(socket_path)
except OSError:
    if os.path.exists(socket_path):
        raise

server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
server.bind(socket_path)

while True:
    server.listen(1)
    connection, client_address = server.accept()
    print("Got connection, entering display loop")
    #s.flush()
    while True:
        print("waiting to receive")
        data = connection.recv(1024)
        # slice last data byte (before 0) and convert to int
        print(data)
        data = data[-2:-1]
        data = int.from_bytes(data, "little")
        print("received data, displaying now")
        print(data)
        
        scrollphathd.clear()
        for dc in dcs:
            if data & (1 << dc.bit_idx):
                scrollphathd.set_pixel(dc.grid_x, row_clks, 0.5)
        scrollphathd.show()
        sleep(0.01)
        scrollphathd.clear()
        scrollphathd.show()


