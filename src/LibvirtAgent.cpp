#include "LibvirtAgent.h"
#include "NodeWrap.h"
#include "Exception.h"

#include <syslog.h>


#define POLL_TIME_s 3


static gboolean handleTimer(gpointer user_data)
{
    LibvirtAgent *agent = (LibvirtAgent *)user_data;
    if (agent) {
        agent->updateData();
        return TRUE;
    }

    return FALSE;
}

int
LibvirtAgent::setup(qmf::AgentSession session)
{
    _package.configure(session);
    initErrorSchema(session);

    _node = new NodeWrap(this);

    _timer = g_timeout_add_seconds(POLL_TIME_s, &handleTimer, this);

    return 0;
}

LibvirtAgent::~LibvirtAgent()
{
    if (_timer) {
        g_source_remove(_timer);
    }
    delete _node;
}

void
LibvirtAgent::addData(qmf::Data& data)
{
    getSession().addData(data);
}

void
LibvirtAgent::delData(qmf::Data& data)
{
    getSession().delData(data.getAddr());
}

gboolean
LibvirtAgent::invoke(qmf::AgentSession session, qmf::AgentEvent event,
                     gpointer user_data)
{
    if (event.getType() != qmf::AGENT_METHOD) {
        return TRUE;
    }

    bool handled = _node->handleMethod(session, event);
    if (!handled) {
        raiseException(session, event,
                       ERROR_UNKNOWN_OBJECT, STATUS_UNKNOWN_OBJECT);
    }

    return TRUE;
}

void
LibvirtAgent::updateData(void)
{
    if (_node) {
        _node->poll();
    }
}

int
main(int argc, char **argv)
{
    LibvirtAgent agent;

    int rc = agent.init(argc, argv, "libvirt-qmf");

    openlog("libvirt-qmf", 0, LOG_DAEMON);

    // This prevents us from dying if libvirt disconnects.
    signal(SIGPIPE, SIG_IGN);

    if (rc == 0) {
        agent.run();
    }
    return rc;
}

