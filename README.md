LPC1114_Hex_Upload_Reroll
=========================

Rerolling the LPC1114 serial uploader

I've built my own [SMD LPC1114 board with BLE](http://letsmakerobots.com/content/valdez-mutant-v04-smd-lpc1114-board "").  Hoping to be able to wireless download to the chip

![alt tag](http://letsmakerobots.com/files/userpics/u19048/Valdez_Mutant_v04_Explained_1.jpg)

This is my attempt to re-roll the lpc21isp. Though, I'm not kidding myself, this code would be a stagnant mess if it weren't for [Bdk6's](http://letsmakerobots.com/users/bdk6 "Title") guidance. 

I got a little burnt; there seems no way, given current lpc21isp driver dependency, to control CTS on the FTDI.  And I'd love to see the LPC1114 have cheap support.  If CTS and DTR can be used then anyone who has the equipment to program an Arduino Pro Mini or Lilypad can program the LPC1114.  Without having to buy extra programmer then an LPC1114 could be as easy to upload as Arduino and cost a little more than an Arduino Pro Mini (~$5).  Worth it.  

