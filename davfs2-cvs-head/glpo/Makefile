# Makefile for PO directory in any package using GNU gettext.
# Copyright (C) 1995-1997, 2000-2007, 2009-2010 by Ulrich Drepper <drepper@gnu.ai.mit.edu>
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License but which still want to provide support for the GNU gettext
# functionality.
# Please note that the actual code of GNU gettext is covered by the GNU
# General Public License and is *not* in the public domain.
#
# Origin: gettext-0.18
GETTEXT_MACRO_VERSION = 0.18

PACKAGE = davfs2
VERSION = 1.4.6
PACKAGE_BUGREPORT = http://savannah.nongnu.org/projects/davfs2

SHELL = /bin/sh


srcdir = .
top_srcdir = ..


prefix = /usr
exec_prefix = /usr
datarootdir = ${prefix}/share
datadir = /usr/share
localedir = ${datarootdir}/locale
gettextsrcdir = $(datadir)/gettext/po

INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644

# We use $(mkdir_p).
# In automake <= 1.9.x, $(mkdir_p) is defined either as "mkdir -p --" or as
# "$(mkinstalldirs)" or as "$(install_sh) -d". For these automake versions,
# ${SHELL} /home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/davfs2-cvs-head/config/install-sh does not start with $(SHELL), so we add it.
# In automake >= 1.10, /bin/mkdir -p is derived from ${MKDIR_P}, which is defined
# either as "/path/to/mkdir -p" or ".../install-sh -c -d". For these automake
# versions, $(mkinstalldirs) and $(install_sh) are unused.
mkinstalldirs = $(SHELL) ${SHELL} /home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/davfs2-cvs-head/config/install-sh -d
install_sh = $(SHELL) ${SHELL} /home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/davfs2-cvs-head/config/install-sh
MKDIR_P = /bin/mkdir -p
mkdir_p = /bin/mkdir -p

GMSGFMT_ = /usr/bin/msgfmt
GMSGFMT_no = /usr/bin/msgfmt
GMSGFMT_yes = /usr/bin/msgfmt
GMSGFMT = $(GMSGFMT_$(USE_MSGCTXT))
MSGFMT_ = /usr/bin/msgfmt
MSGFMT_no = /usr/bin/msgfmt
MSGFMT_yes = /usr/bin/msgfmt
MSGFMT = $(MSGFMT_$(USE_MSGCTXT))
XGETTEXT_ = /usr/bin/xgettext
XGETTEXT_no = /usr/bin/xgettext
XGETTEXT_yes = /usr/bin/xgettext
XGETTEXT = $(XGETTEXT_$(USE_MSGCTXT))
MSGMERGE = msgmerge
MSGMERGE_UPDATE = /usr/bin/msgmerge --update
MSGINIT = msginit
MSGCONV = msgconv
MSGFILTER = msgfilter

POFILES =  af.po be.po bg.po ca.po cs.po da.po de.po el.po eo.po es.po et.po eu.po fi.po fr.po ga.po gl.po hu.po it.po ja.po ko.po ms.po nb.po nl.po pl.po pt.po pt_BR.po ro.po ru.po rw.po sk.po sl.po sr.po sv.po tr.po uk.po vi.po zh_CN.po zh_TW.po
GMOFILES =  af.gmo be.gmo bg.gmo ca.gmo cs.gmo da.gmo de.gmo el.gmo eo.gmo es.gmo et.gmo eu.gmo fi.gmo fr.gmo ga.gmo gl.gmo hu.gmo it.gmo ja.gmo ko.gmo ms.gmo nb.gmo nl.gmo pl.gmo pt.gmo pt_BR.gmo ro.gmo ru.gmo rw.gmo sk.gmo sl.gmo sr.gmo sv.gmo tr.gmo uk.gmo vi.gmo zh_CN.gmo zh_TW.gmo
UPDATEPOFILES =  af.po-update be.po-update bg.po-update ca.po-update cs.po-update da.po-update de.po-update el.po-update eo.po-update es.po-update et.po-update eu.po-update fi.po-update fr.po-update ga.po-update gl.po-update hu.po-update it.po-update ja.po-update ko.po-update ms.po-update nb.po-update nl.po-update pl.po-update pt.po-update pt_BR.po-update ro.po-update ru.po-update rw.po-update sk.po-update sl.po-update sr.po-update sv.po-update tr.po-update uk.po-update vi.po-update zh_CN.po-update zh_TW.po-update
DUMMYPOFILES =  af.nop be.nop bg.nop ca.nop cs.nop da.nop de.nop el.nop eo.nop es.nop et.nop eu.nop fi.nop fr.nop ga.nop gl.nop hu.nop it.nop ja.nop ko.nop ms.nop nb.nop nl.nop pl.nop pt.nop pt_BR.nop ro.nop ru.nop rw.nop sk.nop sl.nop sr.nop sv.nop tr.nop uk.nop vi.nop zh_CN.nop zh_TW.nop
DISTFILES.common = Makefile.in.in remove-potcdate.sin \
$(DISTFILES.common.extra1) $(DISTFILES.common.extra2) $(DISTFILES.common.extra3)
DISTFILES = $(DISTFILES.common) Makevars POTFILES.in \
$(POFILES) $(GMOFILES) \
$(DISTFILES.extra1) $(DISTFILES.extra2) $(DISTFILES.extra3)

