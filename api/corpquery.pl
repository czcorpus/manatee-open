#!/usr/bin/perl -w
# Copyright 2007-2013  Pavel Rychly

use manatee;

sub print_kwic($$$$) {
    my ($posattr, $begin, $end, $context) = @_;
    my $iter = $posattr->textat($begin < $context ? 0 : $begin - $context);
    foreach (0 .. ($begin < $context ? $begin : $context) -1) {
	print $iter->next(), " ";
    }
    print "<";
    foreach (0 .. ($end - $begin) -1) {
	print $iter->next(), " ";
    }
    print "> ";
    foreach (0 .. $context -1) {
	print $iter->next(), " ";
    }
    print "\n";
}

$refattr = 'doc.file';
$context = 10;
$hardcut = 15;
    
sub corp_query($$) {
    my ($corpname, $query) = @_;
    my $corp = new manatee::Corpus ($corpname);
    my $result = $corp->eval_query ($query);

    my $wordattr = $corp->get_attr ('-');
    my $ref;
    if ($refattr) {
        $ref = $corp->get_attr ($refattr);
    }

    my $lines = $hardcut;
    while ($lines-- > 0 && ! $result->end()) {
        my $pos = $result->peek_beg();
	print "#$pos";
        if ($refattr) {
            print ',', $ref->pos2str ($pos);
	}
	print " ";
        print_kwic ($wordattr, $pos, $result->peek_end(), $context);
	$result->next();
    }
}


if ($#ARGV == 1) {
    corp_query ($ARGV[0], $ARGV[1]);
} else {
    print "usage: corpquery.pl CORPUSNAME QUERY\n";
}
    
