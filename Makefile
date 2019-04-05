HANDINDIR = /w/classproj/cs33

CC = gcc
CFLAGS = -Wall -O2 -m32 -g

OBJS = mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o

mdriver: $(OBJS)
	$(CC) $(CFLAGS) -o mdriver $(OBJS)

mm.o: mm.c mm.h memlib.h

handin:
	@USER=whoami
	cp -r * $(HANDINDIR)/$(USER)/
	@perl -I /u/cs/class/cs33/cs33t3/malloclab/grade/ /u/cs/class/cs33/cs33t3/malloclab/grade/grade-malloclab.pl
clean:
	rm -f *~ mdriver


