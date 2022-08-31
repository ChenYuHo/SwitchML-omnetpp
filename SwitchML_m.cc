//
// Generated file, do not edit! Created by opp_msgtool 6.0 from SwitchML.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "SwitchML_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

Register_Enum(SwitchMLPacketType, (SwitchMLPacketType::SWITCHML_CONN_REQ, SwitchMLPacketType::SWITCHML_CONN_ACK, SwitchMLPacketType::SWITCHML_DISC_REQ, SwitchMLPacketType::SWITCHML_DISC_ACK, SwitchMLPacketType::SWITCHML_DATA));

Register_Class(SwitchMLPacket)

SwitchMLPacket::SwitchMLPacket(const char *name, short kind) : ::omnetpp::cPacket(name, kind)
{
}

SwitchMLPacket::SwitchMLPacket(const SwitchMLPacket& other) : ::omnetpp::cPacket(other)
{
    copy(other);
}

SwitchMLPacket::~SwitchMLPacket()
{
}

SwitchMLPacket& SwitchMLPacket::operator=(const SwitchMLPacket& other)
{
    if (this == &other) return *this;
    ::omnetpp::cPacket::operator=(other);
    copy(other);
    return *this;
}

void SwitchMLPacket::copy(const SwitchMLPacket& other)
{
    this->from_id = other.from_id;
    this->slot = other.slot;
    this->ver = other.ver;
    this->offset = other.offset;
    this->tensor_key = other.tensor_key;
    this->n_workers = other.n_workers;
    this->layer = other.layer;
    this->job_id = other.job_id;
    this->num_pkts_expected = other.num_pkts_expected;
    this->grad_size = other.grad_size;
    this->num_chunks = other.num_chunks;
    this->chunk_id = other.chunk_id;
    this->upward = other.upward;
}

void SwitchMLPacket::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cPacket::parsimPack(b);
    doParsimPacking(b,this->from_id);
    doParsimPacking(b,this->slot);
    doParsimPacking(b,this->ver);
    doParsimPacking(b,this->offset);
    doParsimPacking(b,this->tensor_key);
    doParsimPacking(b,this->n_workers);
    doParsimPacking(b,this->layer);
    doParsimPacking(b,this->job_id);
    doParsimPacking(b,this->num_pkts_expected);
    doParsimPacking(b,this->grad_size);
    doParsimPacking(b,this->num_chunks);
    doParsimPacking(b,this->chunk_id);
    doParsimPacking(b,this->upward);
}

void SwitchMLPacket::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cPacket::parsimUnpack(b);
    doParsimUnpacking(b,this->from_id);
    doParsimUnpacking(b,this->slot);
    doParsimUnpacking(b,this->ver);
    doParsimUnpacking(b,this->offset);
    doParsimUnpacking(b,this->tensor_key);
    doParsimUnpacking(b,this->n_workers);
    doParsimUnpacking(b,this->layer);
    doParsimUnpacking(b,this->job_id);
    doParsimUnpacking(b,this->num_pkts_expected);
    doParsimUnpacking(b,this->grad_size);
    doParsimUnpacking(b,this->num_chunks);
    doParsimUnpacking(b,this->chunk_id);
    doParsimUnpacking(b,this->upward);
}

int SwitchMLPacket::getFrom_id() const
{
    return this->from_id;
}

void SwitchMLPacket::setFrom_id(int from_id)
{
    this->from_id = from_id;
}

uint32_t SwitchMLPacket::getSlot() const
{
    return this->slot;
}

void SwitchMLPacket::setSlot(uint32_t slot)
{
    this->slot = slot;
}

uint32_t SwitchMLPacket::getVer() const
{
    return this->ver;
}

void SwitchMLPacket::setVer(uint32_t ver)
{
    this->ver = ver;
}

uint32_t SwitchMLPacket::getOffset() const
{
    return this->offset;
}

void SwitchMLPacket::setOffset(uint32_t offset)
{
    this->offset = offset;
}

uint32_t SwitchMLPacket::getTensor_key() const
{
    return this->tensor_key;
}

void SwitchMLPacket::setTensor_key(uint32_t tensor_key)
{
    this->tensor_key = tensor_key;
}

uint32_t SwitchMLPacket::getN_workers() const
{
    return this->n_workers;
}

void SwitchMLPacket::setN_workers(uint32_t n_workers)
{
    this->n_workers = n_workers;
}

uint64_t SwitchMLPacket::getLayer() const
{
    return this->layer;
}

void SwitchMLPacket::setLayer(uint64_t layer)
{
    this->layer = layer;
}

uint64_t SwitchMLPacket::getJob_id() const
{
    return this->job_id;
}

void SwitchMLPacket::setJob_id(uint64_t job_id)
{
    this->job_id = job_id;
}

uint64_t SwitchMLPacket::getNum_pkts_expected() const
{
    return this->num_pkts_expected;
}

void SwitchMLPacket::setNum_pkts_expected(uint64_t num_pkts_expected)
{
    this->num_pkts_expected = num_pkts_expected;
}

uint64_t SwitchMLPacket::getGrad_size() const
{
    return this->grad_size;
}

void SwitchMLPacket::setGrad_size(uint64_t grad_size)
{
    this->grad_size = grad_size;
}

uint64_t SwitchMLPacket::getNum_chunks() const
{
    return this->num_chunks;
}

