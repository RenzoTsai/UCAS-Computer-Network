#!/bin/bash

iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP
