#!/usr/bin/env python3

import sys
from PIL import Image
from contextlib import redirect_stdout

fname = sys.argv[1]

im = Image.open(fname, 'r')
pix_val = list(im.getdata())

imgname = fname.removesuffix('.png')
hname = imgname + '.h'
with open(hname, "w") as outf:
    with redirect_stdout(outf):
        print(f'static const uint8_t {imgname}[] PROGMEM = ' + '{')
        
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

