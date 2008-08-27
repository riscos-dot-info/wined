# Makefile for WinEd


objects = Error.o alndiag.o bordiag.o browser.o browtools.o choices.o coorddiag.o datatrans.o deskfont.o \
          diagutils.o drag.o dragext.o export.o fontpick.o fortify.o globals.o help.o icndiag.o icnedit.o minsize.o monitor.o \
          multiicon.o pane.o picker.o round.o rszdiag.o shortcuts.o spcdiag.o stats.o tempfont.o templates.o \
          terminator.o unsaved.o uri.o usersprt.o viewer.o viewtools.o visarea.o wadiag.o windiag.o wined.o

libraries = flexlib:flexlib-32.o DeskLib:DeskLib.o

compileflags = -DFORTIFY -DDeskLib_DEBUG -std=c99 -mlibscl -Wunused -Wall -fno-strict-aliasing -mthrowback -mpoke-function-name -c
#compileflags = -O3 -std=c99 -mlibscl -Wunused -Wuninitialized -Wall -fno-strict-aliasing -mthrowback -mpoke-function-name -c

linkflags = -mlibscl -o ^.!RunImage

# Default rule
^.!RunImage : $(objects) 
	gcc $(linkflags) $(objects) $(libraries)


# Static dependencies:
dragext.o : DragExt.s
	gcc $(compileflags) DragExt.s
Error.o : Error.c
	gcc $(compileflags) Error.c
alndiag.o : alndiag.c
	gcc $(compileflags) alndiag.c
bordiag.o : bordiag.c
	gcc $(compileflags) bordiag.c
browser.o : browser.c
	gcc $(compileflags) browser.c
browtools.o : browtools.c
	gcc $(compileflags) browtools.c
choices.o : choices.c
	gcc $(compileflags) choices.c
coorddiag.o : coorddiag.c
	gcc $(compileflags) coorddiag.c
datatrans.o : datatrans.c
	gcc $(compileflags) datatrans.c
deskfont.o : deskfont.c
	gcc $(compileflags) deskfont.c
diagutils.o : diagutils.c
	gcc $(compileflags) diagutils.c
drag.o : drag.c
	gcc $(compileflags) drag.c
export.o : export.c
	gcc $(compileflags) export.c
fontpick.o : fontpick.c
	gcc $(compileflags) fontpick.c
fortify.o : fortify.c
	gcc $(compileflags) fortify.c
globals.o : globals.c
	gcc $(compileflags) globals.c
help.o : help.c
	gcc $(compileflags) help.c
icndiag.o : icndiag.c
	gcc $(compileflags) icndiag.c
icnedit.o : icnedit.c
	gcc $(compileflags) icnedit.c
minsize.o : minsize.c
	gcc $(compileflags) minsize.c
monitor.o : monitor.c
	gcc $(compileflags) monitor.c
multiicon.o : multiicon.c
	gcc $(compileflags) multiicon.c
pane.o : pane.c
	gcc $(compileflags) pane.c
picker.o : picker.c
	gcc $(compileflags) picker.c
round.o : round.c
	gcc $(compileflags) round.c
rszdiag.o : rszdiag.c
	gcc $(compileflags) rszdiag.c
spcdiag.o : spcdiag.c
	gcc $(compileflags) spcdiag.c
shortcuts.o : shortcuts.c
	gcc $(compileflags) shortcuts.c
stats.o : stats.c
	gcc $(compileflags) stats.c
tempfont.o : tempfont.c
	gcc $(compileflags) tempfont.c
templates.o : templates.c
	gcc $(compileflags) templates.c
terminator.o : terminator.c
	gcc $(compileflags) terminator.c
unsaved.o : unsaved.c
	gcc $(compileflags) unsaved.c
uri.o : uri.c
	gcc $(compileflags) uri.c
usersprt.o : usersprt.c
	gcc $(compileflags) usersprt.c
viewer.o : viewer.c
	gcc $(compileflags) viewer.c
viewtools.o : viewtools.c
	gcc $(compileflags) viewtools.c
visarea.o : visarea.c
	gcc $(compileflags) visarea.c
wadiag.o : wadiag.c
	gcc $(compileflags) wadiag.c
windiag.o : windiag.c
	gcc $(compileflags) windiag.c
wined.o : wined.c
	gcc $(compileflags) wined.c

.PHONY : clean
clean:
	IfThere ^.!RunImage Then Wipe ^.!RunImage ~CR
	IfThere o Then Wipe o ~CR