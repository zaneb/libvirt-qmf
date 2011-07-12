#ifndef NODE_WRAP_H
#define NODE_WRAP_H

#include "LibvirtAgent.h"

#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <sstream>

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>


class DomainWrap;
class PoolWrap;

class NodeWrap:
    public PackageOwner<LibvirtAgent::PackageDefinition>,
    public ManagedObject
{
    typedef std::vector<DomainWrap*> DomainList;
    typedef std::vector<PoolWrap*> PoolList;

    DomainList _domains;
    PoolList _pools;

    virConnectPtr _conn;

    LibvirtAgent *_agent;

public:
    NodeWrap(LibvirtAgent *agent);
    ~NodeWrap();

    void poll(void);

    bool handleMethod(qmf::AgentSession& session, qmf::AgentEvent& event);

protected:
    void syncDomains(void);
    void syncPools(void);
    void checkPool(char *pool_name);

    bool domainDefineXML(qmf::AgentSession& session,
                         qmf::AgentEvent& event);
    bool storagePoolDefineXML(qmf::AgentSession& session,
                              qmf::AgentEvent& event);
    bool storagePoolCreateXML(qmf::AgentSession& session,
                              qmf::AgentEvent& event);
    bool findStoragePoolSources(qmf::AgentSession& session,
                                qmf::AgentEvent& event);
};

#endif

