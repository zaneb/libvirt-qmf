import unittest
from LibvirtQMFTest import *


PROPERTIES = ('uuid', 'name', 'parentVolume', 'state', 'capacity', 'allocation',
    'available', 'node')


class PoolTest(unittest.TestCase, LibvirtQMFTest):

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

    def tearDown(self):
        if self.pool:
            self.pool.destroy()
            self.pool.delete()
            self.pool.undefine()

    def test_parentNode(self):
        self.assertIn('node', (str(k) for k,v in self.pool.getProperties()))
        self.assertEqual(self.pool.node, self.node.getObjectId())

    def test_volumeCreate(self):
        self.pool.build()
        self.pool.create()
        result = self.pool.createVolumeXML(getVolumeXML())
        self.assertEqual(result.status, 0, result)
        self.assertIn('volume', result.outArgs)
        vol_obj_id = result.outArgs['volume']
        self.assertTrue(vol_obj_id)
        volume = self.getObject(vol_obj_id)
        self.assertIsNotNone(volume)
        volume.delete()

    def test_getDescription(self):
        result = self.pool.getXMLDesc()
        self.assertIn('description', result.outArgs)
        desc = result.outArgs['description']
        nameXML = '<name>%s</name>' % POOL_PARAMS['name']
        self.assertNotEqual(desc.find(nameXML), -1)

    def test_refresh_error(self):
        result = self.pool.refresh()
        self.assertEqual(result.status, (1 << 16) | 55, result)
        self.assertNotEqual(result.text.find('virStoragePoolRefresh'), -1)

    def test_methods(self):
        result = self.pool.build()
        self.assertEqual(result.status, 0, result)
        result = self.pool.create()
        self.assertEqual(result.status, 0, result)

        result = self.pool.refresh()
        self.assertEqual(result.status, 0, result)

        result = self.pool.destroy()
        self.assertEqual(result.status, 0, result)
        result = self.pool.delete()
        self.assertEqual(result.status, 0, result)
        result = self.pool.undefine()
        self.assertEqual(result.status, 0, result)
        self.pool = None

    def test_properties(self):
        properties = tuple(str(k) for k,v in self.pool.getProperties())
        for p in PROPERTIES:
            self.assertIn(p, properties,
                          'Missing property "%s"' % p)


def suite():
    return unittest.TestLoader().loadTestsFromTestCase(PoolTest)

