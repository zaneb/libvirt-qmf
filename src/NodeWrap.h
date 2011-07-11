#ifndef NODE_WRAP_H
#define NODE_WRAP_H

#include "ManagedObject.h"
#include "QmfPackage.h"

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
    public PackageOwner<qmf::com::redhat::libvirt::PackageDefinition>,
    public ManagedObject
{
    typedef std::vector<DomainWrap*> DomainList;
    typedef std::vector<PoolWrap*> PoolList;

    DomainList _domains;
    PoolList _pools;

    virConnectPtr _conn;

    qmf::AgentSession& _session;
    PackageDefinition& _package;

public:
    NodeWrap(qmf::AgentSession& agent_session, PackageDefinition& package);
    ~NodeWrap();

    void doLoop();

    bool handleMethod(qmf::AgentSession& session, qmf::AgentEvent& event);

    virtual PackageDefinition& package(void) { return _package; }

    virtual void addData(qmf::Data& data) {
        _session.addData(data);
    }

    virtual void delData(qmf::Data& data) {
        _session.delData(data.getAddr());
    }

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

