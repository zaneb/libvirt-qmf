## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src/qmf/com/redhat/libvirt $(XML_CFLAGS)

sbin_PROGRAMS = libvirt-qpid

generated_file_list = \
	qmf/com/redhat/libvirt/QmfPackage.cpp\
	qmf/com/redhat/libvirt/QmfPackage.h

nodist_libvirt_qpid_SOURCES = $(generated_file_list)

libvirt_qpid_SOURCES = \
	DomainWrap.cpp \
	Error.cpp \
	Exception.cpp \
	NodeWrap.cpp \
	PoolWrap.cpp \
	VolumeWrap.cpp \
	ManagedObject.h \
	DomainWrap.h \
	Error.h \
	Exception.h \
	NodeWrap.h \
	PoolWrap.h \
	VolumeWrap.h

$(generated_file_list): .libvirt-schema.xml.tstamp

.libvirt-schema.xml.tstamp: libvirt-schema.xml
	qmf-gen -2 -o ./qmf $< && touch $@ || rm -f $@

BUILT_SOURCES = $(generated_file_list)
CLEANFILES = $(generated_file_list) .libvirt-schema.xml.tstamp

libvirt_qpid_LDADD = -lqmf2 -lqpidtypes -lqpidcommon -lqpidmessaging -lvirt $(XML_LIBS)

dist_pkgdata_DATA = libvirt-schema.xml

#CLEANFILES = *.gcov .libs/*.gcda .libs/*.gcno *.gcno *.gcda


