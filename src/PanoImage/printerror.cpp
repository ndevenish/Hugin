#include <stdio.h>
#include <stdarg.h>
#include "wx/wx.h"

void PrintError (char *fmt, ...)
{
   va_list ap;
   char message[257];

   va_start(ap, fmt);
   vsprintf(message, fmt, ap);
   va_end(ap);

   wxMessageBox(message);
}
