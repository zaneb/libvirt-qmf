import unittest
from LibvirtQMFTest import *


PROPERTIES = ('key', 'path', 'name', 'childLVMName', 'capacity', 'allocation',
    'storagePool')

capacity_unit = dict(zip(('', 'K', 'M', 'G', 'T', 'P', 'E'),
                         (1024 ** i for i in range(7))))


class VolumeTest(unittest.TestCase, LibvirtQMFTest):

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
        pool_obj_id = result.outArgs['pool']
        self.pool = self.getObject(pool_obj_id)
        self.pool.build()
        self.pool.create()

        result = self.pool.createVolumeXML(getVolumeXML())
        vol_obj_id = result.outArgs['volume']
        self.volume = self.getObject(vol_obj_id)

    def tearDown(self):
        if self.volume:
            self.volume.delete()

        self.pool.destroy()
        self.pool.delete()
        self.pool.undefine()

    def test_parentPool(self):
        self.assertIn('storagePool', (str(k) for k,v in self.volume.getProperties()))
        self.assertEqual(self.volume.storagePool, self.pool.getObjectId())

    def test_capacity(self):
        self.assertEqual(self.volume.capacity,
                         VOLUME_PARAMS['capacity'] *
                         capacity_unit[VOLUME_PARAMS['capacity_unit']])

    def test_getDescription(self):
        result = self.volume.getXMLDesc()
        self.assertEqual(result.status, 0, result)
        self.assertIn('description', result.outArgs)
        desc = result.outArgs['description']
        pathXML = '<path>%s</path>' % DOMAIN_PARAMS['source_file']
        self.assertNotEqual(desc.find(pathXML), -1)

    def test_delete(self):
        result = self.volume.delete()
        self.assertEqual(result.status, 0, result)
        self.volume = None

    def test_properties(self):
        properties = tuple(str(k) for k,v in self.volume.getProperties())
        for p in PROPERTIES:
            self.assertIn(p, properties,
                          'Missing property "%s"' % p)

    def test_domainDefine(self):
        result = self.node.domainDefineXML(getDomainXML())
        self.assertEqual(result.status, 0, result)
        self.assertIn('domain', result.outArgs)
        domain_obj_id = result.outArgs['domain']
        self.assertTrue(domain_obj_id)
        domain = self.getObject(domain_obj_id)
        self.assertIsNotNone(domain)
        domain.undefine()


def suite():
    return unittest.TestLoader().loadTestsFromTestCase(VolumeTest)

