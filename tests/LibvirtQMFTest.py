#!/usr/bin/env python

import unittest
import qmf.console


(DEFAULT_HOST, DEFAULT_PORT) = ('localhost', 5672)
(CLASSMAP, PACKAGE) = (lambda c: c.lower(), 'com.redhat.libvirt')


(HOST_ENV, PORT_ENV) = ('LIBVIRT_BROKER_HOST', 'LIBVIRT_BROKER_PORT')
(HOST, PORT) = ('host', 'port')

(CLASSNAMES,) = (('Node', 'Pool', 'Volume', 'Domain'),)
(NODE, POOL, VOLUME, DOMAIN) = map(CLASSMAP, CLASSNAMES)

(POOL_PARAMS,) = ({'name': 'test_dir_pool',
                   'path': '/var/lib/libvirt/images'},)
(VOLUME_PARAMS,) = ({'name': 'test_vol.img',
                     'capacity': 42, 'capacity_unit': 'K'},)
(DOMAIN_PARAMS,) = ({'name': 'test_domain',
                     'source_file': '%s/%s' % (POOL_PARAMS['path'],
                                               VOLUME_PARAMS['name'])},)


class LibvirtQMFTest(object):

    brokeraddr = {HOST: DEFAULT_HOST, PORT: DEFAULT_PORT}

    @staticmethod
    def readEnvironment():
        import os
        if HOST_ENV in os.environ:
            LibvirtQMFTest.brokeraddr[HOST] = os.environ[HOST_ENV] or DEFAULT_HOST
        if PORT_ENV in os.environ:
            port = os.environ[PORT_ENV]
            try:
                LibvirtQMFTest.brokeraddr[PORT] = port and int(port) or DEFAULT_PORT
            except ValueError:
                print 'Invalid port "%s"' % port

    @classmethod
    def setUpClass(cls):
        LibvirtQMFTest.readEnvironment()
        cls._session = qmf.console.Session()
        cls._broker = cls._session.addBroker('aqmp://%(host)s:%(port)d' % LibvirtQMFTest.brokeraddr)

    @classmethod
    def tearDownClass(cls):
        cls._session.delBroker(cls._broker)
        cls._session.close()

    @classmethod
    def session(cls):
        return LibvirtQMFTest._session

    @classmethod
    def getObjects(cls, objclass, **kwargs):
        return cls.session().getObjects(_class=objclass,
                                        _package=PACKAGE,
                                        **kwargs)

    @classmethod
    def getObject(cls, obj):
        kw_map = {'_object_name': 'object_name',
                  '_agent_name': 'agent_name',
                  '_agent_epoch': 'epoch'}
        args = dict((kw_map[k], v) for (k, v) in obj.items())
        obj_id = qmf.console.ObjectId.create(**args)
        return cls.session().getObjects(_objectId=obj_id)[0]

    @classmethod
    def cleanUp(cls):
        # Delete existing volume
        for v in cls.getObjects(VOLUME, name=VOLUME_PARAMS['name']):
            r = v.delete()
        # Delete existing pool
        for p in cls.getObjects(POOL, name=POOL_PARAMS['name']):
            r = p.destroy()
            r = p.undefine()
        # Delete existing domain
        for d in cls.getObjects(DOMAIN, name=DOMAIN_PARAMS['name']):
            r = d.destroy()
            r = d.undefine()


def getPoolXML(**kwargs):
    p = POOL_PARAMS.copy()
    p.update(kwargs)
    return """
<pool type="dir">
    <name>%(name)s</name>
    <target>
        <path>%(path)s</path>
    </target>
</pool>
""" % p

def getVolumeXML(**kwargs):
    p = VOLUME_PARAMS.copy()
    p.update(kwargs)
    return """
<volume>
    <name>%(name)s</name>
    <allocation>0</allocation>
    <capacity unit="%(capacity_unit)s">%(capacity)d</capacity>
    <target>
        <format type="raw" />
    </target>
</volume>
""" % p

def getDomainXML(**kwargs):
    p = DOMAIN_PARAMS.copy()
    p.update(kwargs)
    return """
<domain type="kvm">
    <name>%(name)s</name>
    <memory>131072</memory>
    <vcpu>1</vcpu>
    <os>
        <type arch="x86_64">hvm</type>
    </os>
    <devices>
        <emulator>/usr/bin/qemu-kvm</emulator>
        <disk type="file" device="disk">
            <source file="%(source_file)s" />
            <target dev="hda" />
        </disk>
        <graphics type="vnc" port="-1" />
    </devices>
</domain>
"""  % p


def suite():
    modules = map(__import__, CLASSNAMES)
    return unittest.TestSuite([m.suite() for m in modules])

if __name__ == '__main__':
    import Node, Pool, Volume, Domain
    unittest.main(defaultTest='suite')
