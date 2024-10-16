#
# $Header: /user1/terry/s/trie/RCS/Makefile,v 1.2 90/04/18 17:24:10 terry Exp $
#

#
# $Log:	Makefile,v $
# Revision 1.2  90/04/18  17:24:10  terry
# check in so jordan can take it home.
# 
# Revision 1.1  90/03/26  22:29:48  terry
# Initial revision
# 
#


ifeq "$(strip $(shell uname))" "RCU"
  CFLAGS = -Wall
else
  CFLAGS = -Wall -g -pipe
endif


#
# If you are not testing trie.c using
# try.c then remove the definition of TRIE_TESTING
#
TRIE_TESTING = 1

CC = gcc
AS = gas

OBJS = trie.o new.o error.o destroy.o 

ifdef TRIE_TESTING
DEFINES = -DTRIE_ARG_CHECKING -DTRIE_TESTING -DSYSV -DPCS
else
DEFINES = -DSYSV -DPCS
endif

COMMON_HDR = common.h

ifdef TRIE_TESTING
all : try
else
all : $(OBJS)
endif

ifdef TRIE_TESTING
trie.o : trie.h trie.c trie_aux.c destroy.h new.h $(COMMON_HDR) 
	$(CC) $(CFLAGS) $(DEFINES) -c trie.c
else
trie.o : trie.h trie.c destroy.h new.h $(COMMON_HDR)
	$(CC) $(CFLAGS) $(DEFINES) -c trie.c
endif

try : try.o $(OBJS) $(COMMON_HDR)
	$(CC) $(CFLAGS) $(DEFINES) -o try try.o $(OBJS)

try.o : trie.h try.c new.o destroy.o $(COMMON_HDR)
	$(CC) $(CFLAGS) $(DEFINES) -c try.c

new.o : trie.h new.h
	$(CC) $(CFLAGS) $(DEFINES) -c new.c

destroy.o : trie.h destroy.h
	$(CC) $(CFLAGS) $(DEFINES) -c destroy.c

error.o : 
	$(CC) $(CFLAGS) $(DEFINES) -c error.c

clean:
	rm *.o

clobber: clean
	rm try