POTFILES = \
     ../gl/alloca.in.h \
     ../gl/areadlink-with-size.c \
     ../gl/areadlink.h \
     ../gl/asnprintf.c \
     ../gl/asprintf.c \
     ../gl/at-func.c \
     ../gl/basename-lgpl.c \
     ../gl/basename.c \
     ../gl/bitrotate.h \
     ../gl/btowc.c \
     ../gl/c-ctype.c \
     ../gl/c-ctype.h \
     ../gl/c-strcase.h \
     ../gl/c-strcasecmp.c \
     ../gl/c-strncasecmp.c \
     ../gl/canonicalize.c \
     ../gl/canonicalize.h \
     ../gl/chdir-long.c \
     ../gl/chdir-long.h \
     ../gl/cloexec.c \
     ../gl/cloexec.h \
     ../gl/close.c \
     ../gl/closedir.c \
     ../gl/config.charset \
     ../gl/dirent-private.h \
     ../gl/dirent.in.h \
     ../gl/dirfd.c \
     ../gl/dirname-lgpl.c \
     ../gl/dirname.c \
     ../gl/dirname.h \
     ../gl/dosname.h \
     ../gl/dup-safer.c \
     ../gl/dup.c \
     ../gl/dup2.c \
     ../gl/errno.in.h \
     ../gl/error.c \
     ../gl/error.h \
     ../gl/exitfail.c \
     ../gl/exitfail.h \
     ../gl/fchdir.c \
     ../gl/fcntl.c \
     ../gl/fcntl.in.h \
     ../gl/fd-hook.c \
     ../gl/fd-hook.h \
     ../gl/fd-safer.c \
     ../gl/fdopendir.c \
     ../gl/file-set.c \
     ../gl/file-set.h \
     ../gl/filename.h \
     ../gl/filenamecat-lgpl.c \
     ../gl/filenamecat.h \
     ../gl/float+.h \
     ../gl/float.c \
     ../gl/float.in.h \
     ../gl/fstat.c \
     ../gl/fstatat.c \
     ../gl/getcwd-lgpl.c \
     ../gl/getcwd.c \
     ../gl/getdtablesize.c \
     ../gl/gettext.h \
     ../gl/hash-pjw.c \
     ../gl/hash-pjw.h \
     ../gl/hash-triple.c \
     ../gl/hash-triple.h \
     ../gl/hash.c \
     ../gl/hash.h \
     ../gl/iconv.c \
     ../gl/iconv.in.h \
     ../gl/iconv_close.c \
     ../gl/iconv_open-aix.gperf \
     ../gl/iconv_open-hpux.gperf \
     ../gl/iconv_open-irix.gperf \
     ../gl/iconv_open-osf.gperf \
     ../gl/iconv_open-solaris.gperf \
     ../gl/iconv_open.c \
     ../gl/intprops.h \
     ../gl/itold.c \
     ../gl/langinfo.in.h \
     ../gl/localcharset.c \
     ../gl/localcharset.h \
     ../gl/lstat.c \
     ../gl/malloc.c \
     ../gl/mbrtowc.c \
     ../gl/mbsinit.c \
     ../gl/mbtowc-impl.h \
     ../gl/mbtowc.c \
     ../gl/memchr.c \
     ../gl/memchr.valgrind \
     ../gl/mempcpy.c \
     ../gl/memrchr.c \
     ../gl/msvc-inval.c \
     ../gl/msvc-inval.h \
     ../gl/msvc-nothrow.c \
     ../gl/msvc-nothrow.h \
     ../gl/nl_langinfo.c \
     ../gl/open.c \
     ../gl/openat-die.c \
     ../gl/openat-priv.h \
     ../gl/openat-proc.c \
     ../gl/openat.c \
     ../gl/openat.h \
     ../gl/opendir.c \
     ../gl/pathmax.h \
     ../gl/pipe-safer.c \
     ../gl/printf-args.c \
     ../gl/printf-args.h \
     ../gl/printf-parse.c \
     ../gl/printf-parse.h \
     ../gl/readdir.c \
     ../gl/readlink.c \
     ../gl/realloc.c \
     ../gl/ref-add.sin \
     ../gl/ref-del.sin \
     ../gl/regcomp.c \
     ../gl/regex.c \
     ../gl/regex.h \
     ../gl/regex_internal.c \
     ../gl/regex_internal.h \
     ../gl/regexec.c \
     ../gl/rewinddir.c \
     ../gl/rpmatch.c \
     ../gl/same-inode.h \
     ../gl/same.c \
     ../gl/same.h \
     ../gl/save-cwd.c \
     ../gl/save-cwd.h \
     ../gl/size_max.h \
     ../gl/stat.c \
     ../gl/stdarg.in.h \
     ../gl/stdbool.in.h \
     ../gl/stddef.in.h \
     ../gl/stdint.in.h \
     ../gl/stdio.in.h \
     ../gl/stdlib.in.h \
     ../gl/strcasecmp.c \
     ../gl/strdup.c \
     ../gl/streq.h \
     ../gl/strerror-override.c \
     ../gl/strerror-override.h \
     ../gl/strerror.c \
     ../gl/string.in.h \
     ../gl/strings.in.h \
     ../gl/stripslash.c \
     ../gl/strncasecmp.c \
     ../gl/strndup.c \
     ../gl/strnlen.c \
     ../gl/sys_stat.in.h \
     ../gl/sys_types.in.h \
     ../gl/time.in.h \
     ../gl/unistd--.h \
     ../gl/unistd-safer.h \
     ../gl/unistd.in.h \
     ../gl/unistr.in.h \
     ../gl/unistr/u8-mbtoucr.c \
     ../gl/unistr/u8-uctomb-aux.c \
     ../gl/unistr/u8-uctomb.c \
     ../gl/unitypes.in.h \
     ../gl/vasnprintf.c \
     ../gl/vasnprintf.h \
     ../gl/vasprintf.c \
     ../gl/verify.h \
     ../gl/wchar.in.h \
     ../gl/wcrtomb.c \
     ../gl/wctype.in.h \
     ../gl/xalloc-die.c \
     ../gl/xalloc-oversized.h \
     ../gl/xalloc.h \
     ../gl/xasprintf.c \
     ../gl/xgetcwd.c \
     ../gl/xgetcwd.h \
     ../gl/xmalloc.c \
     ../gl/xsize.h \
     ../gl/xstrndup.c \
     ../gl/xstrndup.h \
     ../gl/xvasprintf.c \
     ../gl/xvasprintf.h

