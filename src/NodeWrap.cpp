
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <cstdio>

#include "NodeWrap.h"
#include "DomainWrap.h"
#include "PoolWrap.h"
#include "Error.h"
#include "Exception.h"


NodeWrap::NodeWrap(LibvirtAgent *agent):
    PackageOwner<LibvirtAgent::PackageDefinition>(agent),
    ManagedObject(package().data_Node),
    _agent(agent)
{
    virNodeInfo info;
    char *hostname;
    char libvirt_version[256] = "Unknown";
    char api_version[256] = "Unknown";
    char hv_version[256] = "Unknown";
    char *uri;
    const char *hv_type;
    unsigned long api_v;
    unsigned long libvirt_v;
    unsigned long hv_v;
    int ret;
    unsigned int major;
    unsigned int minor;
    unsigned int rel;

    _conn = virConnectOpen(NULL);
    if (!_conn) {
        REPORT_ERR(_conn, "virConnectOpen");
        throw -1;
    }

    hostname = virConnectGetHostname(_conn);
    if (hostname == NULL) {
        REPORT_ERR(_conn, "virConnectGetHostname");
        throw -1;
    }

    hv_type = virConnectGetType(_conn);
    if (hv_type == NULL) {
        REPORT_ERR(_conn, "virConnectGetType");
        throw -1;
    }

    uri = virConnectGetURI(_conn);
    if (uri == NULL) {
        REPORT_ERR(_conn, "virConnectGetURI");
        throw -1;
    }

    ret = virGetVersion(&libvirt_v, hv_type, &api_v);
    if (ret < 0) {
        REPORT_ERR(_conn, "virGetVersion");
    } else {
        major = libvirt_v / 1000000;
        libvirt_v %= 1000000;
        minor = libvirt_v / 1000;
        rel = libvirt_v % 1000;
        snprintf(libvirt_version, sizeof(libvirt_version), "%d.%d.%d", major, minor, rel);

        major = api_v / 1000000;
        api_v %= 1000000;
        minor = api_v / 1000;
        rel = api_v % 1000;
        snprintf(api_version, sizeof(api_version), "%d.%d.%d", major, minor, rel);
    }

    ret = virConnectGetVersion(_conn, &hv_v);
    if (ret < 0) {
        REPORT_ERR(_conn, "virConnectGetVersion");
    } else {
        major = hv_v / 1000000;
        hv_v %= 1000000;
        minor = hv_v / 1000;
        rel = hv_v % 1000;
        snprintf(hv_version, sizeof(hv_version), "%d.%d.%d", major, minor, rel);
    }

    ret = virNodeGetInfo(_conn, &info);
    if (ret < 0) {
        REPORT_ERR(_conn, "virNodeGetInfo");
        memset((void *) &info, sizeof(info), 1);
    }


    _data.setProperty("hostname", hostname);
    _data.setProperty("uri", uri);
    _data.setProperty("libvirtVersion", libvirt_version);
    _data.setProperty("apiVersion", api_version);
    _data.setProperty("hypervisorVersion", hv_version);
    _data.setProperty("hypervisorType", hv_type);

    _data.setProperty("model", info.model);
    _data.setProperty("memory", (uint64_t)info.memory);
    _data.setProperty("cpus", info.cpus);
    _data.setProperty("mhz", info.mhz);
    _data.setProperty("nodes", info.nodes);
    _data.setProperty("sockets", info.sockets);
    _data.setProperty("cores", info.cores);
    _data.setProperty("threads", info.threads);

    addData(_data);
}

NodeWrap::~NodeWrap()
{
    /* Go through our list of pools and destroy them all! MOOOHAHAHA */
    PoolList::iterator iter_p = _pools.begin();
    while (iter_p != _pools.end()) {
        delete *iter_p;
        iter_p = _pools.erase(iter_p);
    }

    /* Same for domains.. */
    DomainList::iterator iter_d = _domains.begin();
    while (iter_d != _domains.end()) {
        delete *iter_d;
        iter_d = _domains.erase(iter_d);
    }

    delData(_data);
}

