#!/usr/bin/perl

use strict;
use warnings;

undef $/;

for my $file (@ARGV)
{
    open FILE, "<$file";
    my $data = <FILE>;
    close FILE;

    $data =~ s/<link .*?>//gs;
    $data =~ s/<div id="jump-to-nav".*?<\/div>//gs;
    $data =~ s/<script.*?<\/script>//gs;
    $data =~ s/<style.*?<\/style>//gs;
    $data =~ s/<h3 id="siteSub".*?<\/h3>//gs;
    $data =~ s/<table id="toc".*?<\/table>//gs;
    $data =~ s/<div class="editsection".*?<\/div>//gs;
    $data =~ s/<div id="catlinks".*/<\/div><\/div><\/div><\/div><\/body><\/html>/gs;
    $data =~ s/<!-- Saved in parser cache.*?-->//gs;
    $data =~ s/<a href="http:.*?>(.*?)<\/a>/$1/gs;

    open FILE, ">$file";
    print FILE $data;
    close FILE;
}

