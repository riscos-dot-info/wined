WinEd
=====

A Window Template Editor with Advanced Features


Introduction
------------

WinEd is a RISC OS template editor, whose founding aim was "to do everything
which TemplEd could do, only better!". It was originally written by Tony Houghton,
who developed it until version 2.87 in 2003. Details of this version can be
found on the [archive of Tony's site](http://www.snowstone.org.uk/riscos/realh/index.html#wined).
Development was then picked up by Adam Richardson, who continued to maintain
and improve WinEd until the release of version 3.22a in 2009. Details of Adam's
versions can be found on [his website](http://www.snowstone.org.uk/riscos/);
the major releases were provided via PackMan, and also
[made available to download](http://www.snowstone.org.uk/riscos/wined/).

At the time of writing, the most recent version of WinEd can be found at
https://www.stevefryatt.org.uk/risc-os/wined

Building
--------

As of June 2020, this version of WinEd is known to build using the GCCSDK
environment, using the following information. In theory it should still build
using GCC on RISC OS but, although no changes have knowingly been made to the
source to prevent this, it has not been tested.

### DeskLib

You will need a copy of the SCL version of DeskLib. The easiest way to obtain
this in the GCCSDK is to get the Autobuilder to download and build a copy
using the following commands at the prompt.

	cd ~/gccsdk/build
	../autobuilder/build -v desklib-scl

Alternatively, it is possible to check the sources out from the SVN repository
at [riscos.info](https://www.riscos.info) and build direct. From the folder you
wish to download DeskLib to:

	svn co svn://svn.riscos.info/DeskLib/trunk DeskLib
	cd DeskLib/\!DLSources
	make -f Makefile.unix install SCL=true


### FlexLib

You will need a copy of FlexLib, which comes with the ROOL DDE. This will need to
be compiled to an ELF library suitable for use with the GCCSDK, using the
compiler options

	-mlibscl -mhard-float -static

and installed into the GCCSDK.


### Fortify

If you wish to compile with Fortify, you will need to add the `fortify.c`, `fortify.h`
and `ufortify.h` files into `!Wined.Sources`. Otherwise, references to fortify will
need to be removed from the Makefile (both the static dependency, and from within the
`OBJECTS` list at the top).

The definitions for `FORTIFY_LOCK()` and `FORTIFY_UNLOCK()` in `ufortify.h` need to be
changed to empty, as follows:

	#define FORTIFY_LOCK()
	#define FORTIFY_UNLOCK()

To use Fortify, add -DFORTIFY to the COMPILEFLAGS.


### Path Variables

To allow GCCSDK to find the RISC OS paths used in the source code, it is necessary
to set two environment variables:

	export DESKLIB_PATH=$GCCSDK_INSTALL_ENV/include/DeskLib
	export FLEXLIB_PATH=$GCCSDK_INSTALL_ENV/include

These assume that `GCCSDK_INSTALL_ENV` has been set; `Makefile.unix` also assumes this,
however!


### Making the code

The code can be built using by navigating to the `!WinEd/Source` folder and calling Make:

	make -f Makefile.unix

This will build everything from source, and place a complete `!RunImage` within the
`!WinEd` folder. If you have access to this folder from RISC OS (either via HostFS,
LanManFS, NFS, Sunfish or similar), it will be possible to run it directly once built.

To clean out all of the build files, use

	make -f Makefile.unix clean

To make a release version and package it into Zip files for distribution, use

	make -f Makefile.unix release

This will clean the project and re-build it all, then create a distribution archive
(no source) and source archive in the folder within which the project folder is
located. By default the output of `git describe` is used to version the build, but a
specific version can be applied by setting the `VERSION` variable -- for example

	make -f Makefile.unix release VERSION=3.21

Good luck!

-- 
Steve Fryatt, June 2020 and March 2024.
info@stevefryatt.org.uk