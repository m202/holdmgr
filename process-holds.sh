#!/bin/bash

# This can be added to root's crontab

# Secret key used to control access to web scripts
# Use the same value in del.cgi, pass.cgi, and process-holds.sh
KEY="secret"

# Notifications and links to CGI scripts are sent to this address
TO_ADDR="root@localhost"
TO="root <$TO_ADDR>"

# Location of CGI scripts
URL="http://localhost/cgi-bin"

# Tag added to e-mail subject of hold notifications
TAG="HOLD"

export PATH=/bin:/usr/bin:/sbin:/usr/sbin
cd /var/spool/postfix/hold

for q in `ls .`; do
	TMP=`mktemp /tmp/holdmgr.XXXX`
	KEY=`echo -en ${KEY}${q} | sha1sum - | cut -f1 -d\  `

	postcat -e -h $q > $TMP
	SUBJECT="`grep -m1 ^Subject: $TMP | cut -f2- -d:`"
	FROM="`grep -m1 ^sender: $TMP | cut -f2- -d:`"

	echo "To: $TO" > $TMP
	echo "From:$FROM" >> $TMP
	echo "Subject: [$TAG $q]$SUBJECT" >> $TMP
	echo "Content-Type: text/plain; charset=\"utf-8\"" >> $TMP
	echo >> $TMP
	echo "Delete: $URL/del.cgi?q=${q}&k=${KEY}" >> $TMP
	echo "Deliver: $URL/pass.cgi?q=${q}&k=${KEY}" >> $TMP
	echo >> $TMP
	echo "------------------------------------------------------------" >> $TMP
	echo >> $TMP
	postcat -h -b $q >> $TMP

	mkdir -p .scanned
	mv $q .scanned
	cat $TMP | sendmail $TO_ADDR > /dev/null
	rm -f $TMP

	#cat $TMP

done
