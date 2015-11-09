# GNU Makefile

CC = g++
CFLAGS = -std=c++11 -O3 -Wextra -Wall
CFILES = C4.cpp Search.cpp Window.cpp

C488 : 	Makefile $(CFILES)
	  $(CC) $(CFLAGS) -DWIDTH=8 -DHEIGHT=8 -DBOOKWORK=24 -DLOCKSIZE=50 -DTRANSIZE=8306069 $(CFILES) -o $@