void SwitchMLPacket::setNum_chunks(uint64_t num_chunks)
{
    this->num_chunks = num_chunks;
}

uint64_t SwitchMLPacket::getChunk_id() const
{
    return this->chunk_id;
}

void SwitchMLPacket::setChunk_id(uint64_t chunk_id)
{
    this->chunk_id = chunk_id;
}

bool SwitchMLPacket::getUpward() const
{
    return this->upward;
}

void SwitchMLPacket::setUpward(bool upward)
{
    this->upward = upward;
}

class SwitchMLPacketDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_from_id,
        FIELD_slot,
        FIELD_ver,
        FIELD_offset,
        FIELD_tensor_key,
        FIELD_n_workers,
        FIELD_layer,
        FIELD_job_id,
        FIELD_num_pkts_expected,
        FIELD_grad_size,
        FIELD_num_chunks,
        FIELD_chunk_id,
        FIELD_upward,
    };
  public:
    SwitchMLPacketDescriptor();
    virtual ~SwitchMLPacketDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(SwitchMLPacketDescriptor)

SwitchMLPacketDescriptor::SwitchMLPacketDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(SwitchMLPacket)), "omnetpp::cPacket")
{
    propertyNames = nullptr;
}

SwitchMLPacketDescriptor::~SwitchMLPacketDescriptor()
{
    delete[] propertyNames;
}

bool SwitchMLPacketDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<SwitchMLPacket *>(obj)!=nullptr;
}

const char **SwitchMLPacketDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *SwitchMLPacketDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int SwitchMLPacketDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 13+base->getFieldCount() : 13;
}

unsigned int SwitchMLPacketDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_from_id
        FD_ISEDITABLE,    // FIELD_slot
        FD_ISEDITABLE,    // FIELD_ver
        FD_ISEDITABLE,    // FIELD_offset
        FD_ISEDITABLE,    // FIELD_tensor_key
        FD_ISEDITABLE,    // FIELD_n_workers
        FD_ISEDITABLE,    // FIELD_layer
        FD_ISEDITABLE,    // FIELD_job_id
        FD_ISEDITABLE,    // FIELD_num_pkts_expected
        FD_ISEDITABLE,    // FIELD_grad_size
        FD_ISEDITABLE,    // FIELD_num_chunks
        FD_ISEDITABLE,    // FIELD_chunk_id
        FD_ISEDITABLE,    // FIELD_upward
    };
    return (field >= 0 && field < 13) ? fieldTypeFlags[field] : 0;
}

const char *SwitchMLPacketDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "from_id",
        "slot",
        "ver",
        "offset",
        "tensor_key",
        "n_workers",
        "layer",
        "job_id",
        "num_pkts_expected",
        "grad_size",
        "num_chunks",
        "chunk_id",
        "upward",
    };
    return (field >= 0 && field < 13) ? fieldNames[field] : nullptr;
}

int SwitchMLPacketDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "from_id") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "slot") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "ver") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "offset") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "tensor_key") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "n_workers") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "layer") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "job_id") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "num_pkts_expected") == 0) return baseIndex + 8;
    if (strcmp(fieldName, "grad_size") == 0) return baseIndex + 9;
    if (strcmp(fieldName, "num_chunks") == 0) return baseIndex + 10;
    if (strcmp(fieldName, "chunk_id") == 0) return baseIndex + 11;
    if (strcmp(fieldName, "upward") == 0) return baseIndex + 12;
    return base ? base->findField(fieldName) : -1;
}

const char *SwitchMLPacketDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_from_id
        "uint32_t",    // FIELD_slot
        "uint32_t",    // FIELD_ver
        "uint32_t",    // FIELD_offset
        "uint32_t",    // FIELD_tensor_key
        "uint32_t",    // FIELD_n_workers
        "uint64_t",    // FIELD_layer
        "uint64_t",    // FIELD_job_id
        "uint64_t",    // FIELD_num_pkts_expected
        "uint64_t",    // FIELD_grad_size
        "uint64_t",    // FIELD_num_chunks
        "uint64_t",    // FIELD_chunk_id
        "bool",    // FIELD_upward
    };
    return (field >= 0 && field < 13) ? fieldTypeStrings[field] : nullptr;
}

const char **SwitchMLPacketDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *SwitchMLPacketDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int SwitchMLPacketDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    SwitchMLPacket *pp = omnetpp::fromAnyPtr<SwitchMLPacket>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void SwitchMLPacketDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    SwitchMLPacket *pp = omnetpp::fromAnyPtr<SwitchMLPacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'SwitchMLPacket'", field);
    }
}

const char *SwitchMLPacketDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    SwitchMLPacket *pp = omnetpp::fromAnyPtr<SwitchMLPacket>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string SwitchMLPacketDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    SwitchMLPacket *pp = omnetpp::fromAnyPtr<SwitchMLPacket>(object); (void)pp;
    switch (field) {
        case FIELD_from_id: return long2string(pp->getFrom_id());
        case FIELD_slot: return ulong2string(pp->getSlot());
        case FIELD_ver: return ulong2string(pp->getVer());
        case FIELD_offset: return ulong2string(pp->getOffset());
        case FIELD_tensor_key: return ulong2string(pp->getTensor_key());
        case FIELD_n_workers: return ulong2string(pp->getN_workers());
        case FIELD_layer: return uint642string(pp->getLayer());
        case FIELD_job_id: return uint642string(pp->getJob_id());
        case FIELD_num_pkts_expected: return uint642string(pp->getNum_pkts_expected());
        case FIELD_grad_size: return uint642string(pp->getGrad_size());
        case FIELD_num_chunks: return uint642string(pp->getNum_chunks());
        case FIELD_chunk_id: return uint642string(pp->getChunk_id());
        case FIELD_upward: return bool2string(pp->getUpward());
        default: return "";
    }
}

void SwitchMLPacketDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    SwitchMLPacket *pp = omnetpp::fromAnyPtr<SwitchMLPacket>(object); (void)pp;
    switch (field) {
        case FIELD_from_id: pp->setFrom_id(string2long(value)); break;
        case FIELD_slot: pp->setSlot(string2ulong(value)); break;
        case FIELD_ver: pp->setVer(string2ulong(value)); break;
        case FIELD_offset: pp->setOffset(string2ulong(value)); break;
        case FIELD_tensor_key: pp->setTensor_key(string2ulong(value)); break;
        case FIELD_n_workers: pp->setN_workers(string2ulong(value)); break;
        case FIELD_layer: pp->setLayer(string2uint64(value)); break;
        case FIELD_job_id: pp->setJob_id(string2uint64(value)); break;
        case FIELD_num_pkts_expected: pp->setNum_pkts_expected(string2uint64(value)); break;
        case FIELD_grad_size: pp->setGrad_size(string2uint64(value)); break;
        case FIELD_num_chunks: pp->setNum_chunks(string2uint64(value)); break;
        case FIELD_chunk_id: pp->setChunk_id(string2uint64(value)); break;
        case FIELD_upward: pp->setUpward(string2bool(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'SwitchMLPacket'", field);
    }
}

omnetpp::cValue SwitchMLPacketDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    SwitchMLPacket *pp = omnetpp::fromAnyPtr<SwitchMLPacket>(object); (void)pp;
    switch (field) {
        case FIELD_from_id: return pp->getFrom_id();
        case FIELD_slot: return (omnetpp::intval_t)(pp->getSlot());
        case FIELD_ver: return (omnetpp::intval_t)(pp->getVer());
        case FIELD_offset: return (omnetpp::intval_t)(pp->getOffset());
        case FIELD_tensor_key: return (omnetpp::intval_t)(pp->getTensor_key());
        case FIELD_n_workers: return (omnetpp::intval_t)(pp->getN_workers());
        case FIELD_layer: return (omnetpp::intval_t)(pp->getLayer());
        case FIELD_job_id: return (omnetpp::intval_t)(pp->getJob_id());
        case FIELD_num_pkts_expected: return (omnetpp::intval_t)(pp->getNum_pkts_expected());
        case FIELD_grad_size: return (omnetpp::intval_t)(pp->getGrad_size());
        case FIELD_num_chunks: return (omnetpp::intval_t)(pp->getNum_chunks());
        case FIELD_chunk_id: return (omnetpp::intval_t)(pp->getChunk_id());
        case FIELD_upward: return pp->getUpward();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'SwitchMLPacket' as cValue -- field index out of range?", field);
    }
}

void SwitchMLPacketDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    SwitchMLPacket *pp = omnetpp::fromAnyPtr<SwitchMLPacket>(object); (void)pp;
    switch (field) {
        case FIELD_from_id: pp->setFrom_id(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_slot: pp->setSlot(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_ver: pp->setVer(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_offset: pp->setOffset(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_tensor_key: pp->setTensor_key(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_n_workers: pp->setN_workers(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_layer: pp->setLayer(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_job_id: pp->setJob_id(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_num_pkts_expected: pp->setNum_pkts_expected(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_grad_size: pp->setGrad_size(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_num_chunks: pp->setNum_chunks(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_chunk_id: pp->setChunk_id(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_upward: pp->setUpward(value.boolValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'SwitchMLPacket'", field);
    }
}

const char *SwitchMLPacketDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr SwitchMLPacketDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    SwitchMLPacket *pp = omnetpp::fromAnyPtr<SwitchMLPacket>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void SwitchMLPacketDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    SwitchMLPacket *pp = omnetpp::fromAnyPtr<SwitchMLPacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'SwitchMLPacket'", field);
    }
}

Register_Class(LayerAck)

LayerAck::LayerAck(const char *name, short kind) : ::omnetpp::cMessage(name, kind)
{
}

LayerAck::LayerAck(const LayerAck& other) : ::omnetpp::cMessage(other)
{
    copy(other);
}

LayerAck::~LayerAck()
{
}

LayerAck& LayerAck::operator=(const LayerAck& other)
{
    if (this == &other) return *this;
    ::omnetpp::cMessage::operator=(other);
    copy(other);
    return *this;
}

void LayerAck::copy(const LayerAck& other)
{
    this->layer = other.layer;
    this->weight_update_time = other.weight_update_time;
    this->completed = other.completed;
}

void LayerAck::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cMessage::parsimPack(b);
    doParsimPacking(b,this->layer);
    doParsimPacking(b,this->weight_update_time);
    doParsimPacking(b,this->completed);
}

void LayerAck::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cMessage::parsimUnpack(b);
    doParsimUnpacking(b,this->layer);
    doParsimUnpacking(b,this->weight_update_time);
    doParsimUnpacking(b,this->completed);
}

uint64_t LayerAck::getLayer() const
{
    return this->layer;
}

void LayerAck::setLayer(uint64_t layer)
{
    this->layer = layer;
}

omnetpp::simtime_t LayerAck::getWeight_update_time() const
{
    return this->weight_update_time;
}

void LayerAck::setWeight_update_time(omnetpp::simtime_t weight_update_time)
{
    this->weight_update_time = weight_update_time;
}

bool LayerAck::getCompleted() const
{
    return this->completed;
}

void LayerAck::setCompleted(bool completed)
{
    this->completed = completed;
}

class LayerAckDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_layer,
        FIELD_weight_update_time,
        FIELD_completed,
    };
  public:
    LayerAckDescriptor();
    virtual ~LayerAckDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(LayerAckDescriptor)

LayerAckDescriptor::LayerAckDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(LayerAck)), "omnetpp::cMessage")
{
    propertyNames = nullptr;
}

LayerAckDescriptor::~LayerAckDescriptor()
{
    delete[] propertyNames;
}

bool LayerAckDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<LayerAck *>(obj)!=nullptr;
}

const char **LayerAckDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *LayerAckDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int LayerAckDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 3+base->getFieldCount() : 3;
}

unsigned int LayerAckDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_layer
        FD_ISEDITABLE,    // FIELD_weight_update_time
        FD_ISEDITABLE,    // FIELD_completed
    };
    return (field >= 0 && field < 3) ? fieldTypeFlags[field] : 0;
}

const char *LayerAckDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "layer",
        "weight_update_time",
        "completed",
    };
    return (field >= 0 && field < 3) ? fieldNames[field] : nullptr;
}

int LayerAckDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "layer") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "weight_update_time") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "completed") == 0) return baseIndex + 2;
    return base ? base->findField(fieldName) : -1;
}

const char *LayerAckDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "uint64_t",    // FIELD_layer
        "omnetpp::simtime_t",    // FIELD_weight_update_time
        "bool",    // FIELD_completed
    };
    return (field >= 0 && field < 3) ? fieldTypeStrings[field] : nullptr;
}

const char **LayerAckDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *LayerAckDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int LayerAckDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    LayerAck *pp = omnetpp::fromAnyPtr<LayerAck>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void LayerAckDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    LayerAck *pp = omnetpp::fromAnyPtr<LayerAck>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'LayerAck'", field);
    }
}

const char *LayerAckDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    LayerAck *pp = omnetpp::fromAnyPtr<LayerAck>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string LayerAckDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    LayerAck *pp = omnetpp::fromAnyPtr<LayerAck>(object); (void)pp;
    switch (field) {
        case FIELD_layer: return uint642string(pp->getLayer());
        case FIELD_weight_update_time: return simtime2string(pp->getWeight_update_time());
        case FIELD_completed: return bool2string(pp->getCompleted());
        default: return "";
    }
}

void LayerAckDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    LayerAck *pp = omnetpp::fromAnyPtr<LayerAck>(object); (void)pp;
    switch (field) {
        case FIELD_layer: pp->setLayer(string2uint64(value)); break;
        case FIELD_weight_update_time: pp->setWeight_update_time(string2simtime(value)); break;
        case FIELD_completed: pp->setCompleted(string2bool(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LayerAck'", field);
    }
}

omnetpp::cValue LayerAckDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    LayerAck *pp = omnetpp::fromAnyPtr<LayerAck>(object); (void)pp;
    switch (field) {
        case FIELD_layer: return (omnetpp::intval_t)(pp->getLayer());
        case FIELD_weight_update_time: return pp->getWeight_update_time().dbl();
        case FIELD_completed: return pp->getCompleted();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'LayerAck' as cValue -- field index out of range?", field);
    }
}

void LayerAckDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    LayerAck *pp = omnetpp::fromAnyPtr<LayerAck>(object); (void)pp;
    switch (field) {
        case FIELD_layer: pp->setLayer(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_weight_update_time: pp->setWeight_update_time(value.doubleValue()); break;
        case FIELD_completed: pp->setCompleted(value.boolValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LayerAck'", field);
    }
}

const char *LayerAckDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr LayerAckDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    LayerAck *pp = omnetpp::fromAnyPtr<LayerAck>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void LayerAckDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    LayerAck *pp = omnetpp::fromAnyPtr<LayerAck>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LayerAck'", field);
    }
}

Register_Class(AllreduceRequest)

AllreduceRequest::AllreduceRequest(const char *name, short kind) : ::omnetpp::cMessage(name, kind)
{
}

AllreduceRequest::AllreduceRequest(const AllreduceRequest& other) : ::omnetpp::cMessage(other)
{
    copy(other);
}

AllreduceRequest::~AllreduceRequest()
{
}

AllreduceRequest& AllreduceRequest::operator=(const AllreduceRequest& other)
{
    if (this == &other) return *this;
    ::omnetpp::cMessage::operator=(other);
    copy(other);
    return *this;
}

void AllreduceRequest::copy(const AllreduceRequest& other)
{
    this->allreducer_id = other.allreducer_id;
    this->training_process_id = other.training_process_id;
    this->worker_id = other.worker_id;
    this->size = other.size;
    this->rank = other.rank;
    this->layer = other.layer;
    this->tensor_key = other.tensor_key;
    this->job_id = other.job_id;
    this->num_workers_allocated = other.num_workers_allocated;
    this->num_chunks = other.num_chunks;
    this->chunk_id = other.chunk_id;
}

