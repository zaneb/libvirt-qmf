import unittest
from LibvirtQMFTest import *


PROPERTIES = ('uuid', 'name', 'id', 'node', 'maximumMemory', 'memory',
    'cpuTime', 'state', 'numVcpus', 'active')


class DomainTest(unittest.TestCase, LibvirtQMFTest):

    @classmethod
    def setUpClass(cls):
        LibvirtQMFTest.setUpClass()
        cls.cleanUp()

    @classmethod
    def tearDownClass(cls):
        LibvirtQMFTest.tearDownClass()

    def setUp(self):
        self.node = self.getObjects(NODE)[0]

        result = self.node.storagePoolDefineXML(getPoolXML())
        self.pool = self.getObject(result.outArgs['pool'])
        self.pool.build()
        self.pool.create()

        result = self.pool.createVolumeXML(getVolumeXML())
        self.volume = self.getObject(result.outArgs['volume'])

        result = self.node.domainDefineXML(getDomainXML())
        domain_obj_id = result.outArgs['domain']
        self.domain = self.getObject(domain_obj_id)

    def tearDown(self):
        if self.domain:
            self.domain.undefine()

        self.volume.delete()
        self.pool.destroy()
        self.pool.delete()
        self.pool.undefine()

    def test_parentNode(self):
        self.assertEqual(self.domain.node, self.node.getObjectId())

    def test_getDescription(self):
        result = self.domain.getXMLDesc()
        self.assertIn('description', result.outArgs)
        desc = result.outArgs['description']
        nameXML = '<name>%s</name>' % DOMAIN_PARAMS['name']
        self.assertNotEqual(desc.find(nameXML), -1)

    def test_create_error(self):
        result = self.domain.create()
        self.assertEqual(result.status, (1 << 16) | 1, result)
        self.assertNotEqual(result.text.find('virDomainCreate'), -1)

    def test_destroy_error(self):
        result = self.domain.destroy()
        self.assertEqual(result.status, (1 << 16) | 55, result)
        self.assertNotEqual(result.text.find('virDomainDestroy'), -1)

    def test_suspend_error(self):
        result = self.domain.suspend()
        self.assertEqual(result.status, (1 << 16) | 55, result)
        self.assertNotEqual(result.text.find('virDomainSuspend'), -1)

    def test_resume_error(self):
        result = self.domain.resume()
        self.assertEqual(result.status, (1 << 16) | 55, result)
        self.assertNotEqual(result.text.find('virDomainResume'), -1)

    def test_save_error(self):
        result = self.domain.save('/tmp/test_domain_saved')
        self.assertEqual(result.status, (1 << 16) | 55, result)
        self.assertNotEqual(result.text.find('virDomainSave'), -1)

    def test_restore_error(self):
        result = self.domain.restore('/tmp/test_domain_restore')
        self.assertEqual(result.status, (1 << 16) | 9, result)
        self.assertNotEqual(result.text.find('virDomainRestore'), -1)

    def test_shutdown(self):
        result = self.domain.shutdown()
        self.assertEqual(result.status, (1 << 16) | 55, result)
        self.assertNotEqual(result.text.find('virDomainShutdown'), -1)

    def test_reboot_error(self):
        result = self.domain.reboot()
        self.assertEqual(result.status, (1 << 16) | 3, result)
        self.assertNotEqual(result.text.find('virDomainReboot'), -1)

    def test_undefine(self):
        result = self.domain.undefine()
        self.assertEqual(result.status, 0, result)
        self.domain = None

    @unittest.expectedFailure
    def test_properties(self):
        properties = tuple(str(k) for k,v in self.domain.getProperties())
        for p in PROPERTIES:
            self.assertIn(p, properties,
                          'Missing property "%s"' % p)


def suite():
    return unittest.TestLoader().loadTestsFromTestCase(DomainTest)

