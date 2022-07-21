#!/usr/bin/env python
#
# this script is an stdin filter to stdout.
# is remove some geek font character used in custom prompt for exmaple
#

import sys

myinput = sys.stdin.read().strip()

unwanted_chars = "☸ﴃ✖"
replace_chars  = ">   x"

out = myinput.translate(str.maketrans(unwanted_chars, replace_chars))
print(out)
