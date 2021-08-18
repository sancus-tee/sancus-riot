#! /bin/bash
# Build project
make all
# Fill in macs of the Sancus Modules if they exist
sancus-crypto --fill-macs bin/sancus-msp430/hello-world.elf -o macs.elf

# Destroy potential old screen session
screen -X -S riot_prints quit
# Load elf file to device
sancus-loader -device /dev/ttyUSB0 macs.elf
# Create new named screen session to listen for prints
screen -S riot_prints /dev/ttyUSB0 115200
