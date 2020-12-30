#!/bin/bash

iptables -A INPUT -p icmp --icmp-type echo-request -j DROP
iptables -A OUTPUT -p icmp --icmp-type echo-reply -j DROP
iptables -I OUTPUT -p icmp --icmp-type destination-unreachable -j DROP
