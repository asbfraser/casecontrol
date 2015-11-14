#!/bin/bash

if [ "$1" == "1" ];
then

PATH=/bin:/usr/bin
SUBJ="Alert - case switch activated"

mail -s "$SUBJ" root@localhost << __MESSAGE__
ALERT - Case switch activated ($(hostname)) on $(date)

__MESSAGE__

fi

exit 0
