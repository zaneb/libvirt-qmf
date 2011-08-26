## Process this file with automake to produce Makefile.in

SUBDIRS = src doc

rpm: clean
	@(unset CDPATH ; $(MAKE) dist && cp $(distdir).tar.gz \
	  $(HOME)/rpmbuild/SOURCES/ && rpmbuild -ba libvirt-qmf.spec)

install-data-local: install-init install-service
uninstall-local:: uninstall-init uninstall-service

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

install-service: libvirt-qmf.service
	mkdir -p $(DESTDIR)/lib/systemd/system/
	$(INSTALL_SCRIPT) $< \
	  $(DESTDIR)/lib/systemd/system/

uninstall-service:
	rm -f $(DESTDIR)/lib/systemd/system/libvirt-qmf.service

libvirt-qmf.service: libvirt-qmf.service.in
	sed						\
	    -e s!\@localstatedir\@!@localstatedir@!g	\
	    -e s!\@sbindir\@!@sbindir@!g		\
	    -e s!\@sysconfdir\@!@sysconfdir@!g		\
	    < $< > $@-t
	mv $@-t $@

EXTRA_DIST = 	autogen.sh \
		libvirt-qmf.init.in libvirt-qmf.service.in

CLEANFILES = 	libvirt-qmf.init libvirt-qmf.service

