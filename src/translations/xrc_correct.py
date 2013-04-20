#! /usr/bin/env python
# -*- coding: iso-8859-15 -*-

# file xrc_correct.py
# This file is part of hugin
# Licence details can be found in the file COPYING.
#
# author Kornel Benko <Kornel.Benko@berlin.de>
#

import sys, re
import codecs
from getopt import getopt

usage = '''
python xrc_correct.py [-o OUTFILE] FILE

Change charset of given po-template to UTF-8

If the -o argument is not given, writes to stdout.
'''

outfile = ""

(options, args) = getopt(sys.argv[1:], "ho:")
for (opt, param) in options:
        if opt == "-o":
                outfile = param
        elif opt == "-h":
                sys.stdout.write(usage + "\n")
                sys.exit(0)

charset_pat = re.compile(r'^"(Content-Type:\s+text/plain;\s+charset=)CHARSET\\n"[\r\n]*$',re.IGNORECASE)
if outfile:
        out = codecs.open(outfile, "w", 'utf-8')
else:
        out = sys.stdout
for f in args:
        fil = codecs.open(f, "r", 'utf-8')
        for l in fil:
            if charset_pat.match(l):
              (string,) = charset_pat.match(l).groups()
              l = "\"" + string + "UTF-8\\n\"\n"
            out.write(l)
        fil.close()

if outfile:
        out.close()
