#ifndef VOLUME_WRAP_H
#define VOLUME_WRAP_H

#include "PoolWrap.h"

#include <unistd.h>
#include <cstdlib>
#include <iostream>

#include <sstream>

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>


class VolumeWrap:
    PackageOwner<PoolWrap::PackageDefinition>,
    public ManagedObject
{
    virStorageVolPtr _volume_ptr;
    virConnectPtr _conn;

    std::string _volume_name;
    std::string _volume_key;
    std::string _volume_path;

    std::string _lvm_name;
    bool _has_lvm_child;

    PoolWrap *_wrap_parent;

    void checkForLVMPool();

public:
    VolumeWrap(PoolWrap *parent,
               virStorageVolPtr volume_ptr,
               virConnectPtr connect);
    ~VolumeWrap();

    const char *name(void) { return _volume_name.c_str(); }

    void update();

    bool handleMethod(qmf::AgentSession& session, qmf::AgentEvent& event);
};

#endif