CATALOGS =  af.gmo be.gmo bg.gmo ca.gmo cs.gmo da.gmo de.gmo el.gmo eo.gmo es.gmo et.gmo eu.gmo fi.gmo fr.gmo ga.gmo gl.gmo hu.gmo it.gmo ja.gmo ko.gmo ms.gmo nb.gmo nl.gmo pl.gmo pt.gmo pt_BR.gmo ro.gmo ru.gmo rw.gmo sk.gmo sl.gmo sr.gmo sv.gmo tr.gmo uk.gmo vi.gmo zh_CN.gmo zh_TW.gmo

# Makevars gets inserted here. (Don't remove this line!)
## DO NOT EDIT! GENERATED AUTOMATICALLY!
# Copyright (C) 2002-2012 Free Software Foundation, Inc.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This file is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this file.  If not, see <http://www.gnu.org/licenses/>.
#
# As a special exception to the GNU General Public License,
# this file may be distributed as part of a program that
# contains a configuration script generated by Autoconf, under
# the same distribution terms as the rest of that program.
#
# Generated by gnulib-tool.

# Usually the message domain is the same as the package name.
# But here it has a '-gnulib' suffix.
DOMAIN = davfs2-gnulib

# These two variables depend on the location of this directory.
subdir = glpo
top_builddir = ..

