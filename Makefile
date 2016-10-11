DEFINES=PROJECT_CONF_H=\"project-conf.h\"

ifndef TARGET
TARGET = z1
endif

CONTIKI = ../..

CONTIKI_PROJECT = nes-channel-selection-coordinator

all: $(CONTIKI_PROJECT)

%.class: %.java
	javac $(basename $<).java
	
viewrssi3d: ViewRSSI3D.class
	make login | java ViewRSSI3D

viewrssi: ViewRSSI.class
	make login | java ViewRSSI

	
CONTIKI_WITH_RIME = 1
include $(CONTIKI)/Makefile.include

