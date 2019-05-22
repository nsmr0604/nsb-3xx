#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

#include "common.h"

static const char *toolname, *invokename;

void pka_tool_init(const char *name, int argc, char **argv)
{
   toolname = invokename = name;
   if (argc > 0)
      invokename = argv[0];
}

const char *pka_tool_name(void)
{
   assert(invokename);
   return invokename;
}

void pka_tool_version(void)
{
   assert(toolname);

   printf("%s (%s) %s\n", toolname, PACKAGE_NAME, PACKAGE_VERSION);
   puts("Copyright (C) 2013 Elliptic Technologies Inc.");
}

static int pka_tool_vmsg_(FILE *f, int err, const char *fmt, va_list ap)
{
   size_t tool_len, fmt_len = 0, err_len = 0;
   char *newfmt, *errmsg = NULL;
   int ret;

   assert(invokename);
   tool_len = strlen(invokename);

   if (fmt) {
      fmt_len = strlen(": ") + strlen(fmt);
   }

   if (err >= 0) {
      errmsg = strerror(err);
      err_len = strlen(": ") + strlen(errmsg);
   }

   newfmt = malloc(tool_len + fmt_len + err_len + sizeof "\n");
   if (!newfmt)
      return -1;

   if (errmsg && fmt)
      sprintf(newfmt, "%s: %s: %s\n", invokename, fmt, errmsg);
   else if (errmsg)
      sprintf(newfmt, "%s: %s\n", invokename, errmsg);
   else if (fmt)
      sprintf(newfmt, "%s: %s\n", invokename, fmt);
   else
      sprintf(newfmt, "%s\n", invokename);

   ret = vfprintf(f, newfmt, ap);

   free(newfmt);

   return ret;
}

int pka_tool_vmsg(const char *fmt, va_list ap)
{
   return pka_tool_vmsg_(stdout, -1, fmt, ap);
}

int pka_tool_msg(const char *fmt, ...)
{
   va_list ap;
   int ret;

   va_start(ap, fmt);
   ret = pka_tool_vmsg(fmt, ap);
   va_end(ap);

   return ret;
}

int pka_tool_verr(int err, const char *fmt, va_list ap)
{
   if (err == 0)
      err = errno;

   return pka_tool_vmsg_(stderr, err, fmt, ap);
}

int pka_tool_err(int err, const char *fmt, ...)
{
   va_list ap;
   int ret;

   va_start(ap, fmt);
   ret = pka_tool_verr(err, fmt, ap);
   va_end(ap);

   return ret;
}
