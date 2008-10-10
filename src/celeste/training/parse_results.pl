#!/usr/bin/perl

use strict;
use warnings;
use Math::Round qw(:all);

my @files = `ls results`;
my $sum = 0;
foreach my $file (@files){
	
	$file =~ s/\s+//g;
	my $r = "results/".$file;
	
	open (FILE,$r);
	my @array = <FILE>;
	close FILE;
	
	my $total = 0;
	my $count = 0;
	
	foreach my $l (@array){
	
		my @tmp = split(/\s+/,$l);
		$total += $tmp[0];
		$count++;	
	}

	my $ave = nearest_ceil(0.0001,$total/$count);
	print "$file:\t$ave\t($count)\n";
	$sum += $count;

}
print "Total:\t$sum\n";
exit;
