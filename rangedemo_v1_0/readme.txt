This folder contains a simple range demo for the nRF24E1/nRF9E5.

It works as follows. The transmitter continously send one byte packets. Each
time the receiver receives a packet the P00 pin is set low (LED1 is turned on on
the 9E5 eval board). At the same time a 20ms timer is started and if a new
packets is not received before the 20ms time-out the P00 pin is set high (LED1
is turned off). If a new packet is received before the time-out a new 20ms
time-out period is started.

You can change the channel number, power setting and so on in the source file.
The precompiled hex-files included here use channel 351 for the nRF9E5 and
channel 2 for the nRF24E1. Both on max power.