# These options get passed to xgettext.
XGETTEXT_OPTIONS = \
  --keyword=_ --flag=_:1:pass-c-format \
  --keyword=N_ --flag=N_:1:pass-c-format \
  --keyword='proper_name:1,"This is a proper name. See the gettext manual, section Names."' \
  --keyword='proper_name_utf8:1,"This is a proper name. See the gettext manual, section Names."' \
  --flag=error:3:c-format --flag=error_at_line:5:c-format

# This is the copyright holder that gets inserted into the header of the
# $(DOMAIN).pot file.  gnulib is copyrighted by the FSF.
COPYRIGHT_HOLDER = Free Software Foundation, Inc.

# This is the email address or URL to which the translators shall report
# bugs in the untranslated strings:
# - Strings which are not entire sentences, see the maintainer guidelines
#   in the GNU gettext documentation, section 'Preparing Strings'.
# - Strings which use unclear terms or require additional context to be
#   understood.
# - Strings which make invalid assumptions about notation of date, time or
#   money.
# - Pluralisation problems.
# - Incorrect English spelling.
# - Incorrect formatting.
# It can be your email address, or a mailing list address where translators
# can write to without being subscribed, or the URL of a web page through
# which the translators can contact you.
MSGID_BUGS_ADDRESS = bug-gnulib@gnu.org

# This is the list of locale categories, beyond LC_MESSAGES, for which the
# message catalogs shall be used.  It is usually empty.
EXTRA_LOCALE_CATEGORIES =

# This tells whether the $(DOMAIN).pot file contains messages with an 'msgctxt'
# context.  Possible values are "yes" and "no".  Set this to yes if the
# package uses functions taking also a message context, like pgettext(), or
# if in $(XGETTEXT_OPTIONS) you define keywords with a context argument.
USE_MSGCTXT = no

.SUFFIXES:
.SUFFIXES: .po .gmo .mo .sed .sin .nop .po-create .po-update

.po.mo:
	@echo "$(MSGFMT) -c -o $@ $<"; \
	$(MSGFMT) -c -o t-$@ $< && mv t-$@ $@

.po.gmo:
	@lang=`echo $* | sed -e 's,.*/,,'`; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}rm -f $${lang}.gmo && $(GMSGFMT) -c --statistics --verbose -o $${lang}.gmo $${lang}.po"; \
	cd $(srcdir) && rm -f $${lang}.gmo && $(GMSGFMT) -c --statistics --verbose -o t-$${lang}.gmo $${lang}.po && mv t-$${lang}.gmo $${lang}.gmo

.sin.sed:
	sed -e '/^#/d' $< > t-$@
	mv t-$@ $@


all: all-no

all-yes: stamp-po
all-no:

# Ensure that the gettext macros and this Makefile.in.in are in sync.
CHECK_MACRO_VERSION = \
	test "$(GETTEXT_MACRO_VERSION)" = "0.18" \
	  || { echo "*** error: gettext infrastructure mismatch: using a Makefile.in.in from gettext version $(GETTEXT_MACRO_VERSION) but the autoconf macros are from gettext version 0.18" 1>&2; \
	       exit 1; \
	     }

# $(srcdir)/$(DOMAIN).pot is only created when needed. When xgettext finds no
# internationalized messages, no $(srcdir)/$(DOMAIN).pot is created (because
# we don't want to bother translators with empty POT files). We assume that
# LINGUAS is empty in this case, i.e. $(POFILES) and $(GMOFILES) are empty.
# In this case, stamp-po is a nop (i.e. a phony target).

# stamp-po is a timestamp denoting the last time at which the CATALOGS have
# been loosely updated. Its purpose is that when a developer or translator
# checks out the package via CVS, and the $(DOMAIN).pot file is not in CVS,
# "make" will update the $(DOMAIN).pot and the $(CATALOGS), but subsequent
# invocations of "make" will do nothing. This timestamp would not be necessary
# if updating the $(CATALOGS) would always touch them; however, the rule for
# $(POFILES) has been designed to not touch files that don't need to be
# changed.
stamp-po: $(srcdir)/$(DOMAIN).pot
	@$(CHECK_MACRO_VERSION)
	test ! -f $(srcdir)/$(DOMAIN).pot || \
	  test -z "$(GMOFILES)" || $(MAKE) $(GMOFILES)
	@test ! -f $(srcdir)/$(DOMAIN).pot || { \
	  echo "touch stamp-po" && \
	  echo timestamp > stamp-poT && \
	  mv stamp-poT stamp-po; \
	}

