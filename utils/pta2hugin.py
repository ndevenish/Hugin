#!/usr/bin/python

#
# one of my first tries at python
#

import re;
import sys;
import string;

img_regexp = re.compile(r'^\#-imgfile (\d+) (\d+) "(.+)"');


if len(sys.argv) <2:
    print "PTAssembler project to hugin converter\n"
    print "Usage:  $sys.argv[0] project.ptp [strip image filename prefix]\n"
    print "   Prints the converted script to standart out\n"
    sys.exit(1);

f = open(sys.argv[1]);

if len(sys.argv) == 3:
    prefix = sys.argv[2];
else:
    prefix = "";

for line in f:
    line = line.replace("\n","")
    line = line.replace("\r","")
    m = img_regexp.match(line)
    if m:
        width = m.group(1);
        height = m.group(2);
        filename = m.group(3);
        if prefix == "":
            prefix = filename[:filename.rfind('\\')+1];
#            print 'new prefix: %s' % (prefix);
        if filename[:len(prefix)] == prefix:
            filename = filename[len(prefix):];
            
        filename = filename.replace("\\","/");
#        print 'new filename: %s\n' %(filename)
    if line[0:2] == "o ":
        line ="i %s w%s h%s n\"%s\"" % (line[2:], width, height, filename)

    print line
