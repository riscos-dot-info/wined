/* Error.c */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DeskLib:Hourglass.h"
#include "DeskLib:MsgTrans.h"
#include "DeskLib:Str.h"
#include "DeskLib:WimpSWIs.h"

static os_error error_error;

char error_title[40] = "WinEd";

void Error_Report(int errnum,char *errmess,...)
{
  va_list argptr;

  error_error.errnum = errnum;

  Str_MakeASCIIZ(errmess,252);

  va_start(argptr,errmess);
  vsprintf(error_error.errmess,errmess,argptr);
  va_end(argptr);

  Hourglass_Smash();
  Wimp_ReportError(&error_error,1,error_title);
}

void Error_ReportFatal(int errnum,char *errmess,...)
{
  va_list argptr;

  error_error.errnum = errnum;

  va_start(argptr,errmess);
  vsprintf(error_error.errmess,errmess,argptr);
  va_end(argptr);

  Hourglass_Smash();
  Wimp_ReportError(&error_error,5,error_title);
  exit(errnum);
}

void Error_ReportFatalInternal(int errnum,char *errmess,...)
{
  va_list argptr;

  error_error.errnum = errnum;

  va_start(argptr,errmess);
  vsprintf(error_error.errmess,errmess,argptr);
  va_end(argptr);

  Hourglass_Smash();
  Wimp_ReportError(&error_error,5,error_title);
  exit(errnum);
}

BOOL Error_Check(os_error *errblk)
{
  if (!errblk)
    return FALSE;
  Hourglass_Smash();
  Wimp_ReportError(errblk,1,error_title);
  return TRUE;
}

void Error_CheckFatal(os_error *errblk)
{
  if (!errblk)
    return;

  Hourglass_Smash();
  Wimp_ReportError(errblk,5,error_title);
  exit(errblk->errnum);
}

BOOL Error_OutOfMemory(char *place,BOOL fatal)
{
  MsgTrans_Lookup(0,"NoStore:Not enough memory",error_error.errmess,128);
  sprintf(error_error.errmess + strlen(error_error.errmess)," (%s)",place);
  error_error.errnum = 0;
  if (fatal)
    Error_CheckFatal(&error_error);
  else
    Error_Check(&error_error);

  return FALSE;
}

void Error_ReportInternal(int errnum,char *errmess,...)
{
  va_list argptr;

  error_error.errnum = errnum;

  va_start(argptr,errmess);
  vsprintf(error_error.errmess,errmess,argptr);
  va_end(argptr);

  Hourglass_Smash();
  Wimp_ReportError(&error_error,1,error_title);
}
