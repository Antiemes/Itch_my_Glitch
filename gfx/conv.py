#!/usr/bin/env python3

from PIL import Image
im = Image.open('nem.png', 'r')
pix_val = list(im.getdata())

print('static const uint8_t nem[] PROGMEM = {')

bp = 0
bv = 0

for (r, g, b, a) in pix_val:
    bv |= (1 << bp) if r else 0
    bp += 1
    if bp == 8:
        bp = 0
        print(bv, end = ', ')
        bv = 0


print('};')

