#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <string.h>
#include <sys/select.h>
#include <errno.h>

#include "NodeWrap.h"
#include "PoolWrap.h"
#include "VolumeWrap.h"
#include "Error.h"
#include "Exception.h"


VolumeWrap::VolumeWrap(PoolWrap *parent,
                       virStorageVolPtr volume_ptr,
                       virConnectPtr connect):
    PackageOwner<PoolWrap::PackageDefinition>(parent),
    ManagedObject(package().data_Volume),
    _volume_ptr(volume_ptr), _conn(connect),
    _lvm_name(""), _has_lvm_child(false),
    _wrap_parent(parent)
{
    const char *volume_key;
    char *volume_path;
    const char *volume_name;

    volume_key = virStorageVolGetKey(_volume_ptr);
    if (volume_key == NULL) {
        REPORT_ERR(_conn, "Error getting storage volume key\n");
        throw 1;
    }
    _volume_key = volume_key;

    volume_path = virStorageVolGetPath(_volume_ptr);
    if (volume_path == NULL) {
        REPORT_ERR(_conn, "Error getting volume path\n");
        throw 1;
    }
    _volume_path = volume_path;

    volume_name = virStorageVolGetName(_volume_ptr);
    if (volume_name == NULL) {
        REPORT_ERR(_conn, "Error getting volume name\n");
        throw 1;
    }
    _volume_name = volume_name;

    _data.setProperty("key", _volume_key);
    _data.setProperty("path", _volume_path);
    _data.setProperty("name", _volume_name);
    _data.setProperty("childLVMName", _lvm_name);
    _data.setProperty("storagePool", parent->objectID());

    // Set defaults
    _data.setProperty("capacity", (uint64_t)0);
    _data.setProperty("allocation", (uint64_t)0);

    addData(_data);

    printf("done\n");
}

void
VolumeWrap::checkForLVMPool()
{
    char *real_path = NULL;
    const char *pool_sources_xml;

    pool_sources_xml = _wrap_parent->getPoolSourcesXml();

    if (pool_sources_xml) {
        xmlDocPtr doc;
        xmlNodePtr cur;

        doc = xmlParseMemory(pool_sources_xml, strlen(pool_sources_xml));

        if (doc == NULL ) {
            return;
        }

        cur = xmlDocGetRootElement(doc);

        if (cur == NULL) {
            xmlFreeDoc(doc);
            return;
        }

        xmlChar *path = NULL;
        xmlChar *name = NULL;

        cur = cur->xmlChildrenNode;
        while (cur != NULL) {
            xmlNodePtr source;
            if ((!xmlStrcmp(cur->name, (const xmlChar *) "source"))) {
                source = cur->xmlChildrenNode;
                while (source != NULL) {
                    if ((!xmlStrcmp(source->name, (const xmlChar *) "device"))) {
                        path = xmlGetProp(source, (const xmlChar *) "path");
                    }

                    if ((!xmlStrcmp(source->name, (const xmlChar *) "name"))) {
                        name = xmlNodeListGetString(doc, source->xmlChildrenNode, 1);
                    }

                source = source->next;
                }
                if (name && path) {
                    virStorageVolPtr vol;

                    printf ("xml returned device name %s, path %s; volume path is %s\n", name, path, _volume_path.c_str());
                    vol = virStorageVolLookupByPath(_conn, (char *) path);
                    if (vol != NULL) {
                        real_path = virStorageVolGetPath(vol);
                        if (real_path && strcmp(real_path, _volume_path.c_str()) == 0) {
                            printf ("found matching storage volume associated with pool!\n");
                            _lvm_name.assign((char *) name);
                            _has_lvm_child = true;
                        }
                    }
                    xmlFree(path);
                    xmlFree(name);
                    path = NULL;
                    name = NULL;
                }
            }
            cur = cur->next;
        }
        xmlFreeDoc(doc);
    }
}

void
VolumeWrap::update()
{
    virStorageVolInfo info;
    int ret;

    printf("Updating volume info\n");

    ret = virStorageVolGetInfo(_volume_ptr, &info);
    if (ret < 0) {
        REPORT_ERR(_conn, "VolumeWrap: Unable to get info of storage volume info\n");
        return;
    }
    _data.setProperty("capacity", (uint64_t)info.capacity);
    _data.setProperty("allocation", (uint64_t)info.allocation);
}

VolumeWrap::~VolumeWrap()
{
    virStorageVolFree(_volume_ptr);

    delData(_data);
}

bool
VolumeWrap::handleMethod(qmf::AgentSession& session, qmf::AgentEvent& event)
{
    int ret;

    if (*this != event.getDataAddr()) {
        return false;
    }

    const std::string& methodName(event.getMethodName());
    qpid::types::Variant::Map args(event.getArguments());

    if (methodName == "getXMLDesc") {
        const char *desc = virStorageVolGetXMLDesc(_volume_ptr, 0);
        if (!desc) {
            std::string err = FORMAT_ERR(_conn, "Error getting xml description for volume (virStorageVolGetXMLDesc).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            event.addReturnArgument("description", desc);
            session.methodSuccess(event);
        }
        return true;
    }

    if (methodName == "delete") {
        ret = virStorageVolDelete(_volume_ptr, 0);
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error deleting storage volume (virStorageVolDelete).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            update();
            session.methodSuccess(event);
        }
        return true;
    }

    raiseException(session, event,
                   ERROR_UNKNOWN_METHOD, STATUS_UNKNOWN_METHOD);
    return true;
}
