# downloads a selection of pages specified in the 'pages.txt' file from the
# Panotools wiki at http://wiki.panotools.org/ and assembles them into an
# offline HTML manual for Hugin.
#
# Requires wget and perl
#
# Bruno Postle <bruno@postle.net>

all: fetch dewiki

fetch: pages.txt
	sh fetch.sh

dewiki:
	perl dewiki.pl *.html

.PHONY: fetch dewiki

