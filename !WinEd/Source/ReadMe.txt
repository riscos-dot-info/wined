Building WinEd
==============

As of June 2020, this version of WinEd is known to build using the GCCSDK environment, using the following information. In theory it should still build using GCC on RISC OS, but although no changes have knowingly been made to the source to prevent this, it has not tested.


DeskLib
-------

You will need a copy of the SCL version of DeskLib. The easiest way to achieve this in the GCCSDK is to get the Autobuilder to download and build a copy using the following commands at the prompt (but see note below).

  cd ~/gccsdk/build
  ../autobuilder/build -v desklib-scl

Alternatively, it is possible to check the sources out from the SVN repository at riscos.info and biild direct. From the folder you wish to download DeskLib to:

  svn co svn://svn.riscos.info/DeskLib/trunk DeskLib
  cd DeskLib/\!DLSources
  make -f Makefile.unix install SCL=true

NB. At the time of writing, WinEd requires a modification to DeskLib which is not in trunk. Until this has been merged, it will therefore be necessary to check out a branch and build manually from that:

  svn co svn://svn.riscos.info/DeskLib/branches/wined DeskLib
  cd DeskLib/\!DLSources
  make -f Makefile.unix install SCL=true


FlexLib
-------

You will need a copy of FlexLib, which comes with the ROOL DDE. This will need to be compiled to an ELF library suitable for use with the GCCSDK, using the compiler options

  -mlibscl -mhard-float -static

and installed into the GCCSDK.


Fortify
-------

If you wish to compile with Fortify, you will need to add the fortify.c, fortify.h and ufortify.h files into !Wined.Sources. Otherwise, references to fortify will need to be removed from the Makefile (both the static dependency, from within the OBJECTS list at the top).

The definitions for FORTIFY_LOCK() and FORTIFY_UNLOCK() in ufortify.h need to be changed to empty, as follows:

  #define FORTIFY_LOCK()
  #define FORTIFY_UNLOCK()

To use Fortify, add -DFORTIFY to the COMPILEFLAGS.


Path Variables
--------------

To allow GCCSDK to find the RISC OS paths used in the source code, it is necessary to set two environment variables:

  export DESKLIB_PATH=$GCCSDK_INSTALL_ENV/include/DeskLib
  export FLEXLIB_PATH=$GCCSDK_INSTALL_ENV/include

These assume that GCCSDK_INSTALL_ENV has been set; Makefile.unix also assumes this, however!


Making the code
---------------

The code can be built using by navigating to the !WinEd/Source folder and calling Make:

  make -f Makefile.unix

Good luck!

-- 
Steve Fryatt, June 2020.
info@stevefryatt.org.uk