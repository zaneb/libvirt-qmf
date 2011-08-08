import unittest
from LibvirtQMFTest import *


PROPERTIES = ('hostname', 'uri', 'libvirtVersion', 'apiVersion',
    'hypervisorVersion', 'hypervisorType', 'model', 'memory', 'cpus', 'mhz',
    'nodes', 'sockets', 'cores', 'threads')


class NodeTest(unittest.TestCase, LibvirtQMFTest):

    @classmethod
    def setUpClass(cls):
        LibvirtQMFTest.setUpClass()
        cls.cleanUp()

    @classmethod
    def tearDownClass(cls):
        LibvirtQMFTest.tearDownClass()

    def test_get(self):
        nodes = self.getObjects(NODE)
        self.assertTrue(nodes)
        self.assertEqual(len(nodes), 1)
        node = nodes[0]
        self.assertIsNotNone(node)

    def node(self):
        return self.getObjects(NODE)[0]

    def test_poolDefine(self):
        result = self.node().storagePoolDefineXML(getPoolXML())
        self.assertEqual(result.status, 0, result)
        self.assertIn('pool', result.outArgs)
        pool_obj_id = result.outArgs['pool']
        self.assertTrue(pool_obj_id)
        pool = self.getObject(pool_obj_id)
        self.assertIsNotNone(pool)
        pool.destroy()
        pool.delete()
        pool.undefine()

    def test_poolCreate(self):
        result = self.node().storagePoolCreateXML(getPoolXML(path="/tmp"))
        self.assertEqual(result.status, 0, result)
        self.assertIn('pool', result.outArgs)
        pool_obj_id = result.outArgs['pool']
        self.assertTrue(pool_obj_id)
        pool = self.getObject(pool_obj_id)
        self.assertIsNotNone(pool)
        pool.destroy()
        pool.delete()
        pool.undefine()

    def test_findPoolSources(self):
        result = self.node().findStoragePoolSources('logical', '')
        self.assertEqual(result.status, 0, result)
        self.assertIn('xmlDesc', result.outArgs)
        desc = result.outArgs['xmlDesc'].strip()
        self.assertTrue(desc.startswith('<sources>'))
        self.assertTrue(desc.endswith('</sources>'))

    @unittest.expectedFailure
    def test_findPoolSources_error(self):
        result = self.node().findStoragePoolSources('rubbish', '')
        self.assertEqual(result.status, (1 << 16) | 1, result)
        self.assertNotEqual(result.text.find('virConnectFindStoragePoolSources'), -1)

    def test_properties(self):
        properties = tuple(str(k) for k,v in self.node().getProperties())
        for p in PROPERTIES:
            self.assertIn(p, properties,
                          'Missing property "%s"' % p)


def suite():
    return unittest.TestLoader().loadTestsFromTestCase(NodeTest)

