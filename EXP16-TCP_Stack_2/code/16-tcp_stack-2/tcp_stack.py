#!/usr/bin/python

import sys
import string
import socket
from time import sleep

data = string.digits + string.lowercase + string.uppercase

def server(port):
    s = socket.socket()
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    s.bind(('0.0.0.0', int(port)))
    s.listen(3)
    
    cs, addr = s.accept()
    print addr

    f = open('server-output.dat', 'wb')
    
    while True:
        data = cs.recv(1000)
        print len(data)
        f.write(data)
        if data:
            data = 'server echoes: ok'
            cs.send(data)
        if len(data) == 0:
            break
    
    f.close()
    s.close()


def client(ip, port):
    s = socket.socket()
    s.connect((ip, int(port)))
    f = open('client-input.dat', 'r')
    str = f.read()
    length = len(str)
    i = 0
    
    while length > 0:
        s.send(str[i:i+1000])
        length -= 1000

    f.close()
    s.close()

if __name__ == '__main__':
    if sys.argv[1] == 'server':
        server(sys.argv[2])
    elif sys.argv[1] == 'client':
        client(sys.argv[2], sys.argv[3])