# Note: Target 'all' must not depend on target '$(DOMAIN).pot-update',
# otherwise packages like GCC can not be built if only parts of the source
# have been downloaded.

# This target rebuilds $(DOMAIN).pot; it is an expensive operation.
# Note that $(DOMAIN).pot is not touched if it doesn't need to be changed.
$(DOMAIN).pot-update: $(POTFILES) $(srcdir)/POTFILES.in remove-potcdate.sed
	if LC_ALL=C grep 'GNU davfs2' $(top_srcdir)/* 2>/dev/null | grep -v 'libtool:' >/dev/null; then \
	  package_gnu='GNU '; \
	else \
	  package_gnu=''; \
	fi; \
	if test -n '$(MSGID_BUGS_ADDRESS)' || test '$(PACKAGE_BUGREPORT)' = '@'PACKAGE_BUGREPORT'@'; then \
	  msgid_bugs_address='$(MSGID_BUGS_ADDRESS)'; \
	else \
	  msgid_bugs_address='$(PACKAGE_BUGREPORT)'; \
	fi; \
	case `$(XGETTEXT) --version | sed 1q | sed -e 's,^[^0-9]*,,'` in \
	  '' | 0.[0-9] | 0.[0-9].* | 0.1[0-5] | 0.1[0-5].* | 0.16 | 0.16.[0-1]*) \
	    $(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	      --add-comments=TRANSLATORS: $(XGETTEXT_OPTIONS)  --flag=error:3:c-format --flag=error_at_line:5:c-format --flag=asprintf:2:c-format --flag=vasprintf:2:c-format --flag=xasprintf:1:c-format \
	      --files-from=$(srcdir)/POTFILES.in \
	      --copyright-holder='$(COPYRIGHT_HOLDER)' \
	      --msgid-bugs-address="$$msgid_bugs_address" \
	    ;; \
	  *) \
	    $(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	      --add-comments=TRANSLATORS: $(XGETTEXT_OPTIONS)  --flag=error:3:c-format --flag=error_at_line:5:c-format --flag=asprintf:2:c-format --flag=vasprintf:2:c-format --flag=xasprintf:1:c-format \
	      --files-from=$(srcdir)/POTFILES.in \
	      --copyright-holder='$(COPYRIGHT_HOLDER)' \
	      --package-name="$${package_gnu}davfs2" \
	      --package-version='1.4.6' \
	      --msgid-bugs-address="$$msgid_bugs_address" \
	    ;; \
	esac
	test ! -f $(DOMAIN).po || { \
	  if test -f $(srcdir)/$(DOMAIN).pot; then \
	    sed -f remove-potcdate.sed < $(srcdir)/$(DOMAIN).pot > $(DOMAIN).1po && \
	    sed -f remove-potcdate.sed < $(DOMAIN).po > $(DOMAIN).2po && \
	    if cmp $(DOMAIN).1po $(DOMAIN).2po >/dev/null 2>&1; then \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(DOMAIN).po; \
	    else \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(srcdir)/$(DOMAIN).pot && \
	      mv $(DOMAIN).po $(srcdir)/$(DOMAIN).pot; \
	    fi; \
	  else \
	    mv $(DOMAIN).po $(srcdir)/$(DOMAIN).pot; \
	  fi; \
	}

# This rule has no dependencies: we don't need to update $(DOMAIN).pot at
# every "make" invocation, only create it when it is missing.
# Only "make $(DOMAIN).pot-update" or "make dist" will force an update.
$(srcdir)/$(DOMAIN).pot:
	$(MAKE) $(DOMAIN).pot-update

# This target rebuilds a PO file if $(DOMAIN).pot has changed.
# Note that a PO file is not touched if it doesn't need to be changed.
$(POFILES): $(srcdir)/$(DOMAIN).pot
	@lang=`echo $@ | sed -e 's,.*/,,' -e 's/\.po$$//'`; \
	if test -f "$(srcdir)/$${lang}.po"; then \
	  test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	  echo "$${cdcmd}$(MSGMERGE_UPDATE) $(MSGMERGE_OPTIONS) --lang=$${lang} $${lang}.po $(DOMAIN).pot"; \
	  cd $(srcdir) \
	    && { case `$(MSGMERGE_UPDATE) --version | sed 1q | sed -e 's,^[^0-9]*,,'` in \
	           '' | 0.[0-9] | 0.[0-9].* | 0.1[0-7] | 0.1[0-7].*) \
	             $(MSGMERGE_UPDATE) $(MSGMERGE_OPTIONS) $${lang}.po $(DOMAIN).pot;; \
	           *) \
	             $(MSGMERGE_UPDATE) $(MSGMERGE_OPTIONS) --lang=$${lang} $${lang}.po $(DOMAIN).pot;; \
	         esac; \
	       }; \
	else \
	  $(MAKE) $${lang}.po-create; \
	fi