void NodeWrap::syncDomains(void)
{
    /* Sync up with domains that are defined but not active. */
    int maxname = virConnectNumOfDefinedDomains(_conn);
    if (maxname < 0) {
        REPORT_ERR(_conn, "virConnectNumOfDefinedDomains");
        return;
    } else {
        char **dnames;
        dnames = (char **) malloc(sizeof(char *) * maxname);

        if ((maxname = virConnectListDefinedDomains(_conn, dnames, maxname)) < 0) {
            REPORT_ERR(_conn, "virConnectListDefinedDomains");
            free(dnames);
            return;
        }


        for (int i = 0; i < maxname; i++) {
            virDomainPtr domain_ptr;

            bool found = false;
            for (DomainList::iterator iter = _domains.begin();
                    iter != _domains.end(); iter++) {
                if ((*iter)->name() == dnames[i]) {
                    found = true;
                    break;
                }
            }

            if (found) {
                continue;
            }

            domain_ptr = virDomainLookupByName(_conn, dnames[i]);
            if (!domain_ptr) {
                REPORT_ERR(_conn, "virDomainLookupByName");
            } else {
                DomainWrap *domain;
                try {
                    domain = new DomainWrap(this, domain_ptr, _conn);
                    printf("Created new domain: %s, ptr is %p\n", dnames[i], domain_ptr);
                    _domains.push_back(domain);
                } catch (int i) {
                    printf("Error constructing domain\n");
                    REPORT_ERR(_conn, "constructing domain.");
                    delete domain;
                }
            }
        }
        for (int i = 0; i < maxname; i++) {
            free(dnames[i]);
        }

        free(dnames);
    }

    /* Go through all the active domains */
    int maxids = virConnectNumOfDomains(_conn);
    if (maxids < 0) {
        REPORT_ERR(_conn, "virConnectNumOfDomains");
        return;
    } else {
        int *ids;
        ids = (int *) malloc(sizeof(int *) * maxids);

        if ((maxids = virConnectListDomains(_conn, ids, maxids)) < 0) {
            printf("Error getting list of defined domains\n");
            return;
        }

        for (int i = 0; i < maxids; i++) {
            virDomainPtr domain_ptr;
            char dom_uuid[VIR_UUID_STRING_BUFLEN];

            domain_ptr = virDomainLookupByID(_conn, ids[i]);
            if (!domain_ptr) {
                REPORT_ERR(_conn, "virDomainLookupByID");
                continue;
            }

            if (virDomainGetUUIDString(domain_ptr, dom_uuid) < 0) {
                REPORT_ERR(_conn, "virDomainGetUUIDString");
                continue;
            }

            bool found = false;
            for (DomainList::iterator iter = _domains.begin();
                    iter != _domains.end(); iter++) {
                if (strcmp((*iter)->uuid().c_str(), dom_uuid) == 0) {
                    found = true;
                    break;
                }
            }

            if (found) {
                virDomainFree(domain_ptr);
                continue;
            }

            DomainWrap *domain;
            try {
                domain = new DomainWrap(this, domain_ptr, _conn);
                printf("Created new domain: %d, ptr is %p\n", ids[i], domain_ptr);
                _domains.push_back(domain);
            } catch (int i) {
                printf("Error constructing domain\n");
                REPORT_ERR(_conn, "constructing domain.");
                delete domain;
            }
        }

        free(ids);
    }

    /* Go through our list of domains and ensure that they still exist. */
    DomainList::iterator iter = _domains.begin();
    while (iter != _domains.end()) {
        printf("verifying domain %s\n", (*iter)->name().c_str());
        virDomainPtr ptr = virDomainLookupByUUIDString(_conn, (*iter)->uuid().c_str());
        if (ptr == NULL) {
            REPORT_ERR(_conn, "virDomainLookupByUUIDString");
            delete (*iter);
            iter = _domains.erase(iter);
        } else {
            virDomainFree(ptr);
            iter++;
        }
    }
}

void NodeWrap::checkPool(char *pool_name)
{
    virStoragePoolPtr pool_ptr;

    bool found = false;
    for (PoolList::iterator iter = _pools.begin();
            iter != _pools.end(); iter++) {
        if ((*iter)->name() == pool_name) {
            found = true;
            break;
        }
    }

    if (found) {
        return;
    }

    pool_ptr = virStoragePoolLookupByName(_conn, pool_name);
    if (!pool_ptr) {
        REPORT_ERR(_conn, "virStoragePoolLookupByName");
    } else {
        printf("Creating new pool: %s, ptr is %p\n", pool_name, pool_ptr);
        PoolWrap *pool;
        try {
            pool = new PoolWrap(this, pool_ptr, _conn);
            printf("Created new pool: %s, ptr is %p\n", pool_name, pool_ptr);
            _pools.push_back(pool);
        } catch (int i) {
            printf("Error constructing pool\n");
            REPORT_ERR(_conn, "constructing pool.");
            delete pool;
        }
    }
}

