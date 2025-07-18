#!/usr/bin/env python3

import sys
from PIL import Image
from contextlib import redirect_stdout

fname = sys.argv[1]

im = Image.open(fname, 'r')
pix_val = list(im.getdata())

imgname = fname.removesuffix('.png')
hname = 'gfx_' + imgname + '.h'
with open(hname, "w") as outf:
    with redirect_stdout(outf):
        print(f'static const uint8_t gfx_{imgname}[] PROGMEM = ' + '{')
        
        bp = 0
        bv = 0
        
        for v in pix_val:
            (r, g, b) = v[:3]
            bv |= (1 << bp) if r else 0
            bp += 1
            if bp == 8:
                bp = 0
                print(bv, end = ', ')
                bv = 0
        
        
        print('};')

