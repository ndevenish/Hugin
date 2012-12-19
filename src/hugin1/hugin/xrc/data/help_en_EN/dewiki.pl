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
    $data =~ s/<h3 id="siteSub".*?<\/h3>//gs;
    $data =~ s/<form .*?<\/form>//gs;
    $data =~ s/<table id="toc".*?<\/table>//gs;
    $data =~ s/<div class="editsection".*?<\/div>//gs;
    $data =~ s/<div [^>]*class=["']pBody["'].*?<\/div>//gs;
    $data =~ s/<div [^>]*class=["']portlet["'].*?<\/div>//gs;
    $data =~ s/<div class=["']generated-sidebar portlet["'].*?<\/div>//gs;
    $data =~ s/<div id=["']searchBody["'].*?<\/div>//gs;
    $data =~ s/<div id=["']column-one["'].*?<\/div>//gs;
    $data =~ s/<div id="(f-poweredbyico|f-copyrightico)".*?<\/div>//gs;
    $data =~ s/<div id=["']catlinks["'].*/<\/div><\/div><\/body><\/html>/gs;
    $data =~ s/<ul id="f-list">.*?<\/ul>//gs;
    $data =~ s/<!-- Saved in parser cache.*?-->//gs;
    $data =~ s/<a href="(https?:[^"]*).*?>(.*?)<\/a>/$2<a class="external" href="$1">[*]<\/a>/gs;
    $data =~ s/<!--[^>]*NewPP limit report[^>]*-->//gs;
    $data =~ s/<div[^>]*>[[:space:]]*<\/div>//gs;
    $data =~ s/<meta name="ResourceLoaderDynamicStyles".*? \/>/<style media="screen" type="text\/css" title="Screen style sheet"> \@import url(manual.css); <\/style>/s;

    open FILE, ">$file";
    print FILE $data;
    close FILE;
}