install: install-exec install-data
install-exec:
install-data: install-data-no
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  $(mkdir_p) $(DESTDIR)$(gettextsrcdir); \
	  for file in $(DISTFILES.common) Makevars.template; do \
	    $(INSTALL_DATA) $(srcdir)/$$file \
			    $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	  for file in Makevars; do \
	    rm -f $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	else \
	  : ; \
	fi
install-data-no: all
install-data-yes: all
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  dir=$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkdir_p) $(DESTDIR)$$dir; \
	  if test -r $$cat; then realcat=$$cat; else realcat=$(srcdir)/$$cat; fi; \
	  $(INSTALL_DATA) $$realcat $(DESTDIR)$$dir/$(DOMAIN).mo; \
	  echo "installing $$realcat as $(DESTDIR)$$dir/$(DOMAIN).mo"; \
	  for lc in '' $(EXTRA_LOCALE_CATEGORIES); do \
	    if test -n "$$lc"; then \
	      if (cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc 2>/dev/null) | grep ' -> ' >/dev/null; then \
	        link=`cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc | sed -e 's/^.* -> //'`; \
	        mv $(DESTDIR)$(localedir)/$$lang/$$lc $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	        mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        (cd $(DESTDIR)$(localedir)/$$lang/$$lc.old && \
	         for file in *; do \
	           if test -f $$file; then \
	             ln -s ../$$link/$$file $(DESTDIR)$(localedir)/$$lang/$$lc/$$file; \
	           fi; \
	         done); \
	        rm -f $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	      else \
	        if test -d $(DESTDIR)$(localedir)/$$lang/$$lc; then \
	          :; \
	        else \
	          rm -f $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        fi; \
	      fi; \
	      rm -f $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	      ln -s ../LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo 2>/dev/null || \
	      ln $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo 2>/dev/null || \
	      cp -p $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	      echo "installing $$realcat link as $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo"; \
	    fi; \
	  done; \
	done

install-strip: install

installdirs: installdirs-exec installdirs-data
installdirs-exec:
installdirs-data: installdirs-data-no
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  $(mkdir_p) $(DESTDIR)$(gettextsrcdir); \
	else \
	  : ; \
	fi
installdirs-data-no:
installdirs-data-yes:
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  dir=$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkdir_p) $(DESTDIR)$$dir; \
	  for lc in '' $(EXTRA_LOCALE_CATEGORIES); do \
	    if test -n "$$lc"; then \
	      if (cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc 2>/dev/null) | grep ' -> ' >/dev/null; then \
	        link=`cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc | sed -e 's/^.* -> //'`; \
	        mv $(DESTDIR)$(localedir)/$$lang/$$lc $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	        mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        (cd $(DESTDIR)$(localedir)/$$lang/$$lc.old && \
	         for file in *; do \
	           if test -f $$file; then \
	             ln -s ../$$link/$$file $(DESTDIR)$(localedir)/$$lang/$$lc/$$file; \
	           fi; \
	         done); \
	        rm -f $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	      else \
	        if test -d $(DESTDIR)$(localedir)/$$lang/$$lc; then \
	          :; \
	        else \
	          rm -f $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        fi; \
	      fi; \
	    fi; \
	  done; \
	done

# Define this as empty until I found a useful application.
installcheck:

uninstall: uninstall-exec uninstall-data
uninstall-exec:
uninstall-data: uninstall-data-no
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  for file in $(DISTFILES.common) Makevars.template; do \
	    rm -f $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	else \
	  : ; \
	fi
uninstall-data-no:
uninstall-data-yes:
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  for lc in LC_MESSAGES $(EXTRA_LOCALE_CATEGORIES); do \
	    rm -f $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	  done; \
	done

check: all

