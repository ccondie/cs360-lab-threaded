# Makefile for echo client and server

CXX=			g++ $(CCFLAGS)

MSGD=		msgd-run.o msgd.o user.o message.o
MSG=		msg-run.o msg.o
OBJS =			$(MSGD) $(MSG)

LIBS=

CCFLAGS= -g

all:	msgd-run msg-run

msgd-run:$(MSGD)
	$(CXX) -o msgd $(MSGD) $(LIBS)

msg-run:$(MSG)
	$(CXX) -o msg $(MSG) $(LIBS)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d)

realclean:
	rm -f $(OBJS) $(OBJS:.o=.d) msgd msg user message


# These lines ensure that dependencies are handled automatically.
%.d:	%.cc
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

include	$(OBJS:.o=.d)
