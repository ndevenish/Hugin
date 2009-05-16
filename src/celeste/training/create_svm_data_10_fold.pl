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
 
# Script to create test/training data for 10 fold cross-validation

# Put cropped images in cropped_images/
# in the format imageP.jpg and imageN.jpg for
# positive (cloud) and negative (non-cloud) examples
# Put masked images in masked_images/
# in the format image.jpg (image) and imageM.jpg
# (black & white mask, black = cloud, white = non-cloud)

use strict;
use warnings;
use Math::Round qw(:all);

my $training_dir = "image_data/";
my $fold_dir = "fold_data/";
my $folds_file = "10fold.dat";
my $params = "r10_s18_a8_f6_l0.1_u1.8_uv_avestdcen";

my ($system,%training,%test,$length,%max,%min,@pos,@neg,@mask);

print "Processing cropped images...\n\n";

my @files = <cropped_images/*>;
foreach(@files){
	if ($_ =~ /(\w+_\w+P)\.jpg/){
		my $outfile = $training_dir.$1.".dat";
		print "./celeste_train -p $_ > $outfile\n\n" unless -e $outfile;
		$system = `./celeste_train -p $_ > $outfile` unless -e $outfile;
		push @pos,$outfile;
	}
	if ($_ =~ /(\w+_\w+N)\.jpg/){
		my $outfile = $training_dir.$1.".dat";
		print "./celeste_train -n $_ > $outfile\n\n" unless -e $outfile;
		$system = `./celeste_train -n $_ > $outfile` unless -e $outfile;
		push @neg,$outfile;
	}
}


print "Processing masked images...\n\n";

@files = <masked_images/*>;
foreach(@files){
	if ($_ !~ /M/){
		foreach my $nm (@files){
			next if $nm !~ /M/;
			$length = length $_;
			if (substr($nm,0,$length-4) eq substr($_,0,$length-4)){
				$training{$_} = $nm;
			}
		}
	}
}

foreach(sort{$a cmp $b}  keys %training){
	
	if ($_ =~ /(\w+_\w+)\.jpg/){
		my $outfile = $training_dir.$1."M.dat";
		print "./celeste_train $_ $training{$_} > $outfile\n\n" unless -e $outfile;
		$system = `./celeste_train $_ $training{$_} > $outfile` unless -e $outfile;
		push @mask,$outfile;
	}
}

print "\nPositive images:\t",scalar @pos,"\n";
print "Negative images:\t",scalar @neg,"\n";
print "Masked images:\t\t",scalar @mask,"\n";

my $pos_limit = (scalar @pos)/10;
my $neg_limit = (scalar @neg)/10;
my $mask_limit = (scalar @mask)/10;


die "\nNot enough images to create folds!\n\n" unless $pos_limit >= 1;
die "Not enough images to create folds!\n\n" unless $neg_limit >= 1;
die "Not enough images to create folds!\n\n" unless $mask_limit >= 1;

print "Creating folds...\n\n";

unless (-e $folds_file){

	for my $f (1..10){
		
		# Create the folds

		open(FILE,">>$folds_file");		
		print FILE "FOLD $f: ";
		print FILE " " unless $f == 10;
		
		for (1..$pos_limit){
			print FILE "$pos[0] ";		
			shift @pos;
		}		
		for (1..$neg_limit){
			print FILE "$neg[0] ";		
			shift @neg;
		}	
		for (1..$mask_limit){
			print FILE "$mask[0] ";		
			shift @mask;
		}
		print FILE "\n";
		close FILE;
		
	}	
}

for my $f (1..10){

	print "Creating fold $f:\n";

	my $fold_test = $fold_dir."fold_".$f."_test_".$params.".dat";
	my $fold_train = $fold_dir."fold_".$f."_train_".$params.".dat";
	my $fold_train_pos = $fold_dir."fold_".$f."_train_".$params."_pos.dat";
	my $fold_train_neg = $fold_dir."fold_".$f."_train_".$params."_neg.dat";

	$system = `rm $fold_test` if -e $fold_test;
	$system = `rm $fold_train` if -e $fold_train;
	$system = `rm $fold_train_pos` if -e $fold_train_pos;
	$system = `rm $fold_train_neg` if -e $fold_train_neg;

	# Read the folds file and create training and test files
	
	if (-e $folds_file){

		open(FILE,$folds_file);
		my @file = <FILE>;
		close FILE;
		foreach my $l (@file){
		
			my @tmp = split(/\s/,$l);
			$tmp[1] =~ s/\://;
		
			if ($tmp[1] == $f){
				
				foreach my $t (@tmp){
					next if $t eq " ";
					next unless -e $t;
					print "cat $t >> $fold_test\n";
					$system = `cat $t >> $fold_test`;
				
				}			
			}else{
				foreach my $t (@tmp){
					next if $t eq " ";
					next unless -e $t;
					print "cat $t >> $fold_train\n";
					$system = `cat $t >> $fold_train`;
				}			
			}		
		}
		
		$system = `cat $fold_test | grep "^+1" > $fold_train_pos`;
		$system = `cat $fold_test | grep "^-1" > $fold_train_neg`;
		$system = `rm $fold_test`;
			
	}
	print "\n";
}

print "Done.\n";
exit;



