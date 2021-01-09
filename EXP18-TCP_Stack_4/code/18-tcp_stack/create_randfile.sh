#!/bin/bash

dd if=/dev/urandom bs=100KB count=3 | base64 > client-input.dat