void AllreduceRequest::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cMessage::parsimPack(b);
    doParsimPacking(b,this->allreducer_id);
    doParsimPacking(b,this->training_process_id);
    doParsimPacking(b,this->worker_id);
    doParsimPacking(b,this->size);
    doParsimPacking(b,this->rank);
    doParsimPacking(b,this->layer);
    doParsimPacking(b,this->tensor_key);
    doParsimPacking(b,this->job_id);
    doParsimPacking(b,this->num_workers_allocated);
    doParsimPacking(b,this->num_chunks);
    doParsimPacking(b,this->chunk_id);
}

void AllreduceRequest::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cMessage::parsimUnpack(b);
    doParsimUnpacking(b,this->allreducer_id);
    doParsimUnpacking(b,this->training_process_id);
    doParsimUnpacking(b,this->worker_id);
    doParsimUnpacking(b,this->size);
    doParsimUnpacking(b,this->rank);
    doParsimUnpacking(b,this->layer);
    doParsimUnpacking(b,this->tensor_key);
    doParsimUnpacking(b,this->job_id);
    doParsimUnpacking(b,this->num_workers_allocated);
    doParsimUnpacking(b,this->num_chunks);
    doParsimUnpacking(b,this->chunk_id);
}

int AllreduceRequest::getAllreducer_id() const
{
    return this->allreducer_id;
}

void AllreduceRequest::setAllreducer_id(int allreducer_id)
{
    this->allreducer_id = allreducer_id;
}

int AllreduceRequest::getTraining_process_id() const
{
    return this->training_process_id;
}

void AllreduceRequest::setTraining_process_id(int training_process_id)
{
    this->training_process_id = training_process_id;
}

int AllreduceRequest::getWorker_id() const
{
    return this->worker_id;
}

void AllreduceRequest::setWorker_id(int worker_id)
{
    this->worker_id = worker_id;
}

uint64_t AllreduceRequest::getSize() const
{
    return this->size;
}

void AllreduceRequest::setSize(uint64_t size)
{
    this->size = size;
}

uint64_t AllreduceRequest::getRank() const
{
    return this->rank;
}

void AllreduceRequest::setRank(uint64_t rank)
{
    this->rank = rank;
}

uint64_t AllreduceRequest::getLayer() const
{
    return this->layer;
}

void AllreduceRequest::setLayer(uint64_t layer)
{
    this->layer = layer;
}

uint64_t AllreduceRequest::getTensor_key() const
{
    return this->tensor_key;
}

void AllreduceRequest::setTensor_key(uint64_t tensor_key)
{
    this->tensor_key = tensor_key;
}

uint64_t AllreduceRequest::getJob_id() const
{
    return this->job_id;
}

void AllreduceRequest::setJob_id(uint64_t job_id)
{
    this->job_id = job_id;
}

uint64_t AllreduceRequest::getNum_workers_allocated() const
{
    return this->num_workers_allocated;
}

void AllreduceRequest::setNum_workers_allocated(uint64_t num_workers_allocated)
{
    this->num_workers_allocated = num_workers_allocated;
}

uint64_t AllreduceRequest::getNum_chunks() const
{
    return this->num_chunks;
}

void AllreduceRequest::setNum_chunks(uint64_t num_chunks)
{
    this->num_chunks = num_chunks;
}

uint64_t AllreduceRequest::getChunk_id() const
{
    return this->chunk_id;
}

void AllreduceRequest::setChunk_id(uint64_t chunk_id)
{
    this->chunk_id = chunk_id;
}

class AllreduceRequestDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_allreducer_id,
        FIELD_training_process_id,
        FIELD_worker_id,
        FIELD_size,
        FIELD_rank,
        FIELD_layer,
        FIELD_tensor_key,
        FIELD_job_id,
        FIELD_num_workers_allocated,
        FIELD_num_chunks,
        FIELD_chunk_id,
    };
  public:
    AllreduceRequestDescriptor();
    virtual ~AllreduceRequestDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(AllreduceRequestDescriptor)

AllreduceRequestDescriptor::AllreduceRequestDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(AllreduceRequest)), "omnetpp::cMessage")
{
    propertyNames = nullptr;
}

AllreduceRequestDescriptor::~AllreduceRequestDescriptor()
{
    delete[] propertyNames;
}

bool AllreduceRequestDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<AllreduceRequest *>(obj)!=nullptr;
}

const char **AllreduceRequestDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *AllreduceRequestDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int AllreduceRequestDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 11+base->getFieldCount() : 11;
}

unsigned int AllreduceRequestDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_allreducer_id
        FD_ISEDITABLE,    // FIELD_training_process_id
        FD_ISEDITABLE,    // FIELD_worker_id
        FD_ISEDITABLE,    // FIELD_size
        FD_ISEDITABLE,    // FIELD_rank
        FD_ISEDITABLE,    // FIELD_layer
        FD_ISEDITABLE,    // FIELD_tensor_key
        FD_ISEDITABLE,    // FIELD_job_id
        FD_ISEDITABLE,    // FIELD_num_workers_allocated
        FD_ISEDITABLE,    // FIELD_num_chunks
        FD_ISEDITABLE,    // FIELD_chunk_id
    };
    return (field >= 0 && field < 11) ? fieldTypeFlags[field] : 0;
}

