#!/usr/bin/env python
# -*- coding: utf-8 -*-

# file xrc_pot.py
# This file is part of hugin
# The idea inspired from LyX
# Licence details can be found in the file COPYING.
#
# autors Kornel Benko <Kornel.Benko@berlin.de>
#        Thomas Modes  <Thomas.Modes@gmx.de>
#
# original \author Bo Peng
#
# Full author contact details are available in file CREDITS

# Usage: use
#     xrc_pot.py -h
# to get usage message

# This script will extract translatable strings from input files and write
# to output in gettext .pot format.
#
import sys, os, re, getopt
import codecs

if sys.version_info < (2, 4, 0):
    from sets import Set as set

def relativePath(path, base):
    '''return relative path from top source dir'''
    # full pathname of path
    path1 = os.path.normpath(os.path.realpath(path)).split(os.sep)
    path2 = os.path.normpath(os.path.realpath(base)).split(os.sep)
    if path1[:len(path2)] != path2:
        sys.out.write("Path %s is not under top source directory\n" % path)
    path3 = os.path.join(*path1[len(path2):]);
    # replace all \ by / such that we get the same comments on Windows and *nix
    path3 = path3.replace('\\', '/')
    return path3


def writeString(outfile, infile, basefile, lineno, string):
    string = string.replace('\\', '\\\\').replace('"', '')
    if string == "":
        return
    outfile.write('#: %s:%d\nmsgid "%s"\nmsgstr ""\n\n' % \
        (relativePath(infile, basefile), lineno, string))

def convertString(string):
    string = string.replace('&amp;', '&').replace('&quot;', '"')
    string = string.replace('_', '&')
    string = string.replace('&lt;', '<').replace('&gt;', '>')
    string = string.replace('\\n', r'&#x0a;')
    string = string.replace('\\', '\\\\').replace('"', r'\"')
    string = string.replace('&#x0a;', r'\n')
    #string = string.decode('iso-8859-1')    # this is for python2 OK
    return string

def xrc_l10n(input_files, output, base):
    '''Generate pot file from src/..../*.xrc'''
    output = codecs.open(output, 'a', 'utf-8')
    #output = open(output, 'a')
    pat_ret = re.compile(r'(.*)\r')
    pat_txt = re.compile(r'\s*<(label|help|value|longhelp|tooltip|htmlcode|title|item)(\s+[^>]*)?>(.*)</\1>')
    pat_multi = re.compile(r'\s*<(label|help|value|longhelp|tooltip|htmlcode|title|item)(\s+[^>]*)?>(.*)$')
    pat_end = re.compile(r'(.*)</(label|help|value|longhelp|tooltip|htmlcode|title|item)>')
    pat_whole = re.compile(r'(.*)$')
    numeric = re.compile(r'^\d+$')
    ignored = ["_(\"\");\n"]
    for line in open(base + "/translations/ignored-strings.txt"):
        ignored.append(line)
    nodewait = ""		# expect this node in closing on multiline
    multi = 0		# multiline text
    stringmulti = ""
    string = ""
    stringline = 0
    for src in input_files:
        input = codecs.open(src, 'r', 'iso-8859-1')
        for lineno, line in enumerate(input.readlines()):
            # get lines that match e.g. <label>...</label>
            if pat_ret.match(line):
                (line,) = pat_ret.match(line).groups()
            if multi > 0:
                if pat_end.match(line):
                    (string2, node2) = pat_end.match(line).groups()
                    stringline = lineno + 1 - multi
                    if not node2 == nodewait:
                      sys.out.write("node2 = \"" + node2 + "\"\n" + \
                        "nodewait = \"" + nodewait + "\"\n" + \
                        "file = \"" + src + "\":" + str(stringline))
                      exit(1)
                    multi = 0
                    string = stringmulti + '\\n' + convertString(string2)
                    stringmulti = ""
                else:
                    (string2,) = pat_whole.match(line).groups()
                    stringmulti = stringmulti + '\\n' + convertString(string2)
                    multi += 1
                    continue
            elif pat_txt.match(line):
                (node,pars,string,) = pat_txt.match(line).groups()
                if numeric.match(string):
                    continue
                string = convertString(string)
                stringline = lineno
            elif pat_multi.match(line):
                (node,pars,string,) = pat_multi.match(line).groups()
                nodewait = node
                multi = 1
                stringmulti = convertString(string)
                continue

            if numeric.match(string):
                string = ""
                continue
            istring = "_(\"" + string + "\");\n"
            if istring not in ignored:
                output.write('#: %s:%d\nmsgid "%s"\nmsgstr ""\n\n' % \
                    (relativePath(src, base), stringline+1, string))
            string = ""
        input.close()
    output.close()


Usage = '''
xrc_pot.py [-b|--base top_src_dir] [-o|--output output_file] [-h|--help] [-s|src_file filename] -t|--type input_type input_files

where
    --base:
        path to the top source directory. default to '.'
    --output:
        output pot file, default to './xrc.pot'
    --src_file
        filename that contains a list of input files in each line
    --input_type can be
        xrc: xrc files
'''

if __name__ == '__main__':
    input_type = None
    output = 'xrc.pot'
    base = '.'
    input_files = []
    #
    optlist, args = getopt.getopt(sys.argv[1:], 'ht:o:b:s:',
        ['help', 'type=', 'output=', 'base=', 'src_file='])
    for (opt, value) in optlist:
        if opt in ['-h', '--help']:
            sys.out.write(Usage + "\n")
            sys.exit(0)
        elif opt in ['-o', '--output']:
            output = value
        elif opt in ['-b', '--base']:
            base = value
        elif opt in ['-t', '--type']:
            input_type = value
        elif opt in ['-s', '--src_file']:
            input_files = [f.strip() for f in open(value)]

    if input_type not in ['xrc'] or output is None:
        sys.out.write('Wrong input type or output filename.\n')
        sys.exit(1)

    input_files += args

    if input_type == 'xrc':
        xrc_l10n(input_files, output, base)


