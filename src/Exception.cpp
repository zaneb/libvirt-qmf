#include "Exception.h"
#include "Error.h"
#include <qmf/Schema.h>
#include <qmf/SchemaProperty.h>
#include <qmf/Data.h>


#define ERROR_TEXT "error_text"
#define ERROR_CODE "error_code"


static qmf::Schema errorSchema(qmf::SCHEMA_TYPE_DATA,
        "org.libvirt", "error");


void
initErrorSchema(qmf::AgentSession& session)
{
    qmf::SchemaProperty status_text(ERROR_TEXT, qmf::SCHEMA_DATA_STRING);
    qmf::SchemaProperty status_code(ERROR_CODE, qmf::SCHEMA_DATA_INT);

    errorSchema.addProperty(status_text);
    errorSchema.addProperty(status_code);

    session.registerSchema(errorSchema);
}

void
raiseException(qmf::AgentSession& session,
               qmf::AgentEvent& event,
               const std::string& error_text,
               unsigned int error_code)
{
    qmf::Data response(errorSchema);
    response.setProperty(ERROR_TEXT, error_text);
    response.setProperty(ERROR_CODE, error_code);
    session.raiseException(event, response);
}
