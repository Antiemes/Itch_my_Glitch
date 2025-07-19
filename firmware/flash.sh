sudo avrdude -c dragon_isp -p m328p -P usb -U flash:w:./.pio/build/uno/firmware.hex
