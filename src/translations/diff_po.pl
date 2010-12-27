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
# License along with this software; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

use strict;

my ($status, $foundline, $msgid, $msgstr, $fuzzy);

my %Messages = ();
my %newMessages = ();
if (@ARGV != 2) {
  die("Usage: diff_po.pl <existing.po> <contributed.po> - Expected exactly 2 parameters");
}

&check("first", $ARGV[0]);
&check("second", $ARGV[1]);

&parse_po_file($ARGV[0], \%Messages);
&parse_po_file($ARGV[1], \%newMessages);

my @MsgKeys = keys %Messages;

for my $k (@MsgKeys) {
  if (exists($newMessages{$k})) {
    &printIfDiff($k, $Messages{$k}, $newMessages{$k});
    delete($Messages{$k});
    delete($newMessages{$k});
  }
}

for my $k (keys %Messages) {
  print "deleted message\n";
  print "line = " . $Messages{$k}->{line} . "\n";
  print "fuzzy = " . $Messages{$k}->{fuzzy} . "\n";
  print "msgid = \"$k\"\n";
  print "msgstr = \"" . $Messages{$k}->{msgstr} . "\"\n";
}

for my $k (keys %newMessages) {
  print "new message\n";
  print "line = " . $newMessages{$k}->{line} . "\n";
  print "fuzzy = " . $newMessages{$k}->{fuzzy} . "\n";
  print "msgid = \"$k\"\n";
  print "msgstr = \"" . $newMessages{$k}->{msgstr} . "\"\n";
}

exit(0);

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

sub printIfDiff($$$)
{
  my ($k, $rM, $rnM) = @_;
  my $doprint = 0;
  $doprint = 1 if ($rM->{fuzzy} != $rnM ->{fuzzy});
  $doprint = 1 if ($rM->{msgstr} != $rnM ->{msgstr});
  if ($doprint) {
    print "diffline = " . $rM->{line} . "," . $rnM->{line} . "\n";
    print "msgid = \"$k\"\n";
    print "< fuzzy = " . $rM->{fuzzy} . "\n";
    print "< msgstr = " . $rM->{msgstr} . "\n";
    print "> fuzzy = " . $rnM->{fuzzy} . "\n";
    print "> msgstr = " . $rnM->{msgstr} . "\n";
    print "\n";
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

