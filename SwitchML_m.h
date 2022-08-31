//
// Generated file, do not edit! Created by opp_msgtool 6.0 from SwitchML.msg.
//

#ifndef __SWITCHML_M_H
#define __SWITCHML_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// opp_msgtool version check
#define MSGC_VERSION 0x0600
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgtool: 'make clean' should help.
#endif

class SwitchMLPacket;
class LayerAck;
class AllreduceRequest;
class Job;
/**
 * Enum generated from <tt>SwitchML.msg:12</tt> by opp_msgtool.
 * <pre>
 * enum SwitchMLPacketType
 * {
 *     SWITCHML_CONN_REQ = 0;
 *     SWITCHML_CONN_ACK = 1;
 *     SWITCHML_DISC_REQ = 2;
 *     SWITCHML_DISC_ACK = 3;
 *     SWITCHML_DATA = 4;
 * }
 * </pre>
 */
enum SwitchMLPacketType {
    SWITCHML_CONN_REQ = 0,
    SWITCHML_CONN_ACK = 1,
    SWITCHML_DISC_REQ = 2,
    SWITCHML_DISC_ACK = 3,
    SWITCHML_DATA = 4
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const SwitchMLPacketType& e) { b->pack(static_cast<int>(e)); }
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, SwitchMLPacketType& e) { int n; b->unpack(n); e = static_cast<SwitchMLPacketType>(n); }

/**
 * Class generated from <tt>SwitchML.msg:21</tt> by opp_msgtool.
 * <pre>
 * packet SwitchMLPacket
 * {
 *     int from_id;
 *     uint32_t slot;
 *     uint32_t ver;
 *     uint32_t offset;
 *     uint32_t tensor_key;
 *     uint32_t n_workers;
 *     uint64_t layer;
 *     uint64_t job_id;
 *     uint64_t num_pkts_expected;
 *     uint64_t grad_size;
 *     uint64_t num_chunks;
 *     uint64_t chunk_id;
 *     bool upward;
 * }
 * </pre>
 */
class SwitchMLPacket : public ::omnetpp::cPacket
{
  protected:
    int from_id = 0;
    uint32_t slot = 0;
    uint32_t ver = 0;
    uint32_t offset = 0;
    uint32_t tensor_key = 0;
    uint32_t n_workers = 0;
    uint64_t layer = 0;
    uint64_t job_id = 0;
    uint64_t num_pkts_expected = 0;
    uint64_t grad_size = 0;
    uint64_t num_chunks = 0;
    uint64_t chunk_id = 0;
    bool upward = false;

  private:
    void copy(const SwitchMLPacket& other);

  protected:
    bool operator==(const SwitchMLPacket&) = delete;

