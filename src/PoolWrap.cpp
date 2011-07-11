#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>

#include "NodeWrap.h"
#include "PoolWrap.h"
#include "VolumeWrap.h"
#include "Error.h"
#include "Exception.h"


PoolWrap::PoolWrap(NodeWrap *parent,
                   virStoragePoolPtr pool_ptr,
                   virConnectPtr connect):
    PackageOwner<NodeWrap::PackageDefinition>(parent),
    ManagedObject(package().data_Pool),
    _pool_ptr(pool_ptr), _conn(connect)
{
    int ret;
    char pool_uuid_str[VIR_UUID_STRING_BUFLEN];
    const char *pool_name_str;
    char *parent_volume = NULL;

    ret = virStoragePoolGetUUIDString(_pool_ptr, pool_uuid_str);
    if (ret < 0) {
        REPORT_ERR(_conn, "PoolWrap: Unable to get UUID\n");
        throw 1;
    }

    pool_name_str = virStoragePoolGetName(_pool_ptr);
    if (pool_name_str == NULL) {
        REPORT_ERR(_conn, "PoolWrap: error getting pool name\n");
        throw 1;
    }

    _pool_sources_xml = virConnectFindStoragePoolSources(_conn, "logical", NULL, 0);

    if (_pool_sources_xml) {
        xmlDocPtr doc;
        xmlNodePtr cur;

        doc = xmlParseMemory(_pool_sources_xml, strlen(_pool_sources_xml));

        if (doc == NULL ) {
            goto done;
        }

        cur = xmlDocGetRootElement(doc);

        if (cur == NULL) {
            xmlFreeDoc(doc);
            goto done;
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
                    if (strcmp(pool_name_str, (char *) name) == 0) {
                        virStorageVolPtr vol;
                        vol = virStorageVolLookupByPath(_conn, (char *) path);
                        if (vol != NULL) {
                            printf ("found storage volume associated with pool!\n");
                            parent_volume = virStorageVolGetPath(vol);
                            printf ("xml returned device name %s, path %s; volume path is %s\n", name, path, parent_volume);
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

done:
    _pool_name = pool_name_str;
    _pool_uuid = pool_uuid_str;

    _data.setProperty("uuid", _pool_uuid);
    _data.setProperty("name", _pool_name);
    _data.setProperty("parentVolume", parent_volume ? parent_volume : "");
    _data.setProperty("node", parent->objectID());

    // Call refresh storage volumes in case anything changed in libvirt.
    // I don't think we're too concerned if it fails?
    virStoragePoolRefresh(_pool_ptr, 0);

    // Set storage pool state to an inactive. Should the state be different, the
    // subsequent call to update will pick it up and fix it.
    _storagePoolState = VIR_STORAGE_POOL_INACTIVE;

    // Set the state before adding the new Data object
    updateProperties();

    // This must be done before update(), which will check for and create any
    // volumes. (VolumeWrap objects will need a valid parent DataAddr.)
    addData(_data);

    // Call update() here so we set the state and see if there are any volumes
    // before returning the new object.
    update();
}

PoolWrap::~PoolWrap()
{
    // Destroy volumes..
    VolumeList::iterator iter = _volumes.begin();
    while (iter != _volumes.end()) {
        delete (*iter);
        iter = _volumes.erase(iter);
    }

    virStoragePoolFree(_pool_ptr);

    delData(_data);
}

const char *
PoolWrap::getPoolSourcesXml()
{
    return _pool_sources_xml;
}

void
PoolWrap::syncVolumes()
{
    int maxactive;
    int ret;
    int i;
    virStoragePoolInfo info;

    std::cout << "Syncing volumes.\n";

    ret = virStoragePoolGetInfo(_pool_ptr, &info);
    if (ret < 0) {
        REPORT_ERR(_conn, "PoolWrap: Unable to get info of storage pool");
        return;
    }

    // Only try to list volumes if the storage pool is active.
    if (info.state != VIR_STORAGE_POOL_INACTIVE) {

        maxactive = virStoragePoolNumOfVolumes(_pool_ptr);
        if (maxactive < 0) {
            //vshError(ctl, FALSE, "%s", _("Failed to list active vols"));
            REPORT_ERR(_conn, "error getting number of volumes in pool\n");
            return;
        }

        char **names;
        names = (char **) malloc(sizeof(char *) * maxactive);

        ret = virStoragePoolListVolumes(_pool_ptr, names, maxactive);
        if (ret < 0) {
            REPORT_ERR(_conn, "error getting list of volumes\n");
            return;
        }

        for (i = 0; i < ret; i++) {
            virStorageVolPtr vol_ptr;
            bool found = false;
            char *volume_name = names[i];

            for (VolumeList::iterator iter = _volumes.begin();
                 iter != _volumes.end(); iter++) {
                if (strcmp((*iter)->name(), volume_name) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                vol_ptr = virStorageVolLookupByName(_pool_ptr, volume_name);
                if (vol_ptr == NULL) {
                    REPORT_ERR(_conn, "error looking up storage volume by name\n");
                    continue;
                }
                VolumeWrap *volume = NULL;
                try {
                    VolumeWrap *volume = new VolumeWrap(this, vol_ptr, _conn);
                    printf("Created new volume: %s, ptr is %p\n", volume_name, vol_ptr);
                    _volumes.push_back(volume);
                } catch (int i) {
                    printf ("Error constructing volume\n");
                    REPORT_ERR(_conn, "constructing volume.");
                    if (volume) {
                        delete volume;
                    }
                }
            }
        }

        for (i = 0; i < ret; i++) {
            free(names[i]);
        }
        free(names);
    }

    /* Go through our list of volumes and ensure that they still exist. */
    VolumeList::iterator iter = _volumes.begin();
    while (iter != _volumes.end()) {
        printf ("Verifying volume %s\n", (*iter)->name());
        virStorageVolPtr ptr = virStorageVolLookupByName(_pool_ptr, (*iter)->name());
        if (ptr == NULL) {
            printf("Destroying volume %s\n", (*iter)->name());
            delete(*iter);
            iter = _volumes.erase(iter);
        } else {
            virStorageVolFree(ptr);
            iter++;
        }
    }

    /* And finally *phew*, call update() on all volumes. */
    for (iter = _volumes.begin(); iter != _volumes.end(); iter++) {
        (*iter)->update();
    }
}

void
PoolWrap::updateProperties(void)
{
    virStoragePoolInfo info;
    int ret;

    ret = virStoragePoolGetInfo(_pool_ptr, &info);
    if (ret < 0) {
        REPORT_ERR(_conn, "PoolWrap: Unable to get info of storage pool");
        return;
    }

    const char *state = NULL;
    switch (info.state) {
        case VIR_STORAGE_POOL_INACTIVE:
            state = "inactive";
            break;
        case VIR_STORAGE_POOL_BUILDING:
            state = "building";
            break;
        case VIR_STORAGE_POOL_RUNNING:
        default:
            state = "running";
            break;
        case VIR_STORAGE_POOL_DEGRADED:
            state = "degraded";
            break;
    }
    _data.setProperty("state", state);

    _data.setProperty("capacity", (uint64_t)info.capacity);
    _data.setProperty("allocation", (uint64_t)info.allocation);
    _data.setProperty("available", (uint64_t)info.available);

    // Check if state has changed compared to stored state. If so, rescan
    // storage pool sources (eg. logical pools on a lun might now be visible)
    if (_storagePoolState != info.state) {
        _pool_sources_xml = virConnectFindStoragePoolSources(_conn, "logical", NULL, 0);
    }

    _storagePoolState = info.state;
}

void
PoolWrap::update(void)
{
    updateProperties();

    // Sync volumes after (potentially) rescanning for logical storage pool
    // sources so we pick up any new pools if the state of this pool changed.
    syncVolumes();
}

bool
PoolWrap::handleMethod(qmf::AgentSession& session, qmf::AgentEvent& event)
{
    int ret;

    if (*this != event.getDataAddr()) {
        bool handled = false;
        VolumeList::iterator iter = _volumes.begin();

        while (!handled && iter != _volumes.end()) {
            handled = (*iter)->handleMethod(session, event);
        }
        return handled;
    }

    const std::string& methodName(event.getMethodName());
    qpid::types::Variant::Map args(event.getArguments());

    if (methodName == "getXMLDesc") {
        const char *desc = virStoragePoolGetXMLDesc(_pool_ptr, 0);
        if (!desc) {
            std::string err = FORMAT_ERR(_conn, "Error getting XML description of storage pool (virStoragePoolGetXMLDesc).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            event.addReturnArgument("description", desc);
            session.methodSuccess(event);
        }
        return true;
    }

    if (methodName == "create") {
        ret = virStoragePoolCreate(_pool_ptr, 0);
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error creating new storage pool (virStoragePoolCreate).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            update();
            session.methodSuccess(event);
        }
        return true;
    }

    if (methodName == "build") {
        ret = virStoragePoolBuild(_pool_ptr, 0);
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error building storage pool (virStoragePoolBuild).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            update();
            session.methodSuccess(event);
        }
        return true;
    }

    if (methodName == "destroy") {
        ret = virStoragePoolDestroy(_pool_ptr);
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error destroying storage pool (virStoragePoolDestroy).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            update();
            session.methodSuccess(event);
        }
        return true;
    }

    if (methodName == "delete") {
        ret = virStoragePoolDelete(_pool_ptr, 0);
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error deleting storage pool (virStoragePoolDelete).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            update();
            session.methodSuccess(event);
        }
        return true;
    }

    if (methodName == "undefine") {
        ret = virStoragePoolUndefine(_pool_ptr);
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error undefining storage pool (virStoragePoolUndefine).", &ret);
            raiseException(session, event, err, STATUS_USER + ret);
        } else {
            update();
            session.methodSuccess(event);
        }
        return true;
    }

    if (methodName == "createVolumeXML") {
        if (createVolumeXML(session, event)) {
            session.methodSuccess(event);
        }
        return true;
    }

    if (methodName == "refresh") {
        ret = virStoragePoolRefresh(_pool_ptr, 0);
        if (ret < 0) {
            std::string err = FORMAT_ERR(_conn, "Error refreshing storage pool (virStoragePoolRefresh).", &ret);
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

bool
PoolWrap::createVolumeXML(qmf::AgentSession& session, qmf::AgentEvent& event)
{
    int ret;
    virStorageVolPtr volume_ptr;

    qpid::types::Variant::Map args(event.getArguments());

    std::string xmlDesc(args["xmlDesc"]);
    volume_ptr = virStorageVolCreateXML(_pool_ptr, xmlDesc.c_str(), 0);
    if (volume_ptr == NULL) {
        std::string err = FORMAT_ERR(_conn, "Error creating new storage volume from XML description (virStorageVolCreateXML).", &ret);
        raiseException(session, event, err, STATUS_USER + ret);
        return false;
    }

    VolumeWrap *volume;
    try {
        volume = new VolumeWrap(this, volume_ptr, _conn);
        _volumes.push_back(volume);
        event.addReturnArgument("volume", volume->objectID());
    } catch (int i) {
        delete volume;
        std::string err = FORMAT_ERR(_conn, "Error constructing pool object in virStorageVolCreateXML.", &ret);
        raiseException(session, event, err, STATUS_USER + ret);
        return false;
    }

    update();
    return true;
}

