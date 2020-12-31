#!/bin/bash

iptables -A INPUT -p ip -j DROP
echo 0 > /proc/sys/net/ipv4/ip_forward
