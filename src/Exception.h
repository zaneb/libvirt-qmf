#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <qmf/AgentSession.h>
#include <qmf/AgentEvent.h>
#include <string>


#define STATUS_OK               0
#define STATUS_UNKNOWN_OBJECT   1
#define STATUS_UNKNOWN_METHOD   2
#define STATUS_NOT_IMPLEMENTED  3
#define STATUS_EXCEPTION        7
#define STATUS_USER             0x10000

#define ERROR_UNKNOWN_OBJECT   "Unknown Object"
#define ERROR_UNKNOWN_METHOD   "Unknown Method"
#define ERROR_NOT_IMPLEMENTED  "Not implemented"


void
initErrorSchema(qmf::AgentSession& session);

void
raiseException(qmf::AgentSession& session,
               qmf::AgentEvent& event,
               const std::string& error_text,
               unsigned int error_code);

#endif
