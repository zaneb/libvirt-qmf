#ifndef POOL_WRAP_H
#define POOL_WRAP_H

#include "NodeWrap.h"

#include <unistd.h>
#include <cstdlib>
#include <iostream>

#include <sstream>

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>


class VolumeWrap;

class PoolWrap:
    public PackageOwner<NodeWrap::PackageDefinition>,
    public ManagedObject
{
    typedef std::vector<VolumeWrap*> VolumeList;

    virStoragePoolPtr _pool_ptr;
    virConnectPtr _conn;

    VolumeList _volumes;

    char *_pool_sources_xml;
    int _storagePoolState;

    std::string _pool_name;
    std::string _pool_uuid;

public:
    PoolWrap(NodeWrap *parent,
             virStoragePoolPtr pool_ptr,
             virConnectPtr connect);
    ~PoolWrap();

    std::string name(void) { return _pool_name; }
    std::string uuid(void) { return _pool_uuid; }

    bool handleMethod(qmf::AgentSession& session, qmf::AgentEvent& event);

    const char *getPoolSourcesXml();
    void update();
    void syncVolumes();

private:
    void updateProperties();
    bool createVolumeXML(qmf::AgentSession& session, qmf::AgentEvent& event);

};

#endif

