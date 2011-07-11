#ifndef DOMAIN_WRAP_H
#define DOMAIN_WRAP_H

#include "NodeWrap.h"

#include <unistd.h>
#include <cstdlib>
#include <iostream>

#include <sstream>

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>


class DomainWrap:
    PackageOwner<NodeWrap::PackageDefinition>,
    public ManagedObject
{
    virDomainPtr _domain_ptr;
    virConnectPtr _conn;

    std::string _domain_name;
    std::string _domain_uuid;

public:
    DomainWrap(NodeWrap *parent,
               virDomainPtr domain_ptr, virConnectPtr conn);
    ~DomainWrap();

    std::string& name(void) { return _domain_name; }
    std::string& uuid(void) { return _domain_uuid; }
    void update();

    bool handleMethod(qmf::AgentSession& session, qmf::AgentEvent& event);

private:
    bool migrate(qmf::AgentSession& session, qmf::AgentEvent& event);
};

#endif
