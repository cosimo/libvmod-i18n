
vmoddir = $(VMODDIR)
vmod_LTLIBRARIES = libvmod_i18n.la

libvmod_i18n_la_CFLAGS = -I$(VARNISHSRC)/include -I$(VARNISHSRC)
libvmod_i18n_la_LDFLAGS = -module -export-dynamic -avoid-version -shared

libvmod_i18n_la_SOURCES = \
	vmod_i18n.c

nodist_libvmod_i18n_la_SOURCES = \
	vcc_if.c \
	vcc_if.h

VTC_LOG_COMPILER = $(VARNISHSRC)/bin/varnishtest/varnishtest -v -i \
	-Dvarnishd=$(VARNISHSRC)/bin/varnishd/varnishd \
	-Dvmod_topbuild=$(abs_top_builddir)
TEST_EXTENSIONS = .vtc
TESTS = @VMOD_TESTS@

$(libvmod_i18n_la_OBJECTS): vcc_if.h

vcc_if.c vcc_if.h: $(VARNISHSRC)/lib/libvmod_std/vmod.py \
	$(top_srcdir)/src/vmod.vcc
	$(PYTHON) $(VARNISHSRC)/lib/libvmod_std/vmod.py \
	    $(top_srcdir)/src/vmod.vcc

EXTRA_DIST = vmod.vcc \
	$(TESTS)

CLEANFILES = $(builddir)/vcc_if.c \
	$(builddir)/vcc_if.h
