#!/usr/bin/perl

# Copyright (C) 2008 by Tim Nugent
# timnugent@gmail.com
#
# This file is part of hugin.
#
# Hugin is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# Hugin is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Hugin  If not, see <http://www.gnu.org/licenses/>.
 

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
