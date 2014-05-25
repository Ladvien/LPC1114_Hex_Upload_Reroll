LPC1114_Hex_Upload_Reroll
=========================

Rerolling the LPC1114 serial uploader

This is my attempt to re-roll the lpc21isp.  I got a little burnt; there seems no way given current lpc21isp driver dependency to control CTS on the FTDI.  And I'd love to see the LPC1114 have cheap support.  If CTS and DTR can be used, then anyone who has the equipment to program an Arduino Pro Mini or Lilypad can program the LPC1114.  Though, I'm not kidding myself, this code would be a stagnant mess if it weren't for Bdk6's guidance.

This is [an example](http://example.com/ "Title") inline link.

![alt tag](https://cdn.sparkfun.com//assets/parts/3/9/5/8/09873-02d.jpg)
