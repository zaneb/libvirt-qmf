
#include "NodeWrap.h"
#include "DomainWrap.h"
#include "Error.h"
#include "Exception.h"

#include <cstdio>


DomainWrap::DomainWrap(
        NodeWrap *parent,
        virDomainPtr domain_ptr,
        virConnectPtr conn):
    PackageOwner<NodeWrap::PackageDefinition>(parent),
    ManagedObject(package().data_Domain),
    _domain_ptr(domain_ptr), _conn(conn)
{
    char dom_uuid[VIR_UUID_STRING_BUFLEN];

    if (virDomainGetUUIDString(_domain_ptr, dom_uuid) < 0) {
        REPORT_ERR(_conn, "DomainWrap: Unable to get UUID string of domain.");
        throw 1;
    }

    _domain_uuid = dom_uuid;

    const char *dom_name = virDomainGetName(_domain_ptr);
    if (!dom_name) {
        REPORT_ERR(_conn, "Unable to get domain name!\n");
        throw 1;
    }

    _domain_name = dom_name;

    _data.setProperty("uuid", _domain_uuid);
    _data.setProperty("name", _domain_name);
    _data.setProperty("node", parent->objectID());

    // Set defaults
    _data.setProperty("state", "");
    _data.setProperty("numVcpus", 0);
    _data.setProperty("maximumMemory", (uint64_t)0);
    _data.setProperty("memory", (uint64_t)0);
    _data.setProperty("cpuTime", (uint64_t)0);

    addData(_data);

    printf("Initialised new domain object for %s\n", _domain_name.c_str());
}

DomainWrap::~DomainWrap()
{
    virDomainFree(_domain_ptr);
    delData(_data);
}

void
DomainWrap::update()
{
    virDomainInfo info;
    int ret;

    ret = virDomainGetInfo(_domain_ptr, &info);
    if (ret < 0) {
        REPORT_ERR(_conn, "Domain get info failed.");
        /* Next domainSync() will take care of this in the node wrapper if the domain is
         * indeed dead. */
        return;
    } else {
        const char *state = NULL;
        switch (info.state) {
            case VIR_DOMAIN_NOSTATE:
            default:
                state = "nostate";
                break;
            case VIR_DOMAIN_RUNNING:
                state = "running";
                break;
            case VIR_DOMAIN_BLOCKED:
                state = "blocked";
                break;
            case VIR_DOMAIN_PAUSED:
                state = "paused";
                break;
            case VIR_DOMAIN_SHUTDOWN:
                state = "shutdown";
                break;
            case VIR_DOMAIN_SHUTOFF:
                state = "shutoff";
                break;
            case VIR_DOMAIN_CRASHED:
                state = "crashed";
                break;
        }

        _data.setProperty("state", state);
        _data.setProperty("numVcpus", info.nrVirtCpu);
        _data.setProperty("maximumMemory", (uint64_t)info.maxMem);
        _data.setProperty("memory", (uint64_t)info.memory);
        _data.setProperty("cpuTime", (uint64_t)info.cpuTime);
    }

    int id = virDomainGetID(_domain_ptr);
    // Just set it directly, if there's an error, -1 will be right.
    _data.setProperty("id", id);

    _data.setProperty("active", (id > 0) ? "true" : "false");
}