const char *AllreduceRequestDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "allreducer_id",
        "training_process_id",
        "worker_id",
        "size",
        "rank",
        "layer",
        "tensor_key",
        "job_id",
        "num_workers_allocated",
        "num_chunks",
        "chunk_id",
    };
    return (field >= 0 && field < 11) ? fieldNames[field] : nullptr;
}

int AllreduceRequestDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "allreducer_id") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "training_process_id") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "worker_id") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "size") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "rank") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "layer") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "tensor_key") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "job_id") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "num_workers_allocated") == 0) return baseIndex + 8;
    if (strcmp(fieldName, "num_chunks") == 0) return baseIndex + 9;
    if (strcmp(fieldName, "chunk_id") == 0) return baseIndex + 10;
    return base ? base->findField(fieldName) : -1;
}

const char *AllreduceRequestDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_allreducer_id
        "int",    // FIELD_training_process_id
        "int",    // FIELD_worker_id
        "uint64_t",    // FIELD_size
        "uint64_t",    // FIELD_rank
        "uint64_t",    // FIELD_layer
        "uint64_t",    // FIELD_tensor_key
        "uint64_t",    // FIELD_job_id
        "uint64_t",    // FIELD_num_workers_allocated
        "uint64_t",    // FIELD_num_chunks
        "uint64_t",    // FIELD_chunk_id
    };
    return (field >= 0 && field < 11) ? fieldTypeStrings[field] : nullptr;
}

const char **AllreduceRequestDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *AllreduceRequestDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int AllreduceRequestDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    AllreduceRequest *pp = omnetpp::fromAnyPtr<AllreduceRequest>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void AllreduceRequestDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    AllreduceRequest *pp = omnetpp::fromAnyPtr<AllreduceRequest>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'AllreduceRequest'", field);
    }
}

const char *AllreduceRequestDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    AllreduceRequest *pp = omnetpp::fromAnyPtr<AllreduceRequest>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string AllreduceRequestDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    AllreduceRequest *pp = omnetpp::fromAnyPtr<AllreduceRequest>(object); (void)pp;
    switch (field) {
        case FIELD_allreducer_id: return long2string(pp->getAllreducer_id());
        case FIELD_training_process_id: return long2string(pp->getTraining_process_id());
        case FIELD_worker_id: return long2string(pp->getWorker_id());
        case FIELD_size: return uint642string(pp->getSize());
        case FIELD_rank: return uint642string(pp->getRank());
        case FIELD_layer: return uint642string(pp->getLayer());
        case FIELD_tensor_key: return uint642string(pp->getTensor_key());
        case FIELD_job_id: return uint642string(pp->getJob_id());
        case FIELD_num_workers_allocated: return uint642string(pp->getNum_workers_allocated());
        case FIELD_num_chunks: return uint642string(pp->getNum_chunks());
        case FIELD_chunk_id: return uint642string(pp->getChunk_id());
        default: return "";
    }
}

void AllreduceRequestDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    AllreduceRequest *pp = omnetpp::fromAnyPtr<AllreduceRequest>(object); (void)pp;
    switch (field) {
        case FIELD_allreducer_id: pp->setAllreducer_id(string2long(value)); break;
        case FIELD_training_process_id: pp->setTraining_process_id(string2long(value)); break;
        case FIELD_worker_id: pp->setWorker_id(string2long(value)); break;
        case FIELD_size: pp->setSize(string2uint64(value)); break;
        case FIELD_rank: pp->setRank(string2uint64(value)); break;
        case FIELD_layer: pp->setLayer(string2uint64(value)); break;
        case FIELD_tensor_key: pp->setTensor_key(string2uint64(value)); break;
        case FIELD_job_id: pp->setJob_id(string2uint64(value)); break;
        case FIELD_num_workers_allocated: pp->setNum_workers_allocated(string2uint64(value)); break;
        case FIELD_num_chunks: pp->setNum_chunks(string2uint64(value)); break;
        case FIELD_chunk_id: pp->setChunk_id(string2uint64(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'AllreduceRequest'", field);
    }
}

omnetpp::cValue AllreduceRequestDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    AllreduceRequest *pp = omnetpp::fromAnyPtr<AllreduceRequest>(object); (void)pp;
    switch (field) {
        case FIELD_allreducer_id: return pp->getAllreducer_id();
        case FIELD_training_process_id: return pp->getTraining_process_id();
        case FIELD_worker_id: return pp->getWorker_id();
        case FIELD_size: return (omnetpp::intval_t)(pp->getSize());
        case FIELD_rank: return (omnetpp::intval_t)(pp->getRank());
        case FIELD_layer: return (omnetpp::intval_t)(pp->getLayer());
        case FIELD_tensor_key: return (omnetpp::intval_t)(pp->getTensor_key());
        case FIELD_job_id: return (omnetpp::intval_t)(pp->getJob_id());
        case FIELD_num_workers_allocated: return (omnetpp::intval_t)(pp->getNum_workers_allocated());
        case FIELD_num_chunks: return (omnetpp::intval_t)(pp->getNum_chunks());
        case FIELD_chunk_id: return (omnetpp::intval_t)(pp->getChunk_id());
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'AllreduceRequest' as cValue -- field index out of range?", field);
    }
}

void AllreduceRequestDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    AllreduceRequest *pp = omnetpp::fromAnyPtr<AllreduceRequest>(object); (void)pp;
    switch (field) {
        case FIELD_allreducer_id: pp->setAllreducer_id(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_training_process_id: pp->setTraining_process_id(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_worker_id: pp->setWorker_id(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_size: pp->setSize(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_rank: pp->setRank(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_layer: pp->setLayer(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_tensor_key: pp->setTensor_key(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_job_id: pp->setJob_id(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_num_workers_allocated: pp->setNum_workers_allocated(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_num_chunks: pp->setNum_chunks(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_chunk_id: pp->setChunk_id(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'AllreduceRequest'", field);
    }
}

const char *AllreduceRequestDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr AllreduceRequestDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    AllreduceRequest *pp = omnetpp::fromAnyPtr<AllreduceRequest>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void AllreduceRequestDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    AllreduceRequest *pp = omnetpp::fromAnyPtr<AllreduceRequest>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'AllreduceRequest'", field);
    }
}

Register_Class(Job)

Job::Job(const char *name, short kind) : ::omnetpp::cMessage(name, kind)
{
}

Job::Job(const Job& other) : ::omnetpp::cMessage(other)
{
    copy(other);
}

Job::~Job()
{
}

Job& Job::operator=(const Job& other)
{
    if (this == &other) return *this;
    ::omnetpp::cMessage::operator=(other);
    copy(other);
    return *this;
}

void Job::copy(const Job& other)
{
    this->submit_time = other.submit_time;
    this->start_time = other.start_time;
    this->finish_time = other.finish_time;
    this->model = other.model;
    this->job_id = other.job_id;
    this->gpu = other.gpu;
    this->rank = other.rank;
    this->num_workers_allocated = other.num_workers_allocated;
    this->iters = other.iters;
}

void Job::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cMessage::parsimPack(b);
    doParsimPacking(b,this->submit_time);
    doParsimPacking(b,this->start_time);
    doParsimPacking(b,this->finish_time);
    doParsimPacking(b,this->model);
    doParsimPacking(b,this->job_id);
    doParsimPacking(b,this->gpu);
    doParsimPacking(b,this->rank);
    doParsimPacking(b,this->num_workers_allocated);
    doParsimPacking(b,this->iters);
}

void Job::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cMessage::parsimUnpack(b);
    doParsimUnpacking(b,this->submit_time);
    doParsimUnpacking(b,this->start_time);
    doParsimUnpacking(b,this->finish_time);
    doParsimUnpacking(b,this->model);
    doParsimUnpacking(b,this->job_id);
    doParsimUnpacking(b,this->gpu);
    doParsimUnpacking(b,this->rank);
    doParsimUnpacking(b,this->num_workers_allocated);
    doParsimUnpacking(b,this->iters);
}

omnetpp::simtime_t Job::getSubmit_time() const
{
    return this->submit_time;
}

void Job::setSubmit_time(omnetpp::simtime_t submit_time)
{
    this->submit_time = submit_time;
}

omnetpp::simtime_t Job::getStart_time() const
{
    return this->start_time;
}

void Job::setStart_time(omnetpp::simtime_t start_time)
{
    this->start_time = start_time;
}

omnetpp::simtime_t Job::getFinish_time() const
{
    return this->finish_time;
}

void Job::setFinish_time(omnetpp::simtime_t finish_time)
{
    this->finish_time = finish_time;
}

const char * Job::getModel() const
{
    return this->model.c_str();
}

void Job::setModel(const char * model)
{
    this->model = model;
}

uint64_t Job::getJob_id() const
{
    return this->job_id;
}

void Job::setJob_id(uint64_t job_id)
{
    this->job_id = job_id;
}

uint64_t Job::getGpu() const
{
    return this->gpu;
}

void Job::setGpu(uint64_t gpu)
{
    this->gpu = gpu;
}

uint64_t Job::getRank() const
{
    return this->rank;
}

void Job::setRank(uint64_t rank)
{
    this->rank = rank;
}

uint32_t Job::getNum_workers_allocated() const
{
    return this->num_workers_allocated;
}

void Job::setNum_workers_allocated(uint32_t num_workers_allocated)
{
    this->num_workers_allocated = num_workers_allocated;
}

uint64_t Job::getIters() const
{
    return this->iters;
}

void Job::setIters(uint64_t iters)
{
    this->iters = iters;
}

class JobDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_submit_time,
        FIELD_start_time,
        FIELD_finish_time,
        FIELD_model,
        FIELD_job_id,
        FIELD_gpu,
        FIELD_rank,
        FIELD_num_workers_allocated,
        FIELD_iters,
    };
  public:
    JobDescriptor();
    virtual ~JobDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(JobDescriptor)

JobDescriptor::JobDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(Job)), "omnetpp::cMessage")
{
    propertyNames = nullptr;
}

JobDescriptor::~JobDescriptor()
{
    delete[] propertyNames;
}

bool JobDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<Job *>(obj)!=nullptr;
}

const char **JobDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *JobDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int JobDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 9+base->getFieldCount() : 9;
}

unsigned int JobDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_submit_time
        FD_ISEDITABLE,    // FIELD_start_time
        FD_ISEDITABLE,    // FIELD_finish_time
        FD_ISEDITABLE,    // FIELD_model
        FD_ISEDITABLE,    // FIELD_job_id
        FD_ISEDITABLE,    // FIELD_gpu
        FD_ISEDITABLE,    // FIELD_rank
        FD_ISEDITABLE,    // FIELD_num_workers_allocated
        FD_ISEDITABLE,    // FIELD_iters
    };
    return (field >= 0 && field < 9) ? fieldTypeFlags[field] : 0;
}

