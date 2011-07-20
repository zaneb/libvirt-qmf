#ifndef LIBVIRT_AGENT_H
#define LIBVIRT_AGENT_H

#include <matahari/agent.h>
#include "QmfPackage.h"
#include "ManagedObject.h"


class NodeWrap;


class LibvirtAgent:
    public MatahariAgent,
    public PackageOwner<qmf::org::libvirt::PackageDefinition>
{
public:
    ~LibvirtAgent();

    int setup(qmf::AgentSession session);
    gboolean invoke(qmf::AgentSession session, qmf::AgentEvent event,
		    gpointer user_data);

    PackageDefinition& package(void) { return _package; }
    void addData(qmf::Data& data);
    void delData(qmf::Data& data);

    void updateData(void);

private:
    PackageDefinition _package;
    NodeWrap *_node;
    int _timer;
};

#endif

