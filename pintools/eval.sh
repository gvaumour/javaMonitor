#!/bin/bash

PIN=/home/gvaumour/Dev/apps/pin/pin-3.2-81205-gcc-linux/pin
	
MY_AGENT=../JVMTI_agent/libGC_monitoring.so
J2SDK=/usr/lib/jvm/java-8-openjdk-amd64
DACAPO=/home/gvaumour/Dev/benchmark/dacapo/dacapo-9.12-bach.jar

CMD=$J2SDK"/bin/java -agentpath:"$MY_AGENT" -jar "$DACAPO" -v avrora -s small"

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/gvaumour/Draft/JAVA/JVMTI/agentGreg/
$PIN -follow-execv -t ./obj-intel64/gc_watch.so -- $CMD

