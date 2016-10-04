#!/usr/bin/env perl

###############################################################################
# Number of untested lines of code
#
# This number should only ever be reduced.
# i.e. new code must not add untested lines
# This should be changed with each commit which improves test coverage.

my $untested_lines = 2474;


###############################################################################
# Number of code lines excluded from coverage stats
#
# This should ONLY be used where there is no way that a test for the code line(s)
# can be written.
# Code can be excluded as noted in https://linux.die.net/man/1/lcov

my $excluded_lines = 0;

###############################################################################

print "\n\n";
print "Parsing test coverage results\n\n";

# Parse lcov summary with exclusion markers
my $summary    = `lcov --summary lcov.txt 2>&1`;
die "Cannot find line summary" if ( $summary !~ m/^\s+lines\.+\: (\d+\.\d+)\% \((\d+) of (\d+) lines\)$/m );
my $percentage = $1;
my $tested_lines = $2;
my $total_lines = $3;

# Parse lcov summary without exclusion markers
my $summary = `lcov --summary lcov_output_without_exclusions.txt 2>&1`;
die "Cannot find line summary" if ( $summary !~ m/^\s+lines\.+\: (\d+\.\d+)\% \((\d+) of (\d+) lines\)$/m );
my $total_lines_nomarkers = $3;

my $new_excluded_lines = $total_lines_nomarkers - $total_lines;
my $new_untested_lines = $total_lines - $tested_lines;

my $new_untested_or_excluded_lines = $total_lines - $tested_lines + ($new_excluded_lines - $excluded_lines);

my $result = 0;

print "Untested lines: $new_untested_lines (was $untested_lines)\n";
print "Excluded lines: $new_excluded_lines (was $excluded_lines)\n";

if ($excluded_lines < $new_excluded_lines) {
	print STDERR "ERROR: Code added with test coverage exclusions. Add a justification to the commit message and update lcov_parse.pl with \$excluded_lines = $new_excluded_lines\n";
	$result = -1;
}
elsif ($excluded_lines > $new_excluded_lines) {
	print STDERR "NOTE: Please update lcov_parse.pl with \$excluded_lines = $new_excluded_lines\n";
	$result = -1;
}

if ($untested_lines < $new_untested_lines) {
	print STDERR "ERROR: Untested code added. Add tests for the code before resubmitting commit.\n";
	$result = -1;
}
elsif ($untested_lines > $new_untested_lines) {
	if ( ($new_excluded_lines - $excluded_lines) < ($new_untested_lines - $untested_lines) ) {
		print "Thank you for adding tests\n";
	}
	print STDERR "NOTE: Please update lcov_parse.pl with \$untested_lines = $new_untested_lines\n";
	$result = -1;
}

print "\n";

exit($result);
