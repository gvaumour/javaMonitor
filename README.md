# Java Virtual Machine Instrumentation 
================

This project instruments the Java Virtual Machine and 
It allows to capture the number of accesses performed by the garbage collector compared to the application execution to understand better the impact of the garbage collector. The JVMTI_agent detects the JVM initialization and different calls to the garbage collector. It communicates all the phases to the pintools that compute the number of accesses. The communication between the JVMTI agent and the pintools is done through system calls where the JVMTI agent write to standard ouput some specific outputs. The pintools instruments the system calls and recognize the JVMTI agent ones to detect the JVM phases. 

## Compilation 

The pintools can be built with pintool version 3.6 that can be downloaded [here][1]. You need to update the $PIN_ROOT variable in the Makefile with your pin location.

## Running 

The script eval.sh shows how to use together the JVMTI agent with the pintools. You need to set all the variable according to your configuration
$PIN_ROOT/pin -t ./obj-intel64/gc_watch.so -- java -agentpath:../JVMTI_agent/libGC_monitoring.so <your app to evaluate>

Be prepared for the slow down du to the pintools =) 

[1]: https://software.intel.com/en-us/articles/pin-a-binary-instrumentation-tool-downloads
