# GNU Makefile

CC = g++
CFLAGS = -std=c++11 -O3 -Wextra -Wall
CFILES = C4.cpp Search.cpp Window.cpp

C488 : 	$(CFILES)
	  $(CC) $(CFLAGS) -DWIDTH=8 -DHEIGHT=8 -DLOCKSIZE=50 -DTRANSIZE=8306069 $(CFILES) -o $@
