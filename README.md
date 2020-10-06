# rhizo-dialer
A Simple Mobile Telephony Dialer for Linux

To make or receive calls, run (after make and making sure the Quectel is online):

  dialer -m /dev/EG25.AT -s -p

TODO:
set serial port baud rate to 115200 and call "ATZ" (and clean the buffer?)

for now:
setserial /dev/EG25.AT spd_vhi

also, set include user to dialout group
