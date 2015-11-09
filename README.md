Fhourstones 8x8
===============

This version of Fhourstones supports bitboards beyond 64-bits,
as needed for the 8x8 size popular on littlegolem.net and itsyourturn.com.

It was used to solve the 8x8 game in the late 2014 and early 2015,
with the file book88 retaining all solved positions of at most 16 plies.

After running make, the program C488 repeatedly accepts lines containing digits 1-8,
and will solve the resulting position. An example session:

> ./C488
444445555
Solving . . .    
score +  work 33  1 pos / 1 msec = 1 Kpos/sec
1111111188888888222222777777
Solving . . .    
score +  work 15  513960 pos / 153 msec = 3359 Kpos/sec
^D
Be seeing you...
