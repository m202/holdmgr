#!/usr/bin/env perl

use CGI;
use File::Copy;
use Digest::SHA sha1_hex;

$query = CGI->new;

# Secret key used to control access to web scripts
# Use the same value in del.cgi, pass.cgi, and process-holds.sh
$KEY="secret";

$QUEUE = $query->param('q');
chomp $QUEUE;
$INKEY = $query->param('k');
$TEST = sha1_hex("${KEY}${QUEUE}");

print "Content-Type: text/plain\n";
print "\n";

if("$TEST" eq "$INKEY") {

	system ( "./queue_helper", "-d", $QUEUE );

	if ( $? != 0 ) {
		print "Return code = $?\n";
	}
	else {
		print "Success\n";
	}
} else {
	print "Invalid key\n";
}