  public:
    SwitchMLPacket(const char *name=nullptr, short kind=0);
    SwitchMLPacket(const SwitchMLPacket& other);
    virtual ~SwitchMLPacket();
    SwitchMLPacket& operator=(const SwitchMLPacket& other);
    virtual SwitchMLPacket *dup() const override {return new SwitchMLPacket(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    virtual int getFrom_id() const;
    virtual void setFrom_id(int from_id);

    virtual uint32_t getSlot() const;
    virtual void setSlot(uint32_t slot);

    virtual uint32_t getVer() const;
    virtual void setVer(uint32_t ver);

    virtual uint32_t getOffset() const;
    virtual void setOffset(uint32_t offset);

    virtual uint32_t getTensor_key() const;
    virtual void setTensor_key(uint32_t tensor_key);

    virtual uint32_t getN_workers() const;
    virtual void setN_workers(uint32_t n_workers);

    virtual uint64_t getLayer() const;
    virtual void setLayer(uint64_t layer);

    virtual uint64_t getJob_id() const;
    virtual void setJob_id(uint64_t job_id);

    virtual uint64_t getNum_pkts_expected() const;
    virtual void setNum_pkts_expected(uint64_t num_pkts_expected);

    virtual uint64_t getGrad_size() const;
    virtual void setGrad_size(uint64_t grad_size);

    virtual uint64_t getNum_chunks() const;
    virtual void setNum_chunks(uint64_t num_chunks);

    virtual uint64_t getChunk_id() const;
    virtual void setChunk_id(uint64_t chunk_id);

    virtual bool getUpward() const;
    virtual void setUpward(bool upward);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const SwitchMLPacket& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, SwitchMLPacket& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>SwitchML.msg:38</tt> by opp_msgtool.
 * <pre>
 * message LayerAck
 * {
 *     uint64_t layer;
 *     simtime_t weight_update_time;
 *     bool completed;
 * }
 * 
 * //message Setup
 * //{
 * //    int id;
 * //};
 * </pre>
 */
class LayerAck : public ::omnetpp::cMessage
{
  protected:
    uint64_t layer = 0;
    omnetpp::simtime_t weight_update_time = SIMTIME_ZERO;
    bool completed = false;

  private:
    void copy(const LayerAck& other);

  protected:
    bool operator==(const LayerAck&) = delete;

  public:
    LayerAck(const char *name=nullptr, short kind=0);
    LayerAck(const LayerAck& other);
    virtual ~LayerAck();
    LayerAck& operator=(const LayerAck& other);
    virtual LayerAck *dup() const override {return new LayerAck(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    virtual uint64_t getLayer() const;
    virtual void setLayer(uint64_t layer);

    virtual omnetpp::simtime_t getWeight_update_time() const;
    virtual void setWeight_update_time(omnetpp::simtime_t weight_update_time);

    virtual bool getCompleted() const;
    virtual void setCompleted(bool completed);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const LayerAck& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, LayerAck& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>SwitchML.msg:50</tt> by opp_msgtool.
 * <pre>
 * message AllreduceRequest
 * {
 *     int allreducer_id;
 *     int training_process_id;
 *     int worker_id;
 *     uint64_t size;
 *     uint64_t rank;
 *     uint64_t layer;
 *     uint64_t tensor_key;
 *     uint64_t job_id;
 *     uint64_t num_workers_allocated;
 *     uint64_t num_chunks = 1;
 *     uint64_t chunk_id = 0;
 * }
 * </pre>
 */
class AllreduceRequest : public ::omnetpp::cMessage
{
  protected:
    int allreducer_id = 0;
    int training_process_id = 0;
    int worker_id = 0;
    uint64_t size = 0;
    uint64_t rank = 0;
    uint64_t layer = 0;
    uint64_t tensor_key = 0;
    uint64_t job_id = 0;
    uint64_t num_workers_allocated = 0;
    uint64_t num_chunks = 1;
    uint64_t chunk_id = 0;

  private:
    void copy(const AllreduceRequest& other);

  protected:
    bool operator==(const AllreduceRequest&) = delete;

  public:
    AllreduceRequest(const char *name=nullptr, short kind=0);
    AllreduceRequest(const AllreduceRequest& other);
    virtual ~AllreduceRequest();
    AllreduceRequest& operator=(const AllreduceRequest& other);
    virtual AllreduceRequest *dup() const override {return new AllreduceRequest(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    virtual int getAllreducer_id() const;
    virtual void setAllreducer_id(int allreducer_id);

    virtual int getTraining_process_id() const;
    virtual void setTraining_process_id(int training_process_id);

    virtual int getWorker_id() const;
    virtual void setWorker_id(int worker_id);

    virtual uint64_t getSize() const;
    virtual void setSize(uint64_t size);

    virtual uint64_t getRank() const;
    virtual void setRank(uint64_t rank);

    virtual uint64_t getLayer() const;
    virtual void setLayer(uint64_t layer);

    virtual uint64_t getTensor_key() const;
    virtual void setTensor_key(uint64_t tensor_key);

    virtual uint64_t getJob_id() const;
    virtual void setJob_id(uint64_t job_id);

    virtual uint64_t getNum_workers_allocated() const;
    virtual void setNum_workers_allocated(uint64_t num_workers_allocated);

    virtual uint64_t getNum_chunks() const;
    virtual void setNum_chunks(uint64_t num_chunks);

    virtual uint64_t getChunk_id() const;
    virtual void setChunk_id(uint64_t chunk_id);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const AllreduceRequest& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, AllreduceRequest& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>SwitchML.msg:65</tt> by opp_msgtool.
 * <pre>
 * message Job
 * {
 *     simtime_t submit_time = 0;
 *     simtime_t start_time = 0;
 *     simtime_t finish_time = 0;
 *     string model = "bert";
 *     uint64_t job_id = 0;
 *     uint64_t gpu = 8;
 *     uint64_t rank = 0;
 *     uint32_t num_workers_allocated = 2;
 *     uint64_t iters = 2;
 * }
 * </pre>
 */
class Job : public ::omnetpp::cMessage
{
  protected:
    omnetpp::simtime_t submit_time = 0;
    omnetpp::simtime_t start_time = 0;
    omnetpp::simtime_t finish_time = 0;
    omnetpp::opp_string model = "bert";
    uint64_t job_id = 0;
    uint64_t gpu = 8;
    uint64_t rank = 0;
    uint32_t num_workers_allocated = 2;
    uint64_t iters = 2;

  private:
    void copy(const Job& other);

  protected:
    bool operator==(const Job&) = delete;

  public:
    Job(const char *name=nullptr, short kind=0);
    Job(const Job& other);
    virtual ~Job();
    Job& operator=(const Job& other);
    virtual Job *dup() const override {return new Job(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    virtual omnetpp::simtime_t getSubmit_time() const;
    virtual void setSubmit_time(omnetpp::simtime_t submit_time);

    virtual omnetpp::simtime_t getStart_time() const;
    virtual void setStart_time(omnetpp::simtime_t start_time);

    virtual omnetpp::simtime_t getFinish_time() const;
    virtual void setFinish_time(omnetpp::simtime_t finish_time);

    virtual const char * getModel() const;
    virtual void setModel(const char * model);

    virtual uint64_t getJob_id() const;
    virtual void setJob_id(uint64_t job_id);

    virtual uint64_t getGpu() const;
    virtual void setGpu(uint64_t gpu);

    virtual uint64_t getRank() const;
    virtual void setRank(uint64_t rank);

    virtual uint32_t getNum_workers_allocated() const;
    virtual void setNum_workers_allocated(uint32_t num_workers_allocated);

    virtual uint64_t getIters() const;
    virtual void setIters(uint64_t iters);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const Job& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, Job& obj) {obj.parsimUnpack(b);}


namespace omnetpp {

template<> inline SwitchMLPacket *fromAnyPtr(any_ptr ptr) { return check_and_cast<SwitchMLPacket*>(ptr.get<cObject>()); }
template<> inline LayerAck *fromAnyPtr(any_ptr ptr) { return check_and_cast<LayerAck*>(ptr.get<cObject>()); }
template<> inline AllreduceRequest *fromAnyPtr(any_ptr ptr) { return check_and_cast<AllreduceRequest*>(ptr.get<cObject>()); }
template<> inline Job *fromAnyPtr(any_ptr ptr) { return check_and_cast<Job*>(ptr.get<cObject>()); }

}  // namespace omnetpp

#endif // ifndef __SWITCHML_M_H

