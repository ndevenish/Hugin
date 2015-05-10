#! /usr/bin/env perl
# (C) 2010 Kornel Benko

# script to compare changes between translation files before merging them

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this software. If not, see
# <http://www.gnu.org/licenses/>.

use strict;

my ($status, $foundline, $msgid, $msgstr, $fuzzy);

my %Messages = ();              # Used for original po-file
my %newMessages = ();           # new po-file
my %Untranslated = ();          # inside new po-file
my %Fuzzy = ();                 # inside new po-file
my $result = 0;                 # exit value

if (@ARGV != 2) {
  die("Expected exactly 2 parameters");
}

&check("original", $ARGV[0]);
&check("new", $ARGV[1]);

&parse_po_file($ARGV[0], \%Messages);
&parse_po_file($ARGV[1], \%newMessages);

my @MsgKeys = &getLineSortedKeys(\%newMessages);

for my $k (@MsgKeys) {
  if ($newMessages{$k}->{msgstr} eq "") {
    # this is still untranslated string
    $Untranslated{$newMessages{$k}->{line}} = $k;
  }
  elsif ($newMessages{$k}->{fuzzy}) {
    #fuzzy string
    $Fuzzy{$newMessages{$k}->{line}} = $k;
  }
  if (exists($Messages{$k})) {
    &printIfDiff($k, $Messages{$k}, $newMessages{$k});
    delete($Messages{$k});
    delete($newMessages{$k});
  }
}

@MsgKeys = &getLineSortedKeys(\%Messages);
for my $k (@MsgKeys) {
  $result |= 8;
  print "deleted message\n";
  print "< line = " . $Messages{$k}->{line} . "\n";
  print "< fuzzy = " . $Messages{$k}->{fuzzy} . "\n";
  print "< msgid = \"$k\"\n";
  print "< msgstr = \"" . $Messages{$k}->{msgstr} . "\"\n";
}

@MsgKeys = &getLineSortedKeys(\%newMessages);
for my $k (@MsgKeys) {
  $result |= 16;
  print "new message\n";
  print "> line = " . $newMessages{$k}->{line} . "\n";
  print "> fuzzy = " . $newMessages{$k}->{fuzzy} . "\n";
  print "> msgid = \"$k\"\n";
  print "> msgstr = \"" . $newMessages{$k}->{msgstr} . "\"\n";
}

&printExtraMessages("fuzzy", \%Fuzzy);
&printExtraMessages("untranslated", \%Untranslated);

exit($result);

sub check($$)
{
  my ($spec, $filename) = @_;

  if (! -e $filename ) {
    die("$spec po file does not exist");
  }
  if ( ! -f $filename ) {
    die("$spec po file is not regular");
  }
  if ( ! -r $filename ) {
    die("$spec po file is not readable");
  }
}

sub printDiff($$$$)
{
  my ($k, $nk, $rM, $rnM) = @_;
  print "diffline = " . $rM->{line} . "," . $rnM->{line} . "\n";
  print "< msgid = \"$k\"\n";
  print "< fuzzy = " . $rM->{fuzzy} . "\n";
  print "< msgstr = " . $rM->{msgstr} . "\n";
  if ($k ne $nk) {
    print "> msgid = \"$nk\"\n";
  }
  print "> fuzzy = " . $rnM->{fuzzy} . "\n";
  print "> msgstr = " . $rnM->{msgstr} . "\n";
  print "\n";
}

sub printIfDiff($$$)
{
  my ($k, $rM, $rnM) = @_;
  my $doprint = 0;
  $doprint = 1 if ($rM->{fuzzy} != $rnM ->{fuzzy});
  $doprint = 1 if ($rM->{msgstr} ne $rnM ->{msgstr});
  if ($doprint) {
    $result |= 4;
    &printDiff($k, $k, $rM, $rnM);
  }
}

sub parse_po_file($$)
{
  my ($file, $rMessages) = @_;
  if (open(FI, '<', $file)) {
    $status = "normal";
    $fuzzy = 0;
    my $lineno = 0;
    while (my $line = <FI>) {
      $lineno++;
      &parse_po_line($line, $lineno, $rMessages);
    }
    &parse_po_line("", $lineno + 1, $rMessages);
    close(FI);
  }
}

sub parse_po_line($$$)
{
  my ($line, $lineno, $rMessages) = @_;
  chomp($line);

  if ($status eq "normal") {
    if ($line =~ /^#, fuzzy/) {
      $fuzzy = 1;
    }
    elsif ($line =~ s/^msgid\s+//) {
      $foundline = $lineno;
      $status = "msgid";
      $msgid = "";
      &parse_po_line($line);
    }
  }
  elsif ($status eq "msgid") {
    if ($line =~ /^\s*"(.*)"\s*/) {
      $msgid .= $1;
    }
    elsif ($line =~ s/^msgstr\s+//) {
      $status = "msgstr";
      $msgstr = "";
      &parse_po_line($line);
    }
  }
  elsif ($status eq "msgstr") {
    if ($line =~ /^\s*"(.*)"\s*/) {
      $msgstr .= $1;
    }
    else {
      if ($msgid ne "") {
	$rMessages->{$msgid}->{line} = $foundline;
	$rMessages->{$msgid}->{fuzzy} = $fuzzy;
	$rMessages->{$msgid}->{msgstr} = $msgstr;
      }
      $fuzzy = 0;
      $status = "normal";
    }
  }
  else {
    die("invalid status");
  }
}

sub getLineSortedKeys($)
{
  my ($rMessages) = @_;

  return sort {$rMessages->{$a}->{line} <=> $rMessages->{$b}->{line};} keys %{$rMessages};
}

sub printExtraMessages($$)
{
  my ($type, $rExtra) = @_;
  my @UntranslatedKeys = sort { $a <=> $b;} keys %{$rExtra};

  if (@UntranslatedKeys > 0) {
    print "Still " . 0 + @UntranslatedKeys . " $type messages found in $ARGV[1]\n";
    for my $l (@UntranslatedKeys) {
      print "> line $l: \"" . $rExtra->{$l} . "\"\n"; 
    }
  }
}
