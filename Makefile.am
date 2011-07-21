## Process this file with automake to produce Makefile.in

SUBDIRS = src doc

rpm: clean
	@(unset CDPATH ; $(MAKE) dist && cp $(distdir).tar.gz \
	  $(HOME)/rpmbuild/SOURCES/ && rpmbuild -ba libvirt-qmf.spec)

install-data-local: install-init
uninstall-local:: uninstall-init

install-init: libvirt-qmf.init
	mkdir -p $(DESTDIR)$(sysconfdir)/rc.d/init.d
	$(INSTALL_SCRIPT) libvirt-qmf.init \
	  $(DESTDIR)$(sysconfdir)/rc.d/init.d/libvirt-qmf

uninstall-init:
	rm -f $(DESTDIR)$(sysconfdir)/rc.d/init.d/libvirt-qmf

libvirt-qmf.init: libvirt-qmf.init.in
	sed						\
	    -e s!\@localstatedir\@!@localstatedir@!g	\
	    -e s!\@sbindir\@!@sbindir@!g		\
	    -e s!\@sysconfdir\@!@sysconfdir@!g		\
	    < $< > $@-t
	chmod a+x $@-t
	mv $@-t $@

EXTRA_DIST = 	autogen.sh \
		libvirt-qmf.init.in

CLEANFILES = 	libvirt-qmf.init

