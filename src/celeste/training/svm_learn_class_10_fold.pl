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

# Script to train SVM then classify +ve and -ve datasets.
# Results are printed and written to $results
# Require SVM light - http://svmlight.joachims.org/
# Set the paths to svm_learn and svm_classify below

use strict;
use warnings;
use Math::Round qw(:all);

## Paths
my $svm_learn    = "svm_learn";
my $svm_classify = "svm_classify";

my @files = `ls -tr fold_data | grep -v pos | grep -v neg | grep dat`;
foreach my $file (@files){
	
	$file =~ s/\s+//g;
	print "Processing $file\n";

	my $params;
	my $training = "fold_data/".$file;
	my $pos_test = "fold_data/";
	my $neg_test = "fold_data/";
	my $predictions_pos = "fold_data/";
	my $predictions_neg = "fold_data/";
	my $model = "fold_data/";
	my $results = "results/";

	if ($file =~ /(fold_\d+_train_)(\w+_\w+_\w+_\w+_\w+\.\w+_\w+\.\w+_\w+_\w+)\.dat/){
		$model .= $1.$2.".model";
		$pos_test .= $1.$2."_pos.dat";
		$neg_test .= $1.$2."_neg.dat";
		$predictions_pos .= $1.$2."_pos.pred";
		$predictions_neg .= $1.$2."_neg.pred";
		$params = $2;
		$results .= $params.".results";
	}

	if (-e $model){
		print "Model already exists\n";
		next;	
	}	

	unless (-e $training){
		die "$training doesn't exist!";
	}
	unless (-e $pos_test){
		die "$pos_test doesn't exist!";
	}
	unless (-e $neg_test){
		die "$neg_test doesn't exist!";
	}

	my $neg_count = `cat $training | grep -c \"^-1\"`;
	my $pos_count = `cat $training | grep -c \"^+1\"`;


	## Learning options
	my $t = 2;
	my $bi = 0;
	my $j = nearest_ceil(0.0001,$neg_count/$pos_count);
	my $c = 12;
	my $z = 'c';

	## RBF params
	my $g = 0.0007;
	my $i = 0;
	my $w = 0.1;

	## Order of polynomial
	my $d = 3;
	my $s = 0;
	my $r = 1;

	## Optimisation
	my $m = 500;
	my $v = 0;

	my $tp = 0;
	my $fp = 0;
	my $tn = 0;
	my $fn = 0;
	my $mathews = 0;
	my $fp_rate = 0;
	my $fn_rate = 0;
	my $accuracy = 0;
	my $dp = 0.00001;

	print "Learning:\n";
	print "$svm_learn -t $t -d $d -g $g -b $bi -j $j -c $c -i $i -z $z -w $w -v $v -m $m $training $model\n";
	my $svm = `$svm_learn -t $t -d $d -g $g -b $bi -j $j -c $c -i $i -z $z -w $w -v $v -m $m $training $model`;
	print "$svm\n\n" if $svm;

	print "\nClassifying clouds:\n";
	print "$svm_classify $pos_test $model $predictions_pos\n";
	$svm = `$svm_classify $pos_test $model $predictions_pos`;
	print "$svm\n";

	my @tmp = split (/\n/,$svm);
	foreach (@tmp){
		if ($_ =~ /Accuracy\son\stest\sset:\s(\d+.\d+)%\s\((\d+)\scorrect,\s(\d+)\sincorrect,\s(\d+)\stotal\)/){
			$tp = $2;
			$fp = $3;
		}
	}

	print "Classifying non-clouds:\n";
	print "$svm_classify $neg_test $model $predictions_neg\n";
	$svm = `$svm_classify $neg_test $model $predictions_neg`;
	print "$svm\n";

	@tmp = split (/\n/,$svm);
	foreach (@tmp){
		if ($_ =~ /Accuracy\son\stest\sset:\s(\d+.\d+)%\s\((\d+)\scorrect,\s(\d+)\sincorrect,\s(\d+)\stotal\)/){
			$tn = $2;
			$fn = $3;
		}
	}


	#`rm $predictions_pos` if -e $predictions_pos;
	#`rm $predictions_neg` if -e $predictions_neg;
	#`rm $model` if -e $model;

	my $top = (($tp * $tn)-($fn * $fp));
	my $bottom = (sqrt(($tn + $fn)*($tp + $fn)*($tn + $fp)*($tp + $fp)));
	$mathews = nearest_ceil($dp,($top/$bottom)) if ($top&&$bottom);   
	$fp_rate = 100 * nearest_ceil($dp,$fp/($tp+$fp)) if $fp;
	$fn_rate = 100 * nearest_ceil($dp,$fn/($tn+$fn)) if $fn;
	$accuracy = 100 * nearest_ceil($dp,($tp + $tn)/($tp + $tn + $fp + $fn));

	print "Results:\n";
	print "MCC:\t\t$mathews\n";
	print "FP Rate:\t$fp_rate %\n"; 
	print "FN Rate:\t$fn_rate %\n"; 
	print "Accuracy:\t$accuracy %\n\n"; 

	open (OUT,">>$results");
	print OUT "$mathews\t-t $t -d $d -g $g -b $bi -j $j -c $c -i $i -z $z -w $w -v $v -m $m --- $params\n";
	close OUT;

}
exit;