bool
DomainWrap::handleMethod(qmf::AgentSession& session, qmf::AgentEvent& event)
{
    int ret;

    if (*this != event.getDataAddr()) {
        return false;
    }

    const std::string& methodName(event.getMethodName());
    qpid::types::Variant::Map args(event.getArguments());

    if (methodName == "create") {
        int ret = virDomainCreate(_domain_ptr);
        update();
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error creating new domain (virDomainCreate).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            session.methodSuccess(event);
        }
        return true;
    }

    else if (methodName == "destroy") {
        int ret = virDomainDestroy(_domain_ptr);
        update();
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error destroying domain (virDomainDestroy).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            session.methodSuccess(event);
        }
        return true;
    }

    else if (methodName == "undefine") {
        int ret = virDomainUndefine(_domain_ptr);
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error undefining domain (virDomainUndefine).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            session.methodSuccess(event);
        }
        /* We now wait for domainSync() to clean this up. */
        return true;
    }

    else if (methodName == "suspend") {
        int ret = virDomainSuspend(_domain_ptr);
        update();
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error suspending domain (virDomainSuspend).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            session.methodSuccess(event);
        }
        return true;
    }

    else if (methodName == "resume") {
        int ret = virDomainResume(_domain_ptr);
        update();
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error resuming domain (virDomainResume).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            session.methodSuccess(event);
        }
        return true;
    }

    else if (methodName == "save") {
        const std::string filename = args["filename"].asString();
        int ret = virDomainSave(_domain_ptr, filename.c_str());
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error saving domain (virDomainSave).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            session.methodSuccess(event);
        }
        return true;
    }

    else if (methodName == "restore") {
        const std::string filename = args["filename"].asString();
        int ret = virDomainRestore(_conn, filename.c_str());
        update();
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error saving domain (virDomainSave).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            session.methodSuccess(event);
        }
        return true;
    }

    else if (methodName == "shutdown") {
        int ret = virDomainShutdown(_domain_ptr);
        update();
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error shutting down domain (virDomainShutdown).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            session.methodSuccess(event);
        }
        return true;
    }

    else if (methodName == "reboot") {
        int ret = virDomainReboot(_domain_ptr, 0);
        update();
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error rebooting domain (virDomainReboot).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            session.methodSuccess(event);
        }
        return true;
    }

    else if (methodName == "getXMLDesc") {
        const char *desc = virDomainGetXMLDesc(_domain_ptr,
                VIR_DOMAIN_XML_SECURE | VIR_DOMAIN_XML_INACTIVE);
        if (!desc) {
            std::string err = FORMAT_ERR(_conn, "Error rebooting domain (virDomainReboot).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            event.addReturnArgument("description", desc);
            session.methodSuccess(event);
        }
        return true;
    }

    else if (methodName == "migrate") {
        if (migrate(session, event)) {
            session.methodSuccess(event);
        }
        return true;
    }

    raiseException(session, event,
                   ERROR_UNKNOWN_METHOD, STATUS_UNKNOWN_METHOD);
    return true;
}

bool
DomainWrap::migrate(qmf::AgentSession& session, qmf::AgentEvent& event)
{
    virConnectPtr dest_conn;
    virDomainPtr rem_dom;
    qpid::types::Variant::Map args(event.getArguments());
    int ret;

    // This is actually quite broken. Most setups won't allow unauthorized
    // connection from one node to another directly like this.
    dest_conn = virConnectOpen(args["destinationUri"].asString().c_str());
    if (!dest_conn) {
        std::string err = FORMAT_ERR(dest_conn, "Unable to connect to remote system for migration: virConnectOpen", &ret);
        raiseException(session, event, err, STATUS_USER + ret);
        return false;
    }

    const std::string newDomainName_arg(args["newDomainName"]);
    const char *new_dom_name = NULL;
    if (newDomainName_arg.size() > 0) {
        new_dom_name = newDomainName_arg.c_str();
    }

    const std::string uri_arg(args["uri"]);
    const char *uri = NULL;
    if (uri_arg.size() > 0) {
        uri = uri_arg.c_str();
    }

    uint32_t flags(args["flags"]);
    uint32_t bandwidth(args["bandwidth"]);

    printf ("calling migrate, new_dom_name: %s, uri: %s, flags: %d (live is %d)\n",
            new_dom_name ? new_dom_name : "NULL",
            uri ? uri : "NULL",
            flags,
            VIR_MIGRATE_LIVE);

    rem_dom = virDomainMigrate(_domain_ptr, dest_conn, flags,
                               new_dom_name,
                               uri,
                               bandwidth);

    virConnectClose(dest_conn);

    if (!rem_dom) {
        std::string err = FORMAT_ERR(_conn, "virDomainMigrate", &ret);
        raiseException(session, event, err, STATUS_USER + ret);
        return false;
    }
    virDomainFree(rem_dom);

    return true;
}

