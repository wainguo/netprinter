#!/bin/sh

#SHELLPATH=$(dirname $0)
SHELLPATH="/jffs/gprint"

GSTATUS="$SHELLPATH/gstatus"
GPRINT="$SHELLPATH/gprint"

#PRINTQUEUE="$SHELLPATH/queue";

function print_queue(){
	queue=$(ls -rt $PRINTQUEUE)
	for i in $queue
	do
		cd $SHELLPATH
		status=$($GSTATUS)
		echo "Printer status code <$status> .to be printed -- $i"
		if test $status -eq 0;then
			pr=$($GPRINT $PRINTQUEUE/$i)
			echo "Print status code <$pr> .to be printed -- $i"
			if test $pr -eq 0;then
				echo "Print success. to remove -- $i"
				rm "$PRINTQUEUE/$i"
			fi
		else
			break
		fi
	done
}

#if para $1 given, take $1 as the print queue path
if test -z $1;then
	PRINTQUEUE="$SHELLPATH/queue";
else
	if test -d $1;then
		PRINTQUEUE=$1;
	fi
fi

echo "PRINTQUEUE=$PRINTQUEUE";

while true; do
	print_queue;
	sleep 3;
done