void NodeWrap::syncPools(void)
{
    int maxname;

    maxname = virConnectNumOfStoragePools(_conn);
    if (maxname < 0) {
        REPORT_ERR(_conn, "virConnectNumOfStoragePools");
        return;
    } else {
        char *names[maxname];

        if ((maxname = virConnectListStoragePools(_conn, names, maxname)) < 0) {
            REPORT_ERR(_conn, "virConnectListStoragePools");
            return;
        }

        for (int i = 0; i < maxname; i++) {
            checkPool(names[i]);
            free(names[i]);
        }
    }

    maxname = virConnectNumOfDefinedStoragePools(_conn);
    if (maxname < 0) {
        REPORT_ERR(_conn, "virConnectNumOfDefinedStoragePools");
        return;
    } else {
        char *names[maxname];

        if ((maxname = virConnectListDefinedStoragePools(_conn, names, maxname)) < 0) {
            REPORT_ERR(_conn, "virConnectListDefinedStoragePools");
            return;
        }

        for (int i = 0; i < maxname; i++) {
            checkPool(names[i]);
            free(names[i]);
        }
    }

    /* Go through our list of pools and ensure that they still exist. */
    PoolList::iterator iter = _pools.begin();
    while (iter != _pools.end()) {
        printf("Verifying pool %s\n", (*iter)->name().c_str());
        virStoragePoolPtr ptr = virStoragePoolLookupByUUIDString(_conn, (*iter)->uuid().c_str());
        if (ptr == NULL) {
            printf("Destroying pool %s\n", (*iter)->name().c_str());
            delete(*iter);
            iter = _pools.erase(iter);
        } else {
            virStoragePoolFree(ptr);
            iter++;
        }
    }
}

void
NodeWrap::poll(void)
{
    // We're using this to check to see if our connection is still good.
    // I don't see any reason this call should fail unless there is some
    // connection problem..
    int maxname = virConnectNumOfDefinedDomains(_conn);
    if (maxname < 0) {
        return;
    }

    syncDomains();
    syncPools();

    for (DomainList::iterator iter = _domains.begin();
            iter != _domains.end(); iter++) {
        (*iter)->update();
    }
    for (PoolList::iterator iter = _pools.begin();
            iter != _pools.end(); iter++) {
        (*iter)->update();
    }
}

bool
NodeWrap::domainDefineXML(qmf::AgentSession& session,
                          qmf::AgentEvent& event)
{
    const std::string xmlDesc(event.getArguments()["xmlDesc"].asString());
    int ret;

    virDomainPtr domain_ptr = virDomainDefineXML(_conn, xmlDesc.c_str());
    if (!domain_ptr) {
        std::string err = FORMAT_ERR(_conn, "Error creating domain using xml description (virDomainDefineXML).", &ret);
        raiseException(session, event, err, STATUS_USER + ret);
        return false;
    }

    // Now we have to check to see if this domain is actually new or not,
    // because it's possible that one already exists with this name/description
    // and we just replaced it... *ugh*
    DomainList::iterator iter = _domains.begin();
    while (iter != _domains.end()) {
        if ((*iter)->name() == virDomainGetName(domain_ptr)) {
            // We're just replacing an existing domain, however I'm pretty sure
            // the old domain pointer becomes invalid at this point, so we
            // should destroy the old domain reference. The other option would
            // be to replace it and keep the object valid... not sure which is
            // better.
            printf("Old domain already exists, removing it in favor of new object.");
            delete(*iter);
            iter = _domains.erase(iter);
        } else {
            iter++;
        }
    }

    DomainWrap *domain;
    try {
        domain = new DomainWrap(this, domain_ptr, _conn);
        _domains.push_back(domain);
        event.addReturnArgument("domain", domain->objectID());
    } catch (int i) {
        delete domain;
        std::string err = FORMAT_ERR(_conn, "Error constructing domain object in virDomainDefineXML.", &ret);
        raiseException(session, event, err, STATUS_USER + ret);
        return false;
    }

    return true;
}

