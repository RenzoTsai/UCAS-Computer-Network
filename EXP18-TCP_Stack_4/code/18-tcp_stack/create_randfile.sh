#!/bin/bash

dd if=/dev/urandom bs=1MB count=1 | base64 > client-input.dat
