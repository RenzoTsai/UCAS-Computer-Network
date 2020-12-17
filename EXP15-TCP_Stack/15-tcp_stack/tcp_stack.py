#!/usr/bin/python

import sys
import string
import socket
from time import sleep


def server(port):
    s = socket.socket()
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    s.bind(('0.0.0.0', int(port)))
    s.listen(3)
    
    cs, addr = s.accept()
    
    sleep(5)
    
    s.close()


def client(ip, port):
    s = socket.socket()
    s.connect((ip, int(port)))
    
    sleep(1)
    
    s.close()

if __name__ == '__main__':
    if sys.argv[1] == 'server':
        server(sys.argv[2])
    elif sys.argv[1] == 'client':
        client(sys.argv[2], sys.argv[3])
