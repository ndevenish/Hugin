#!/bin/env python

ignored = []
for line in open("ignored-strings.txt"):
    ignored.append(line)

filtered = []
for string in open("xrc.cpp"):
    if string not in ignored:
        filtered.append(string)

output = open("xrc-filtered.cpp", "w")
for string in filtered:
    output.write(string)
