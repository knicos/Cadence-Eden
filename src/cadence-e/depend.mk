#
# $Id: depend.mk,v 1.1 2001/08/01 18:30:21 cssbz Exp $
#

# This Makefile snippet is included from other Makefiles, but only if
# the switch --enable-depend was passed to ./configure (as it is rather
# eager dependency checking and will come into action during make clean
# etc - we don't want to confuse casual Eden builders with all this).

# For the source of all this, see the GNU Make info manual.

# Implicit rule to make .d (dependency) files from .c files.  Use the
# compiler pre-processor to generate lines for make.  sed the output from
# (eg) main.o : main.c defs.h -> main.o main.d : main.c defs.h.
%.d: %.c
	set -e; $(CC) -M $(CPPFLAGS) $(ALLCFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

# Make all the .d files
depend:		$(DEPENDS)

# Include all the .d files here to give make knowledge of source dependencies
# but not if the macro INHIBIT_DEPEND is set to something (set this in the
# call to make using something like make INHIBIT_DEPEND=y generate)
ifeq (,$(INHIBIT_DEPEND))
  include $(DEPENDS)
endif
