#!/bin/bash

if [ "$1" = "1" ];
then

PATH=/bin:/usr/bin
SUBJ="Case controller connected"

mail -s "$SUBJ" root@localhost << __MESSAGE__
Case controller connected ($(hostname)) on $(date)

__MESSAGE__

else

PATH=/bin:/usr/bin
SUBJ="Case controller disconnected"

mail -s "$SUBJ" root@localhost << __MESSAGE__
Case controller disconnected ($(hostname)) on $(date)

__MESSAGE__

fi

exit 0