const char *JobDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "submit_time",
        "start_time",
        "finish_time",
        "model",
        "job_id",
        "gpu",
        "rank",
        "num_workers_allocated",
        "iters",
    };
    return (field >= 0 && field < 9) ? fieldNames[field] : nullptr;
}

int JobDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "submit_time") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "start_time") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "finish_time") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "model") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "job_id") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "gpu") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "rank") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "num_workers_allocated") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "iters") == 0) return baseIndex + 8;
    return base ? base->findField(fieldName) : -1;
}

const char *JobDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "omnetpp::simtime_t",    // FIELD_submit_time
        "omnetpp::simtime_t",    // FIELD_start_time
        "omnetpp::simtime_t",    // FIELD_finish_time
        "string",    // FIELD_model
        "uint64_t",    // FIELD_job_id
        "uint64_t",    // FIELD_gpu
        "uint64_t",    // FIELD_rank
        "uint32_t",    // FIELD_num_workers_allocated
        "uint64_t",    // FIELD_iters
    };
    return (field >= 0 && field < 9) ? fieldTypeStrings[field] : nullptr;
}

const char **JobDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *JobDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int JobDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    Job *pp = omnetpp::fromAnyPtr<Job>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void JobDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    Job *pp = omnetpp::fromAnyPtr<Job>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'Job'", field);
    }
}

const char *JobDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    Job *pp = omnetpp::fromAnyPtr<Job>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string JobDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    Job *pp = omnetpp::fromAnyPtr<Job>(object); (void)pp;
    switch (field) {
        case FIELD_submit_time: return simtime2string(pp->getSubmit_time());
        case FIELD_start_time: return simtime2string(pp->getStart_time());
        case FIELD_finish_time: return simtime2string(pp->getFinish_time());
        case FIELD_model: return oppstring2string(pp->getModel());
        case FIELD_job_id: return uint642string(pp->getJob_id());
        case FIELD_gpu: return uint642string(pp->getGpu());
        case FIELD_rank: return uint642string(pp->getRank());
        case FIELD_num_workers_allocated: return ulong2string(pp->getNum_workers_allocated());
        case FIELD_iters: return uint642string(pp->getIters());
        default: return "";
    }
}

void JobDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    Job *pp = omnetpp::fromAnyPtr<Job>(object); (void)pp;
    switch (field) {
        case FIELD_submit_time: pp->setSubmit_time(string2simtime(value)); break;
        case FIELD_start_time: pp->setStart_time(string2simtime(value)); break;
        case FIELD_finish_time: pp->setFinish_time(string2simtime(value)); break;
        case FIELD_model: pp->setModel((value)); break;
        case FIELD_job_id: pp->setJob_id(string2uint64(value)); break;
        case FIELD_gpu: pp->setGpu(string2uint64(value)); break;
        case FIELD_rank: pp->setRank(string2uint64(value)); break;
        case FIELD_num_workers_allocated: pp->setNum_workers_allocated(string2ulong(value)); break;
        case FIELD_iters: pp->setIters(string2uint64(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Job'", field);
    }
}

omnetpp::cValue JobDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    Job *pp = omnetpp::fromAnyPtr<Job>(object); (void)pp;
    switch (field) {
        case FIELD_submit_time: return pp->getSubmit_time().dbl();
        case FIELD_start_time: return pp->getStart_time().dbl();
        case FIELD_finish_time: return pp->getFinish_time().dbl();
        case FIELD_model: return pp->getModel();
        case FIELD_job_id: return (omnetpp::intval_t)(pp->getJob_id());
        case FIELD_gpu: return (omnetpp::intval_t)(pp->getGpu());
        case FIELD_rank: return (omnetpp::intval_t)(pp->getRank());
        case FIELD_num_workers_allocated: return (omnetpp::intval_t)(pp->getNum_workers_allocated());
        case FIELD_iters: return (omnetpp::intval_t)(pp->getIters());
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'Job' as cValue -- field index out of range?", field);
    }
}

void JobDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    Job *pp = omnetpp::fromAnyPtr<Job>(object); (void)pp;
    switch (field) {
        case FIELD_submit_time: pp->setSubmit_time(value.doubleValue()); break;
        case FIELD_start_time: pp->setStart_time(value.doubleValue()); break;
        case FIELD_finish_time: pp->setFinish_time(value.doubleValue()); break;
        case FIELD_model: pp->setModel(value.stringValue()); break;
        case FIELD_job_id: pp->setJob_id(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_gpu: pp->setGpu(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_rank: pp->setRank(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        case FIELD_num_workers_allocated: pp->setNum_workers_allocated(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_iters: pp->setIters(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Job'", field);
    }
}

const char *JobDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr JobDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    Job *pp = omnetpp::fromAnyPtr<Job>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void JobDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    Job *pp = omnetpp::fromAnyPtr<Job>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Job'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