info dvi ps pdf html tags TAGS ctags CTAGS ID:

mostlyclean:
	rm -f remove-potcdate.sed
	rm -f stamp-poT
	rm -f core core.* $(DOMAIN).po $(DOMAIN).1po $(DOMAIN).2po *.new.po
	rm -fr *.o

clean: mostlyclean

distclean: clean
	rm -f Makefile Makefile.in POTFILES *.mo

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f stamp-po $(GMOFILES)

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)
dist distdir:
	$(MAKE) update-po
	@$(MAKE) dist2
# This is a separate target because 'update-po' must be executed before.
dist2: stamp-po $(DISTFILES)
	dists="$(DISTFILES)"; \
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  dists="$$dists Makevars.template"; \
	fi; \
	if test -f $(srcdir)/$(DOMAIN).pot; then \
	  dists="$$dists $(DOMAIN).pot stamp-po"; \
	fi; \
	if test -f $(srcdir)/ChangeLog; then \
	  dists="$$dists ChangeLog"; \
	fi; \
	for i in 0 1 2 3 4 5 6 7 8 9; do \
	  if test -f $(srcdir)/ChangeLog.$$i; then \
	    dists="$$dists ChangeLog.$$i"; \
	  fi; \
	done; \
	if test -f $(srcdir)/LINGUAS; then dists="$$dists LINGUAS"; fi; \
	for file in $$dists; do \
	  if test -f $$file; then \
	    cp -p $$file $(distdir) || exit 1; \
	  else \
	    cp -p $(srcdir)/$$file $(distdir) || exit 1; \
	  fi; \
	done

update-po: Makefile
	$(MAKE) $(DOMAIN).pot-update
	test -z "$(UPDATEPOFILES)" || $(MAKE) $(UPDATEPOFILES)
	$(MAKE) update-gmo

# General rule for creating PO files.

.nop.po-create:
	@lang=`echo $@ | sed -e 's/\.po-create$$//'`; \
	echo "File $$lang.po does not exist. If you are a translator, you can create it through 'msginit'." 1>&2; \
	exit 1

# General rule for updating PO files.

.nop.po-update:
	@lang=`echo $@ | sed -e 's/\.po-update$$//'`; \
	if test "$(PACKAGE)" = "gettext-tools"; then PATH=`pwd`/../src:$$PATH; fi; \
	tmpdir=`pwd`; \
	echo "$$lang:"; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}$(MSGMERGE) $(MSGMERGE_OPTIONS) --lang=$$lang $$lang.po $(DOMAIN).pot -o $$lang.new.po"; \
	cd $(srcdir); \
	if { case `$(MSGMERGE) --version | sed 1q | sed -e 's,^[^0-9]*,,'` in \
	       '' | 0.[0-9] | 0.[0-9].* | 0.1[0-7] | 0.1[0-7].*) \
	         $(MSGMERGE) $(MSGMERGE_OPTIONS) -o $$tmpdir/$$lang.new.po $$lang.po $(DOMAIN).pot;; \
	       *) \
	         $(MSGMERGE) $(MSGMERGE_OPTIONS) --lang=$$lang -o $$tmpdir/$$lang.new.po $$lang.po $(DOMAIN).pot;; \
	     esac; \
	   }; then \
	  if cmp $$lang.po $$tmpdir/$$lang.new.po >/dev/null 2>&1; then \
	    rm -f $$tmpdir/$$lang.new.po; \
	  else \
	    if mv -f $$tmpdir/$$lang.new.po $$lang.po; then \
	      :; \
	    else \
	      echo "msgmerge for $$lang.po failed: cannot move $$tmpdir/$$lang.new.po to $$lang.po" 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	else \
	  echo "msgmerge for $$lang.po failed!" 1>&2; \
	  rm -f $$tmpdir/$$lang.new.po; \
	fi

$(DUMMYPOFILES):

update-gmo: Makefile $(GMOFILES)
	@:

# Recreate Makefile by invoking config.status. Explicitly invoke the shell,
# because execution permission bits may not work on the current file system.
# Use /bin/bash, which is the shell determined by autoconf for the use by its
# scripts, not $(SHELL) which is hardwired to /bin/sh and may be deficient.
Makefile: Makefile.in.in Makevars $(top_builddir)/config.status POTFILES.in LINGUAS
	cd $(top_builddir) \
	  && /bin/bash ./config.status $(subdir)/$@.in po-directories

force:

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
