#!/usr/bin/perl
print << '.';
/*
 * This header file is auto-generated from pttbbs/mbbsd/var.c .
 * Please do NOT edit this file directly.
 */

#ifndef INCLUDE_VAR_H
#define INCLUDE_VAR_H
#include "bbs.h"
.
while( <> ){
    if( /^\w/ ){
	chomp;
	$_ = substr($_, 0, index($_, '=') - 1) if( index($_, '=') != -1 );
	$_ .= ';' if( index($_, ';') == -1 );
	print "extern $_\n";
    } elsif( /^\#/ && !/include/ ){
	print;
    }
}
print "#endif /* INCLUDE_VAR_H */\n";

