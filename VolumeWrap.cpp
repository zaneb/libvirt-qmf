#include <string.h>
#include <sys/select.h>
#include <errno.h>

#include "NodeWrap.h"
#include "PoolWrap.h"
#include "VolumeWrap.h"

#include "ArgsVolumeXml_desc.h"

VolumeWrap::VolumeWrap(ManagementAgent *agent, PoolWrap *parent, 
                       virStorageVolPtr volume_pointer, virConnectPtr connect)
{
    int ret;
    const char *volume_key_s;
    char *volume_path_s;
    const char *volume_name_s;

    volume_ptr = volume_pointer;
    conn = connect;

    volume_key_s = virStorageVolGetKey(volume_ptr);
    if (volume_key_s == NULL) {
        printf ("Error getting storage volume key\n");
        return;
    }
    volume_key = volume_key_s;

    volume_path_s = virStorageVolGetPath(volume_ptr);
    if (volume_path_s == NULL) {
        printf ("Error getting volume path\n");
        return;
    }
    volume_path = volume_path_s;

    volume_name_s = virStorageVolGetName(volume_ptr);
    if (volume_name_s == NULL) {
        printf ("Error getting volume name\n");
        return;
    }
    volume_name = volume_name_s;

    volume = new Volume(agent, this, parent, volume_key, volume_path, volume_name);
    agent->addObject(volume);
}

void
VolumeWrap::update()
{
    virStorageVolInfo info;
    int ret;

    printf("Updating volume info\n");

    ret = virStorageVolGetInfo(volume_ptr, &info);
    if (ret < 0) {
        printf("VolumeWrap: Unable to get info of storage volume info\n");
        return;
    }
    volume->set_capacity(info.capacity);
    volume->set_allocation(info.allocation);
}

VolumeWrap::~VolumeWrap()
{
    volume->resourceDestroy();
    virStorageVolFree(volume_ptr);
}

Manageable::status_t
VolumeWrap::ManagementMethod(uint32_t methodId, Args& args)
{
    Mutex::ScopedLock _lock(vectorLock);
    cout << "Method Received: " << methodId << endl;
    int ret;

    switch (methodId) {
        case Volume::METHOD_XML_DESC:
        {
            ArgsVolumeXml_desc *io_args = (ArgsVolumeXml_desc *) &args;
            char *desc;

            desc = virStorageVolGetXMLDesc(volume_ptr, VIR_DOMAIN_XML_SECURE | VIR_DOMAIN_XML_INACTIVE);
            if (desc) {
                io_args->o_description = desc;
            } else {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}


