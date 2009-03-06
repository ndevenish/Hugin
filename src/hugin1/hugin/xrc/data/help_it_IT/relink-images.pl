#!/usr/bin/perl

use strict;
use warnings;

undef $/;

for my $file (@ARGV)
{
    open FILE, "<$file";
    my $data = <FILE>;
    close FILE;

    $data =~ s/<img([^>]*) src="([^\/"]*)"/<img$1 src="..\/help_en_EN\/$2"/gs;

    open FILE, ">$file";
    print FILE $data;
    close FILE;
}

