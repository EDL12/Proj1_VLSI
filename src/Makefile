#####################################################################
# Makefile for the BLIF file parser for different machines
# 
#
#
#####################################################################



#LEX			= lex 			
YACC			= bison -y 

#YACCDEBUG      	= -DYYDEBUG=1
#DEBUG_NETLIST		= -DNETLIST_DUMP
#DEBUG_BUILD_CKT	= -DDEBUG_BUILD_CKT
#DEBUG_MESSAGES		= -DDEBUG_MESSAGES
#DEBUG_MODE		= -DSTAND_ALONE
DEBUG_FLAGS		= $(DEBUG_NETLIST) $(YACCDEBUG) $(DEBUG_BUILD_CKT) $(DEBUG_MESSAGES) $(DEBUG_MODE)

CFLAGS			= -O $(INCLUDE) -DSIS
LIBS			= -lm #-ll

CC			= gcc
TARGET			= 3fsim

BUILDCKT_SRC		= build_ckt.c
YACC_SRC		= blif.y
LEX_SRC			= blif.l
YACC_HEADER		= y.tab.h
YACC_CPROG		= y.tab.c 
LEX_CPROG		= lex.yy.c

PSRC			= main.c project.c \
			  build_ckt.c $(LEX_CPROG) $(YACC_CPROG)
POBJ			= main.o project.o \
			  build_ckt.o lex.yy.o y.tab.o 
PHDR			= project.h y.tab.h read_ckt.h


#----------------------------------------------------------------------

all:		$(TARGET)

main.o: main.c project.h
build_ckt.o: build_ckt.c  project.h
project.o: project.c project.h

$(TARGET):	$(POBJ)
		$(CC) -o $(TARGET) $(POBJ) $(LIBS)

.c.o:		
		$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

lex.yy.o:	$(LEX_CPROG) $(YACC_HEADER)

y.tab.o:	$(YACC_CPROG) $(YACC_HEADER)

$(YACC_CPROG) $(YACC_HEADER): $(YACC_SRC) read_ckt.h
		$(YACC) -d  $(YACC_SRC)

#$(LEX_CPROG):	$(LEX_SRC) $(YACC_HEADER)
#		$(LEX)  $(LEX_SRC)

clean:
		rm -rf $(TARGET) $(POBJ) $(YACC_CPROG) $(YACC_HEADER) out.log




