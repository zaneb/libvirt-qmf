#ifndef MANAGED_OBJECT_H
#define MANAGED_OBJECT_H

#include <qmf/AgentEvent.h>
#include <qmf/AgentSession.h>
#include <qmf/Data.h>
#include <qmf/DataAddr.h>

#include <assert.h>


template <class T>
class PackageOwner
{
    PackageOwner<T> *_parent;

public:
    typedef T PackageDefinition;

protected:
    PackageOwner<T>(void): _parent(NULL) { }
    PackageOwner<T>(PackageOwner<T> *parent): _parent(parent) { }
    virtual ~PackageOwner<T>() { }

    virtual PackageDefinition& package(void) {
        assert(_parent != NULL);
        return _parent->package();
    }

    virtual void addData(qmf::Data& data) {
        assert(_parent != NULL);
        _parent->addData(data);
    }

    virtual void delData(qmf::Data& data) {
        assert(_parent != NULL);
        _parent->delData(data);
    }
};


class ManagedObject
{
public:
    qpid::types::Variant objectID(void) {
        assert(_data.hasAddr());
        return _data.getAddr().asMap();
    }

    virtual bool handleMethod(qmf::AgentSession& session,
                              qmf::AgentEvent& event) = 0;

protected:
    ManagedObject(qmf::Schema& schema): _data(schema) { }
    virtual ~ManagedObject() { }

    bool operator==(const qmf::DataAddr& addr) {
        if (!_data.hasAddr()) {
            return false;
        }

        // Due to a bug (#QPID3344) in current versions of libqmf2, the first
        // operand to qmf::DataAddr::operator==() must not be const, otherwise
        // a bogus value is returned.
        qmf::DataAddr& mutableAddr = const_cast<qmf::DataAddr&>(_data.getAddr());

        return mutableAddr == addr;
    }

    bool operator!=(const qmf::DataAddr& addr) {
        return !(*this == addr);
    }

    qmf::Data _data;
};

#endif