bool
NodeWrap::storagePoolDefineXML(qmf::AgentSession& session,
                               qmf::AgentEvent& event)
{
    const std::string xmlDesc(event.getArguments()["xmlDesc"].asString());
    virStoragePoolPtr pool_ptr;
    int ret;

    pool_ptr = virStoragePoolDefineXML(_conn, xmlDesc.c_str(), 0);
    if (pool_ptr == NULL) {
        std::string err = FORMAT_ERR(_conn, "Error defining storage pool using xml description (virStoragePoolDefineXML).", &ret);
        raiseException(session, event, err, STATUS_USER + ret);
        return false;
    }

    PoolWrap *pool;
    try {
        pool = new PoolWrap(this, pool_ptr, _conn);
        _pools.push_back(pool);
        event.addReturnArgument("pool", pool->objectID());
    } catch (int i) {
        delete pool;
        std::string err = FORMAT_ERR(_conn, "Error constructing pool object in virStoragePoolDefineXML.", &ret);
        raiseException(session, event, err, STATUS_USER + ret);
        return false;
    }

    return true;
}

bool
NodeWrap::storagePoolCreateXML(qmf::AgentSession& session,
                               qmf::AgentEvent& event)
{
    const std::string xmlDesc(event.getArguments()["xmlDesc"].asString());
    virStoragePoolPtr pool_ptr;
    int ret;

    pool_ptr = virStoragePoolCreateXML (_conn, xmlDesc.c_str(), 0);
    if (pool_ptr == NULL) {
        std::string err = FORMAT_ERR(_conn, "Error creating storage pool using xml description (virStoragePoolCreateXML).", &ret);
        raiseException(session, event, err, STATUS_USER + ret);
        return false;
    }

    PoolWrap *pool;
    try {
        pool = new PoolWrap(this, pool_ptr, _conn);
        _pools.push_back(pool);
        event.addReturnArgument("pool", pool->objectID());
    } catch (int i) {
        delete pool;
        std::string err = FORMAT_ERR(_conn, "Error constructing pool object in virStoragePoolCreateXML.", &ret);
        raiseException(session, event, err, STATUS_USER + ret);
        return false;
    }

    return true;
}

bool
NodeWrap::findStoragePoolSources(qmf::AgentSession& session,
                                 qmf::AgentEvent& event)
{
    qpid::types::Variant::Map args(event.getArguments());
    const std::string type(args["type"].asString());
    const std::string srcSpec(args["srcSpec"].asString());
    char *xml_result;
    int ret;

    xml_result = virConnectFindStoragePoolSources(_conn, type.c_str(), srcSpec.c_str(), 0);
    if (xml_result == NULL) {
        std::string err = FORMAT_ERR(_conn, "Error creating storage pool using xml description (virStoragePoolCreateXML).", &ret);
        raiseException(session, event, err, STATUS_USER + ret);
        return false;
    }

    event.addReturnArgument("xmlDesc", xml_result);
    free(xml_result);

    return true;
}

bool
NodeWrap::handleMethod(qmf::AgentSession& session, qmf::AgentEvent& event)
{
    if (!event.hasDataAddr() || *this == event.getDataAddr()) {
        const std::string& methodName(event.getMethodName());
        bool success;

        if (methodName == "domainDefineXML") {
            success = domainDefineXML(session, event);
        } else if (methodName == "storagePoolDefineXML") {
            success = storagePoolDefineXML(session, event);
        } else if (methodName == "storagePoolCreateXML") {
            success = storagePoolCreateXML(session, event);
        } else if (methodName == "findStoragePoolSources") {
            success = findStoragePoolSources(session, event);
        } else {
            raiseException(session, event,
                           ERROR_UNKNOWN_METHOD, STATUS_UNKNOWN_METHOD);
            return true;
        }

        if (success) {
            session.methodSuccess(event);
        }
        return true;
    } else {
        bool handled = false;

        for (DomainList::iterator iter = _domains.begin();
                !handled && iter != _domains.end(); iter++) {
            handled = (*iter)->handleMethod(session, event);
        }

        for (PoolList::iterator iter = _pools.begin();
                !handled && iter != _pools.end(); iter++) {
            handled = (*iter)->handleMethod(session, event);
        }

        return handled;
    }
}

