#! /usr/bin/perl -w

open HELPI, "help.txt.in";
open HELPO, ">help.txt";
while(<HELPI>)
{
 $_ =~ s/\\([0-9|A-Z]{2})/pack(C,hex($1))/eg;
 print HELPO $_;
}
close HELPI;
close HELPO;
