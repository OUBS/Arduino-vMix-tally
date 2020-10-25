"""Implement simple server for vMix protocol.

The protocol is described here: https://www.vmix.com/help23/index.htm?TCPAPI.html
"""

import socket
import threading
import random

PORT = 8099
INPUTS = 3


def handle(conn):
    """Send fake vMix message to given socket"""
    conn.send('VERSION OK 23.0.0.35\r\n'.encode('utf-8'))
    is_subscribed = False
    conn.settimeout(5.0)
    buf = b''
    while True:
        try:
            buf += conn.recv(1024)
        except socket.timeout:
            pass
        # Handle incoming messages
        while b'\r\n' in buf:
            (line, buf) = buf.split(b'\r\n', 1)
            print('< ' + line.decode('utf-8'))
            if line.decode('utf-8') == 'SUBSCRIBE TALLY':
                is_subscribed = True
                print('> SUBSCRIBE OK TALLY')
                conn.send('SUBSCRIBE OK TALLY\r\n'.encode('utf-8'))
            else:
                print('Unknown command: ' + line.decode('utf-8'))
        # Once subscribed, keep sending TALLY messages.
        if is_subscribed:
            status = ['0'] * INPUTS
            active = random.randint(0, INPUTS-1)
            preview = random.randint(0, INPUTS-1)
            status[preview] = '2'
            status[active] = '1'
            print('> TALLY ' + ''.join(status))
            conn.send(('TALLY ' + ''.join(status) + '\r\n').encode('utf-8'))


# Start listening on TCP sockets and handle the arriving connections
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('0.0.0.0', PORT))
s.listen()

while True:
    conn, addr = s.accept()
    threading.Thread(target=handle, args=(conn,)).start()
