#include <qpid/management/Manageable.h>
#include <qpid/management/ManagementObject.h>
#include <qpid/agent/ManagementAgent.h>
#include <qpid/sys/Mutex.h>

#include "PackageLibvirt.h"
#include "Domain.h"

#include <unistd.h>
#include <cstdlib>
#include <iostream>

#include <sstream>

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>

using namespace qpid::management;
using namespace qpid::sys;
using namespace std;
using qpid::management::ManagementObject;
using qpid::management::Manageable;
using qpid::management::Args;
using qpid::sys::Mutex;


class DomainWrap : public Manageable
{
    string name;
    ManagementAgent *agent;
    Domain *domain;
    Mutex vectorLock;

    virConnectPtr conn;
    virDomainPtr dom;

public:

    DomainWrap(ManagementAgent *agent, NodeWrap *parent, virDomainPtr domain_ptr, 
                virConnectPtr connect, std::string uuid, std::string name);
    ~DomainWrap() 
    { 
        domain->resourceDestroy(); 
    }
    
    ManagementObject* GetManagementObject(void) const
    { 
        return domain; 
    }

    void update();

    status_t ManagementMethod (uint32_t methodId, Args& args);
};
