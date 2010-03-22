#!/usr/bin/env python

import sys

ignored = []
for line in open("ignored-strings.txt"):
    ignored.append(line)

filtered = []
for string in open(sys.argv[1]):
    if string not in ignored:
        filtered.append(string)

output = open(sys.argv[1], "w")
for string in filtered:
    output.write(string)
