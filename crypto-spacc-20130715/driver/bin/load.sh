#!/bin/bash

modprobe rng-core
insmod elppdu.ko
insmod elpmem.ko

for f in elpspacc.ko elpre.ko elpkep.ko elptrng.ko elppka.ko elpmpm.ko elpspacccrypt.ko elpmpmdiag.ko elpreusr.ko elpkepdev.ko elpea.ko elpeadiag.ko elpsaspa.ko elpsaspadiag.ko; do
   if [ -e $f ]; then insmod $f; fi;
done

