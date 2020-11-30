#!/bin/bash

arptables -A FORWARD -j DROP
arptables -A OUTPUT -j DROP
