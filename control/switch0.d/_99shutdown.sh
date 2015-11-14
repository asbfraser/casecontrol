#!/bin/bash

if [ "$1" == "1" ];
then
	/sbin/shutdown -h now
fi

exit 0
