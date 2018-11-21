#ifndef __HERMES_ASYNC_ENGINE_HPP__
#define __HERMES_ASYNC_ENGINE_HPP__

// C++ includes
#include <string>
#include <unordered_map>
#include <atomic>
#include <cassert>
#include <thread>
#include <mutex>
#include <set>
#include <vector>
#include <utility>
#include <future>
#include <chrono>
#include <algorithm>

// C includes
#include <mercury.h>
#include <mercury_macros.h>
#include <mercury_hash_string.h>

// project includes
#if __cplusplus == 201103L
#include <hermes/make_unique.hpp>
#endif // __cplusplus == 201103L

#include <hermes/buffer.hpp>
#include <hermes/exposed_memory.hpp>
#include <hermes/messages.hpp>
#include <hermes/request.hpp>
#include <hermes/detail/request_registrar.hpp>
#include <hermes/detail/request_status.hpp>
#include <hermes/detail/mercury_utils.hpp>
#include <hermes/logging.hpp>

// #ifndef __HERMES_RPC_DEFINITIONS_HPP__
// #error "FATAL: RPC definitions not found. "
//        "Please include it before this file."
// #endif

namespace hermes {

// defined in this file
template <typename Request>
class rpc_handle_v2;

/** Valid transport types (i.e. transport types supported by Mercury) */
enum class transport {
    bmi_tcp = 0, mpi_dynamic, mpi_static, na_sm, cci_tcp, cci_verbs, cci_gni,
    cci_sm, ofi_tcp, ofi_psm2, ofi_verbs, ofi_gni
};

/** enum class hash (required for upcoming std::unordered_maps */
struct enum_class_hash {
    template <typename T>
    std::size_t operator()(T t) const noexcept {
        return static_cast<std::size_t>(t);
    }
};

/** Map to translate from transport to Mercury's transport string */
static const
std::unordered_map<
    transport,
    const char* const,
    enum_class_hash
> known_transports = {
    { transport::bmi_tcp,     "bmi+tcp://"     },
    { transport::mpi_dynamic, "mpi+dynamic://" },
    { transport::mpi_static,  "mpi+static://"  },
    { transport::na_sm,       "na+sm://"       },
    { transport::cci_tcp,     "cci+tcp://"     },
    { transport::cci_verbs,   "cci+verbs://"   },
    { transport::cci_gni,     "cci+gni://"     },
    { transport::cci_sm,      "cci+sm://"      },
    { transport::ofi_tcp,     "ofi+tcp://"     },
    { transport::ofi_psm2,    "ofi+psm2://"    },
    { transport::ofi_verbs,   "ofi+verbs://"   },
    { transport::ofi_gni,     "ofi+gni://"     }
};

#define PRINT_HG_STATE(om, st) \
do { \
    om("  ({}) hg_client_rpc_state = {{", fmt::ptr(st)); \
    om("    m_hg_context: {}", fmt::ptr(st->m_hg_context)); \
    om("    ({}) m_rpc_descriptor = {{", fmt::ptr(st->m_rpc_descriptor.get())); \
    om("      m_type: {}", static_cast<int>(st->m_rpc_descriptor->m_type)); \
    om("      m_name: {}", st->m_rpc_descriptor->m_name); \
    om("      m_hg_id: {}", st->m_rpc_descriptor->m_hg_id); \
    om("      m_in_proc_cb: {}", (void*) st->m_rpc_descriptor->m_in_proc_cb); \
    om("      m_out_proc_cb: {}", (void*) st->m_rpc_descriptor->m_out_proc_cb); \
    om("      m_rpc_cb: {}", (void*) st->m_rpc_descriptor->m_rpc_cb); \
    om("    }};"); \
    om("    m_size: {}", st->m_size); \
    om("    m_buffer: {}", fmt::ptr(st->m_buffer)); \
    om("    m_handle: {}", fmt::ptr(st->m_handle)); \
    om("    m_bulk_handle: {}", fmt::ptr(st->m_bulk_handle)); \
    om("  }};"); \
} while(0);

namespace detail {

/** A simple RAII wrapper for hg_addr_t. This way we can keep track of 
 * generated mercury addresses both in enddpoints and in the address cache 
 * using std::shared_ptr<address>(), and only free them when the last referrer
 * dies, which is convenient */
struct address {

    address(hg_class_t* hg_class, 
            hg_addr_t hg_addr) :
        m_hg_class(hg_class),
        m_hg_addr(hg_addr) {}

    ~address() {
        HG_Addr_free(const_cast<hg_class_t*>(m_hg_class), m_hg_addr);
    }

    // TODO: rename to mercury_address()
    hg_addr_t
    fetch() const {
        return m_hg_addr;
    }

    const hg_class_t* const m_hg_class;
    const hg_addr_t m_hg_addr;
};


/** Execution context required by the RPC's originator (i.e. the client) */
template <typename Request>
struct execution_context_v2 {

    using value_type = Request;
    using Handle = rpc_handle_v2<Request>;
    using Input = typename Request::input_type;
    using Output = typename Request::output_type;
    using MercuryInput = typename Request::mercury_input_type;
    using MercuryOutput = typename Request::mercury_output_type;

    execution_context_v2(Handle* parent,
                      const hg_context_t * const hg_context,
                      const std::shared_ptr<detail::address>& address,
                      Input&& input) :
        m_parent(parent),
        m_hg_context(hg_context),
        m_handle(HG_HANDLE_NULL),
        m_bulk_handle(HG_BULK_NULL),
        m_status(detail::request_status::created),
        m_address(address),
        // the context takes ownership of the user's input in order to ensure
        // that any pointers in m_mercury_input that refer back to it (e.g. 
        // strings) survives as long as needed (this is required because
        // mercury does not take ownership of any pointers in the user input)
        m_user_input(input),
        // invoke explicit cast constructor
        m_mercury_input(input) { }

    Handle* m_parent;
    const hg_context_t* const m_hg_context;
    hg_handle_t m_handle;
    hg_bulk_t m_bulk_handle;
    std::atomic<detail::request_status> m_status;

    const std::shared_ptr<detail::address> m_address;
    Input m_user_input;
    MercuryInput m_mercury_input;

    std::promise<Output> m_output_promise;
};


#if 0
/** Context required by the RPC's originator (i.e. the client) */
template <rpc ID, typename RpcHandle>
struct execution_context {

    execution_context(RpcHandle* parent,
                      const hg_context_t * const hg_context,
                      const std::shared_ptr<detail::address>& address,
                      const RpcInput<ID>& input) :
        m_parent(parent),
        m_hg_context(hg_context),
        m_handle(HG_HANDLE_NULL),
        m_bulk_handle(HG_BULK_NULL),
        m_status(detail::request_status::created),
        m_address(address),
        m_rpc_descriptor(detail::rpc_descriptor<ID>()),
        m_input(input)
//        m_hg_input(m_input),


    {
        DEBUG2("hg_client_rpc_state(hg_context={})", fmt::ptr(m_hg_context));

        //MercuryInput<ID> foo = MercuryInput<ID>(m_input);

        //PRINT_HG_STATE(DEBUG2, this);
    }

    ~execution_context() { 
        DEBUG2("{}()", __func__);
    }

    const hg_context_t*
    hg_context() const {
        return m_hg_context;
    }

    hermes::rpc
    id() const {
        return m_rpc_descriptor.m_id;
    }

#if 0
    hermes::message
    type() const {
        return m_rpc_descriptor.m_type;
    }
#endif

 
    // TODO: rename to mercury_op_id()
    hg_id_t
    hg_id() const {
        return m_rpc_descriptor.m_hg_id;
    }

    // TODO: rename to mercury_address()
    hg_addr_t
    hg_addr() const {
        return m_address->fetch();
    }

    RpcInput<ID>
    rpc_input() const {
        return m_input;
    }

///    MercuryInput<ID>
///    mercury_input() const {
///        return m_hg_input;
///    }

    RpcHandle* m_parent;
    const hg_context_t* const m_hg_context;
    hg_handle_t m_handle;
    hg_bulk_t m_bulk_handle;
    std::atomic<detail::request_status> m_status;

    const std::shared_ptr<detail::address> m_address;
    const rpc_descriptor<ID> m_rpc_descriptor;
    RpcInput<ID> m_input;

    std::promise<RpcOutput<ID>> m_output_promise;

};
#endif


#if 0 /// XXX superseded by hermes::request
/** Context required by the RPC's target (i.e. the server) */
template <rpc ID>
struct target_context {

    //TODO RpcInput<ID> should probably be MercuryInput<ID>
    target_context(const MercuryInput<ID>& input_args) :
        m_input(input_args),
    m_size(0),
    m_buffer(NULL),
    m_handle(NULL),
    m_bulk_handle(NULL) {}

    ~target_context() { }

    MercuryInput<ID>
    mercury_input() const {
        return m_input;
    }

    const MercuryInput<ID> m_input;
//    RpcOutputArgTp<ID> m_output_vals;

    hg_size_t m_size;
    void* m_buffer;
    hg_handle_t m_handle;
    hg_bulk_t m_bulk_handle;
};
#endif

#if 0
//XXX move to own header
template <typename Request>
struct executor_v2 {

    using value_type = Request;
    using MercuryInput = typename Request::mercury_input_type;
    using MercuryOutput = typename Request::mercury_output_type;
    using RequestOutput = typename Request::output_type;

    template <typename ExecutionContext>
    static hg_return_t // TODO return type should probably change
    run(ExecutionContext* ctx) {

        DEBUG2("{}(ctx={})", __func__, fmt::ptr(ctx));

        // create a Mercury handle for the RPC and save it in the RPC's 
        // execution context
        ctx->m_handle = detail::create_handle(ctx->m_hg_context,
                                              ctx->m_address->fetch(),
                                              Request::mercury_id);
        //TODO TODO TODO

        // convert the hermes RPC input to Mercury RPC input
        // (this relies on the explicit conversion constructor defined for 
        // the type)
        auto&& hg_input = MercuryInput(ctx->m_input);

        // send the RPC
        detail::forward_v2<ExecutionContext>(ctx);

        return HG_SUCCESS;
    }
};
#endif


#if 0
template <rpc ID, typename Message>
struct executor {};

/** 
 * Specialization for sending RPCs that do not require a bulk transfer
 * */
template <rpc ID>
struct executor<ID, message::simple> {

    using RpcInfoTp = rpc_descriptor<ID>;

    // TODO: this function and repost() can probably return void and throw
    // an exception if needed. Also, we could cache hg_input into the context
    // to avoid converting it in the event of a repost. Also++ all this could
    // probably be done in the constructor of the context
    template <typename ExecutionContext>
    static hg_return_t
    post(ExecutionContext* ctx) {

        DEBUG2("{}(ctx={})", __func__, fmt::ptr(ctx));

        // create a Mercury handle for the RPC and save it in the RPC's 
        // execution context
        ctx->m_handle = detail::create_handle(ctx->m_hg_context,
                                              ctx->hg_addr(),
                                              ctx->hg_id());

        // convert the hermes RPC input to Mercury RPC input
        // (this relies on the explicit conversion constructor defined for 
        // the type)
        auto&& hg_input = MercuryInput<ID>(ctx->m_input);

        // send the RPC
        detail::forward<ExecutionContext, 
                        MercuryInput<ID>, 
                        MercuryOutput<ID>,
                        RpcOutput<ID>>(
                                ctx, 
                                std::forward<MercuryInput<ID>>(hg_input));

        return HG_SUCCESS;
    }

    template <typename ExecutionContext>
    static hg_return_t
    repost(ExecutionContext* ctx) {

        DEBUG2("{}(ctx={})", __func__, fmt::ptr(ctx));

        // convert the hermes RPC input to Mercury RPC input
        // (this relies on the explicit conversion constructor defined for 
        // the type)
        auto&& hg_input = MercuryInput<ID>(ctx->m_input);

        // send the RPC
        detail::forward<ExecutionContext, 
                        MercuryInput<ID>, 
                        MercuryOutput<ID>,
                        RpcOutput<ID>>(
                                ctx, 
                                std::forward<MercuryInput<ID>>(hg_input));

        return HG_SUCCESS;
    }

    static hg_return_t
    process(hg_handle_t handle) {

        MercuryInput<ID> hg_input;

        // decode input
        hg_return_t ret = HG_Get_input(handle, &hg_input);
        assert(ret == HG_SUCCESS);

        DEBUG2("Looking up RPC descriptor for [{}]", 
               rpc_names[static_cast<int>(ID)]);

        auto rpc_desc = 
            std::static_pointer_cast<RpcInfoTp>(
                    detail::registered_rpcs_v2.at(ID));

        RpcInput<ID> rpc_input = RpcInput<ID>(hg_input);

        // TODO: pass bulk_handle implicitly into rpc_input
        request<RpcInput<ID>> req(handle, 
                                  std::move(rpc_input));

        rpc_desc->invoke_user_handler(std::move(req));

        return HG_SUCCESS;
    }

};

/** 
 * Specialization for sending RPCs that require a bulk transfer
 * */
template <rpc ID>
struct executor<ID, message::bulk> {

    using RpcInfoTp = rpc_descriptor<ID>;

    template <typename ExecutionContext>
    static hg_return_t
    post(ExecutionContext* ctx) {

        DEBUG2("{}(ctx={})", __func__, fmt::ptr(ctx));

        //PRINT_HG_STATE(DEBUG2, ctx);

        // create a Mercury handle for the RPC and save it in the RPC's 
        // execution context
        ctx->m_handle = detail::create_handle(ctx->m_hg_context,
                                              ctx->hg_addr(),
                                              ctx->hg_id());

        // convert the hermes RPC input to Mercury RPC input
        // (this relies on the explicit conversion constructor defined for 
        // the type)
        auto&& hg_input = MercuryInput<ID>(ctx->m_input);

        // also save the Mercury bulk_handle in the execution context so that
        // we can refer to it later
        ctx->m_bulk_handle = hg_input.bulk_handle;

        // send the RPC
        detail::forward<ExecutionContext, 
                        MercuryInput<ID>, 
                        MercuryOutput<ID>,
                        RpcOutput<ID>>(
                                ctx, 
                                std::forward<MercuryInput<ID>>(hg_input));

        return HG_SUCCESS;
    }

    static hg_return_t
    process(hg_handle_t handle) {

        MercuryInput<ID> hg_input;

        // decode input
        hg_return_t ret = HG_Get_input(handle, &hg_input);
        assert(ret == HG_SUCCESS);

        DEBUG2("Looking up RPC descriptor for [{}]", 
               rpc_names[static_cast<int>(ID)]);

        auto rpc_desc = 
            std::static_pointer_cast<RpcInfoTp>(
                    detail::registered_rpcs_v2.at(ID));

        RpcInput<ID> rpc_input = RpcInput<ID>(hg_input);

        // TODO: pass bulk_handle implicitly into rpc_input
        request<RpcInput<ID>> req(handle, 
                                  hg_input.bulk_handle, 
                                  std::move(rpc_input));

        rpc_desc->invoke_user_handler(std::move(req));

        return HG_SUCCESS;
    }
};
#endif



#if 0
// forward declaration of helper functions
template <rpc ID>
hg_return_t
post_lookup_request(const char* address, 
                    execution_context<ID>* ctx);

template <rpc ID>
hg_return_t
lookup_completion_callback(const struct hg_cb_info* cbi);
#endif

} // namespace detail


struct endpoint_compare;

/** */
class endpoint {

    friend class endpoint_compare;
    friend class async_engine;

    endpoint(const std::string& name,
             std::size_t port,
             const std::shared_ptr<detail::address>& address) :
        m_name(name),
        m_port(port),
        m_address(address) {}

public:
    const std::string
    name() const {
        return m_name;
    }

    std::size_t
    port() const {
        return m_port;
    }

private:
    std::shared_ptr<detail::address>
    address() const {
        return m_address;
    }

private:
    const std::string m_name;
    const std::size_t m_port;
    const std::shared_ptr<detail::address> m_address;
};

struct endpoint_compare {
    bool
    operator()(const endpoint& lhs, const endpoint& rhs) {
        return (std::hash<std::string>()(lhs.m_name)*lhs.m_port) <
               (std::hash<std::string>()(rhs.m_name)*rhs.m_port);
    }
};

/** */
class endpoint_set {

    friend class async_engine;

public:

    using iterator = std::set<endpoint, endpoint_compare>::iterator;
    using const_iterator = std::set<endpoint, endpoint_compare>::const_iterator;

    endpoint_set() {}

    explicit endpoint_set(std::initializer_list<endpoint>&& e_list) :
        m_endpoints(e_list) {}

    bool
    add(endpoint&& endp) {
        return m_endpoints.emplace(endp).second;
    }

    std::size_t
    size() const {
        return m_endpoints.size();
    }

    const_iterator
    begin() const {
        return m_endpoints.begin();
    }

    const_iterator
    end() const {
        return m_endpoints.end();
    }

    iterator
    begin() {
        return m_endpoints.begin();
    }

    iterator
    end() {
        return m_endpoints.end();
    }

private:
    std::set<endpoint, endpoint_compare> m_endpoints;
};

template <typename Request>
using output_set = std::vector<typename Request::output_type>;

/** public */

template <typename Request>
class rpc_handle_v2 {

    friend class async_engine;

//    template <
//        typename ExecutionContext, 
//        typename MercuryInput, 
//        typename MercuryOutput,
//        typename Output>
//    friend void
//    detail::forward(ExecutionContext* ctx,
//                    MercuryInput&& input);

    using Input = typename Request::input_type;
    using Output = typename Request::output_type;

    // XXX: maybe not needed
    using ExecutionContext = detail::execution_context_v2<Request>;

    // XXX: we use SFINAE to make sure that the type of the input 
    // is Request::input_type
    template <typename InputData,
              typename Enable = typename 
                  std::is_same<InputData, 
                               typename Request::input_type>::type>
    rpc_handle_v2(const hg_context_t * const hg_context,
               const std::vector<std::shared_ptr<detail::address>>& targets,
               InputData&& input) {

        m_ctxs.reserve(targets.size());
        m_futures.reserve(targets.size());

        for(auto&& addr : targets) {
            DEBUG2("Creating execution_context for RPC");

            m_ctxs.emplace_back(
                std::make_unique<ExecutionContext>(
                    this, 
                    hg_context, 
                    addr, 
                    std::forward<InputData>(input)));

            DEBUG2("Creating future for RPC");

            m_futures.emplace_back(
                    m_ctxs.back()->m_output_promise.get_future());
        }
    }

public:
    rpc_handle_v2(const rpc_handle_v2&) = delete;
    rpc_handle_v2(rpc_handle_v2&&) = default;
    rpc_handle_v2& operator=(const rpc_handle_v2&) = delete;
    rpc_handle_v2& operator=(rpc_handle_v2&&) = default;

    ~rpc_handle_v2() {
        DEBUG2("{}", __func__);
//        wait();// TODO
    }

    void
    wait() const {
        for(auto&& f : m_futures) {
            if(f.valid()) {
                f.wait();
            }
        }
    }

    std::vector<Output>
    get() const {

        assert(m_futures.size() == m_ctxs.size());

        DEBUG("Getting RPC results (pending: {})", m_futures.size());

        constexpr const auto TIMEOUT = std::chrono::seconds(1);
        constexpr const auto RETRIES = 5;

        std::vector<Output> result_set;
        std::vector<bool> retrieved(m_futures.size(), false);
        std::vector<uint8_t> retries(m_futures.size(), RETRIES);

        for(auto pending_requests = m_futures.size();
            pending_requests > 0; 
            /* counter decrease is internal to the loop */) {

            for(std::size_t i = 0; i < m_futures.size(); ++i) {

                if(retrieved[i]) {
                    continue;
                }

                switch(m_futures[i].wait_for(TIMEOUT)) {
                    case std::future_status::timeout: 
                    {

                        m_ctxs[i]->m_status = 
                                (retries[i]-- != 0 ?
                                    detail::request_status::timeout :
                                    detail::request_status::cancelled);

                        DEBUG2("Mercury request timed out, {}",
                                m_ctxs[i]->m_status == detail::request_status::timeout ?
                                fmt::format("reposting request ({} of {})", RETRIES - retries[i], RETRIES) :
                                "cancelling");


                        // cancel the pending Mercury request, this causes
                        // its completion callback to be invoked and we can
                        // use it to either repost the request or make it 
                        // gracefully dispose of itself depending on the value
                        // of request_status
                        hg_return_t ret = HG_Cancel(m_ctxs[i]->m_handle);

                        if(ret != HG_SUCCESS) {
                            WARNING("Failed to cancel RPC: {}", 
                                    HG_Error_to_string(ret));
                        }
                        break;
                    }

                    case std::future_status::ready:
                    {
                        // the request "completed", i.e. we can retrieve either
                        // a valid result or an error condition
                        DEBUG2("RPC completed. Retrieving result.");
                        result_set.emplace_back(m_futures[i].get());
                        retrieved[i] = true;
                        --pending_requests;
                        break;
                    }

                    default:
                    {
                        DEBUG2("RPC future is in an inconsistent state. Aborting.");
                        throw std::runtime_error("Unexpected future_status in "
                                                 "wait_for()");
                    }
                }

            }
        }

        return result_set;
    }

private:
    std::vector<std::unique_ptr<ExecutionContext>> m_ctxs;
    mutable std::vector<std::future<Output>> m_futures;
};


#if 0
template <rpc ID>
class rpc_handle {

    friend class async_engine;

    template <
        typename ExecutionContext, 
        typename MercuryInput, 
        typename MercuryOutput,
        typename RpcOutput>
    friend void
    detail::forward(ExecutionContext* ctx,
                    MercuryInput&& input);

    using ExecutionContext = 
        detail::execution_context<ID, rpc_handle<ID>>;

    rpc_handle(const hg_context_t * const hg_context,
               const std::vector<std::shared_ptr<detail::address>>& targets,
               const RpcInput<ID>& input) {

        m_ctxs.reserve(targets.size());
        m_futures.reserve(targets.size());

        for(auto&& addr : targets) {
            DEBUG2("Creating execution_context for RPC");

            m_ctxs.emplace_back(
                            std::make_shared<ExecutionContext>(
                                this, hg_context, addr, input));

            DEBUG2("Creating future for RPC");

            m_futures.emplace_back(
                    m_ctxs.back()->m_output_promise.get_future());
        }
    }

public:
    rpc_handle(const rpc_handle&) = delete;
    rpc_handle(rpc_handle&&) = default;
    rpc_handle& operator=(const rpc_handle&) = delete;
    rpc_handle& operator=(rpc_handle&&) = default;

    ~rpc_handle() {
        DEBUG2("{}", __func__);
        wait();
    }

    void
    wait() const {
        for(auto&& f : m_futures) {
            if(f.valid()) {
                f.wait();
            }
        }
    }

    result_set<ID>
    get() const {

        assert(m_futures.size() == m_ctxs.size());

        DEBUG("Getting RPC results (pending: {})", m_futures.size());

        constexpr const auto TIMEOUT = std::chrono::seconds(1);
        constexpr const auto RETRIES = 5;

        std::vector<RpcOutput<ID>> result_set;
        std::vector<bool> retrieved(m_futures.size(), false);
        std::vector<uint8_t> retries(m_futures.size(), RETRIES);


        std::size_t pending_requests = m_futures.size();

        while(pending_requests != 0) {
            for(std::size_t i = 0; i < m_futures.size(); ++i) {

                if(retrieved[i]) {
                    continue;
                }

                switch(m_futures[i].wait_for(TIMEOUT)) {
                    case std::future_status::timeout: 
                    {

                        m_ctxs[i]->m_status = 
                                (retries[i]-- != 0 ?
                                    detail::request_status::timeout :
                                    detail::request_status::cancelled);

                        DEBUG2("Mercury request timed out, {}",
                                m_ctxs[i]->m_status == detail::request_status::timeout ?
                                fmt::format("reposting request ({} of {})", RETRIES - retries[i], RETRIES) :
                                "cancelling");


                        // cancel the pending Mercury request, this causes
                        // its completion callback to be invoked and we can
                        // use it to either repost the request or make it 
                        // gracefully dispose of itself depending on the value
                        // of request_status
                        hg_return_t ret = HG_Cancel(m_ctxs[i]->m_handle);

                        if(ret != HG_SUCCESS) {
                            WARNING("Failed to cancel RPC: {}", 
                                    HG_Error_to_string(ret));
                        }
                        break;
                    }

                    case std::future_status::ready:
                    {
                        // the request "completed", i.e. we can retrieve either
                        // a valid result or an error condition
                        DEBUG2("RPC completed. Retrieving result.");
                        result_set.emplace_back(m_futures[i].get());
                        retrieved[i] = true;
                        --pending_requests;
                        break;
                    }

                    default:
                    {
                        DEBUG2("RPC future is in an inconsistent state. Aborting.");
                        throw std::runtime_error("Unexpected future_status in "
                                                 "wait_for()");
                    }
                }

            }
        }

        return result_set;
    }

private:
    //XXX shared_ptr probably no longer needed due to the use of futures
    std::vector<std::shared_ptr<ExecutionContext>> m_ctxs;
    mutable std::vector<std::future<RpcOutput<ID>>> m_futures;
};
#endif

class async_engine {

    struct lookup_ctx {

        explicit lookup_ctx(const hg_class_t* const hg_class) : 
            m_lookup_finished(false),
            m_hg_class(hg_class) { }

        ~lookup_ctx() {
        }

        bool m_lookup_finished;
        const hg_class_t* const m_hg_class;
        hg_addr_t m_hg_addr;
        hg_return_t m_hg_ret;
    };

public:
    async_engine(const async_engine& other) = delete;
    async_engine(async_engine&& rhs) = default;
    async_engine& operator=(const async_engine& other) = delete;
    async_engine& operator=(async_engine&& rhs) = default;

    /**
     * Initialize the Mercury asynchronous engine.
     *
     * @param tr_type [IN]  the desired transport type (see enum class
     *                      tranport_type)
     * @param listen  [IN]  listen for incoming connections
     */
    async_engine(transport tr_type, bool listen = false) :
        m_shutdown(false),
        m_listen(listen),
        m_transport(tr_type),
        m_transport_prefix(
            // we use an immediately-invoked lambda 
            // to const-initialize m_transport_prefix
            [tr_type]() {

                if(!known_transports.count(tr_type)) {
                    throw std::runtime_error("Failed to initialize Mercury: unknown "
                                            "transport requested");
                }

                return known_transports.at(tr_type);
            }()) {



        DEBUG("Initializing Mercury asynchronous engine");

        DEBUG("  Initializing transport layer");

        const auto address = make_address("127.0.0.1", 22222);

        m_hg_class = HG_Init(address.c_str(),
                             m_listen ? HG_TRUE : HG_FALSE);

        if(m_hg_class == NULL) {
            throw std::runtime_error("Failed to initialize Mercury");
        }

        hg_addr_t self_addr;
        hg_return_t ret = HG_Addr_self(m_hg_class, &self_addr);

        if(ret != HG_SUCCESS) {
            throw std::runtime_error("Failed to retrieve self address: " + 
                    std::string(HG_Error_to_string(ret)));
        }

        hg_size_t buf_size = 0;
        ret = HG_Addr_to_string(m_hg_class, NULL, &buf_size, self_addr);

        if(ret != HG_SUCCESS) {
            throw std::runtime_error("Failed to compute address length " +
                                     std::string(HG_Error_to_string(ret)));
        }

HG_Addr_free(m_hg_class, self_addr);

        auto text_address = std::make_unique<char[]>(buf_size);

        ret = HG_Addr_to_string(m_hg_class, text_address.get(), &buf_size, self_addr);

        if(ret != HG_SUCCESS) {
            throw std::runtime_error("Failed to compute address length " +
                                     std::string(HG_Error_to_string(ret)));
        }

        INFO("Self address: {}", text_address.get());

        DEBUG2("    m_hg_class: {}", static_cast<void*>(m_hg_class));

        DEBUG("  Creating context");
        m_hg_context = HG_Context_create(m_hg_class);

        if(m_hg_context == NULL) {
            throw std::runtime_error("Failed to create Mercury context");
        }

        DEBUG2("    m_hg_context: {}", static_cast<void*>(m_hg_context));

        DEBUG("  Registering RPCs");
        register_rpcs();
    }

    /**
     * Destroy the Mercury asynchronous engine.
     */
    ~async_engine() {

        DEBUG("Destroying Mercury asynchronous engine");
        DEBUG("  Stopping runners");

        m_shutdown = true;

        if(m_runner.joinable()) {
            m_runner.join();
        }

        DEBUG("  Cleaning address cache");
        {
            std::lock_guard<std::mutex> lock(m_addr_cache_mutex);
            m_addr_cache.clear();
        }

        if(m_hg_context != NULL) {
            DEBUG("  Destroying context");
            const auto err = HG_Context_destroy(m_hg_context);

            if(err != HG_SUCCESS) {
                // can't throw here
                ERROR("Failed to destroy context: {}\n",
                      HG_Error_to_string(err));
            }
        }

        if(m_hg_class != NULL) {
            DEBUG("  Finalizing transport layer");
            const auto err = HG_Finalize(m_hg_class);

            if(err != HG_SUCCESS) {
                // can't throw here
                ERROR("Failed to shut down transport layer: {}\n",
                      HG_Error_to_string(err));
            }
        }
    }

    hg_return_t
    wait_on(const lookup_ctx& ctx) const {
        hg_return ret;
        unsigned int actual_count;

        while(!ctx.m_lookup_finished) {
            do {
                ret = HG_Trigger(m_hg_context,
                                0,
                                1,
                                &actual_count);
            } while((ret == HG_SUCCESS) &&
                    (actual_count != 0) &&
                    !ctx.m_lookup_finished);

            if(!ctx.m_lookup_finished) {
                ret = HG_Progress(m_hg_context,
                                  100);

                if(ret != HG_SUCCESS && ret != HG_TIMEOUT) {
                    WARNING("Unexpected return code {} from HG_Progress: {}",
                            ret, HG_Error_to_string(ret));
                    return ret;
                }
            }
        }

        return HG_SUCCESS;
    }

    endpoint
    lookup(const std::string& name,
           std::size_t port) const {

        /*const */auto address = make_address(name, port);

        {
            std::lock_guard<std::mutex> lock(m_addr_cache_mutex);

            const auto it = m_addr_cache.find(address);

            if(it != m_addr_cache.end()) {
                DEBUG("Endpoint \"{}\" cached {}", address, fmt::ptr(it->second.get()));
                return endpoint(name, port, it->second);
            }
        }

        address = "ofi+sockets://fi_sockaddr_in://127.0.0.1:22222";

        DEBUG("Looking up endpoint \"{}\"", address);

        assert(m_hg_class);
        assert(m_hg_context);

        lookup_ctx ctx(m_hg_class);

        hg_return_t ret = HG_Addr_lookup(
            // Mercury execution context
            const_cast<hg_context_t*>(m_hg_context),
            // pointer to callback
            [](const struct hg_cb_info* cbi) -> hg_return_t {

                auto* ctx = static_cast<lookup_ctx*>(cbi->arg);

                ctx->m_lookup_finished = true;
                ctx->m_hg_addr = cbi->info.lookup.addr;
                ctx->m_hg_ret = cbi->ret;

                if(cbi->ret != HG_SUCCESS) {
                    return cbi->ret;
                }

                return HG_SUCCESS;
            },
            // pointer to data passed to callback
            //static_cast<void*>(ctx.get()),
            static_cast<void*>(&ctx),
            // name to lookup
            address.c_str(),
            // pointer to returned operation ID
            HG_OP_ID_IGNORE);

        if(ret != HG_SUCCESS) {
            throw std::runtime_error("Failed to lookup target");
        }

        ret = wait_on(ctx);

        if(ret != HG_SUCCESS) {
            DEBUG("Lookup request failed");
            throw std::runtime_error("Failed to lookup target");
        }

        assert(ctx.m_lookup_finished);
        DEBUG("Lookup request succeeded [hg_addr: {}]",
              fmt::ptr(ctx.m_hg_addr));

        
        std::pair<decltype(m_addr_cache)::iterator, bool> rv;

        {
            std::lock_guard<std::mutex> lock(m_addr_cache_mutex);
            rv = m_addr_cache.emplace(
                    address,
                    std::make_shared<detail::address>(
                        m_hg_class, ctx.m_hg_addr));
        }

        return endpoint(name, port, rv.first->second);
    }

    endpoint_set
    lookup(std::initializer_list<
              std::pair<const std::string, const std::size_t>>&& addrs) const {


        std::set<
            std::pair<const std::string, const std::size_t> 
            > unique_addrs(addrs);

        endpoint_set endps;

        // TODO: this waits on each individual lookup. Make it so that
        // all lookups are posted to mercury and we only wait once on
        // total completion
        for(const auto addr : unique_addrs) {
            endps.add(lookup(addr.first, addr.second));
        }

        return endps;
    }

    template <typename Request, typename Callable>
    void
    register_handler(Callable&& handler) {

        const auto descriptor = 
            std::static_pointer_cast<detail::request_descriptor<Request>>(
                detail::registered_requests().at(Request::public_id));

        if(!descriptor) {
            throw std::runtime_error("Failed to register handler for request "
                                     "of unknown type");
        }

        descriptor->set_user_handler(std::forward<Callable>(handler));
    }

    /**
     * Starts the Mercury asynchronous engine
     */
    void
    run() {

        DEBUG("Starting Mercury asynchronous engine");

        assert(m_hg_class);
        assert(m_hg_context);

        m_runner = std::thread(&async_engine::progress_thread, this);
    }

    template <typename BufferSequence>
    exposed_memory
    expose(BufferSequence&& bufseq, 
           access_mode mode) {

        assert(m_hg_context);
        assert(m_hg_class);

        return {m_hg_class, mode, std::forward<BufferSequence>(bufseq)};
    }

#if 0
    /**
     * Construct an RPC.
     *
     * This is an asynchronous operation and it will return immediately.
     *
     * @param target_name [IN]  the target's name
     * @param target_port [IN]  the target's port
     */
    template <rpc ID>
    rpc_handle<ID>
    make_rpc(const endpoint_set& targets,
             const RpcInput<ID>& input) {

        DEBUG2("  Creating public RPC handle");

        std::vector<std::shared_ptr<detail::address>> addrs;
        std::transform(targets.begin(), targets.end(),
                       std::back_inserter(addrs),
                       [](const endpoint& endp) {
                           return endp.address();
                       });

        return {m_hg_context, addrs, input};
    }
#endif

    template <typename Request, typename EndpointSet, typename... Args>
    typename Request::handle_type
    broadcast(EndpointSet&& targets,
              Args&&... args) {

        using Input = typename Request::input_type;
        using Output = typename Request::output_type;
        using Handle = typename Request::handle_type;

        DEBUG2("Posting RPC to multiple endpoints");

        std::vector<std::shared_ptr<detail::address>> addrs;
        std::transform(targets.begin(), targets.end(),
                       std::back_inserter(addrs),
                       [](const endpoint& endp) {
                           return endp.address();
                       });

        auto handle = Handle(m_hg_context, 
                             addrs, 
                             Input(std::forward<Args>(args)...));

        // TODO: move to a private function in rpc_handle class
        std::size_t i = 0;

        for(const auto& ctx : handle.m_ctxs) {

            //    auto input = MercuryInput(ctx->m_input);
            hg_return ret = detail::post_to_mercury(ctx.get());

            // if this submission fails, cancel all previously posted RPCs
            if(ret != HG_SUCCESS) {
                for(std::size_t j = 0; j < i; ++j) {
                    ret = HG_Cancel(handle.m_ctxs[i]->m_handle);

                    if(ret != HG_SUCCESS) {
                        WARNING("Failed to cancel RPC: {}", 
                                HG_Error_to_string(ret));
                    }

                    ctx->m_status = detail::request_status::timeout;
                }

                throw std::runtime_error("Failed to post RPC: " + 
                        std::string(HG_Error_to_string(ret)));
            }
            
            ++i;
        }

        return handle;
        
    }

#if 0
    template <rpc ID, typename... Args>
    rpc_handle<ID>
    post(const endpoint_set& targets,
         Args&&... args) {

        DEBUG2("Posting RPC");

        std::vector<std::shared_ptr<detail::address>> addrs;
        std::transform(targets.begin(), targets.end(),
                       std::back_inserter(addrs),
                       [](const endpoint& endp) {
                           return endp.address();
                       });

        auto handle = 
            rpc_handle<ID>(m_hg_context, 
                           addrs, 
                           RpcInput<ID>(std::forward<Args>(args)...));

        // TODO: move to a private function in rpc_handle class
        std::size_t i = 0;

        for(const auto& ctx : handle.m_ctxs) {

            using MESSAGE = typename detail::rpc_descriptor<ID>::message_type;

            hg_return ret = 
                detail::executor<ID, MESSAGE>::post(ctx.get());

            // if this submission fails, cancel all previously posted RPCs
            if(ret != HG_SUCCESS) {
                for(std::size_t j = 0; j < i; ++j) {
                    ret = HG_Cancel(handle.m_ctxs[i]->m_handle);

                    if(ret != HG_SUCCESS) {
                        WARNING("Failed to cancel RPC: {}", 
                                HG_Error_to_string(ret));
                    }

                    ctx->m_status = detail::request_status::timeout;
                }

                throw std::runtime_error("Failed to post RPC: " + 
                        std::string(HG_Error_to_string(ret)));
            }
            
            ++i;
        }

        return handle;
    }


    template <rpc ID>
    __attribute__((deprecated))
    void
    post(const rpc_handle<ID>& handle) {

        std::size_t i = 0;

        for(const auto& ctx : handle.m_ctxs) {

            using MESSAGE = typename detail::rpc_descriptor<ID>::message_type;

            hg_return ret = 
                detail::executor<ID, MESSAGE>::post(ctx.get());

            // if this submission fails, cancel all previously posted RPCs
            if(ret != HG_SUCCESS) {
                for(std::size_t j = 0; j < i; ++j) {
                    (void) HG_Cancel(handle.m_ctxs[i]->m_handle);
                }

                throw std::runtime_error("Failed to post RPC");
            }
            
            i++;
        }
    }
#endif

    template <typename Input, typename Callable>
    void async_pull(exposed_memory&& src,
                    exposed_memory&& dst,
                    request<Input>&& req,
                    Callable&& user_callback) {

        hg_bulk_t local_bulk_handle = hg_bulk_t(dst);
        hg_bulk_t remote_bulk_handle = hg_bulk_t(src);

        assert(local_bulk_handle != HG_BULK_NULL);
        assert(remote_bulk_handle != HG_BULK_NULL);

        // We need to allow custom user callbacks, but the Mercury API 
        // restricts us in the prototypes that we can use. Since we don't
        // want users to bother with Mercury internals, // we register our own
        // completion_callback() lambda and we dynamically allocate
        // a transfer_context with all the required information that we
        // propagate through Mercury using the arg field in HG_Bulk_transfer().
        // Once our callback is invoked, we can unpack the information, delete
        // the transfer_context and invoke the actual user callback
        struct transfer_context {
            transfer_context(request<Input>&& req, 
                             exposed_memory&& src, 
                             exposed_memory&& dst,
                             Callable&& user_callback) :
                m_request(std::move(req)),
                m_src(src),
                m_dst(dst),
                m_user_callback(user_callback) { }

            ~transfer_context() {
                DEBUG("{}()", __func__);
            }

            request<Input> m_request;
            exposed_memory m_src;
            exposed_memory m_dst;
            // XXX For some reason, declaring m_user_callback as:
            //    Callable m_user_callback;
            // causes a weird bug with GCC 4.9 where any variables captured 
            // by value in the value get corrupted. Replacing it with 
            // std::function fixes this
            std::function<void(request<Input>&&)> m_user_callback;
        };

INFO("===== req.m_handle: {}", fmt::ptr(req.m_handle));

        auto* ctx =
            new transfer_context(std::move(req), 
                                 std::move(src),
                                 std::move(dst),
                                 user_callback);

        const auto completion_callback =
                [](const struct hg_cb_info* cbi) -> hg_return_t {

                    if(cbi->ret != HG_SUCCESS) {
                        DEBUG("Bulk transfer failed");
                        return cbi->ret;
                    }

                    // make sure that ctx is freed regardless of what might 
                    // happen in m_user_callback()
                    const auto ctx = 
                        std::unique_ptr<transfer_context>(
                                reinterpret_cast<transfer_context*>(cbi->arg));

INFO("===== req.m_handle: {}", fmt::ptr(ctx->m_request.m_handle));
                    ctx->m_user_callback(std::move(ctx->m_request));

                    return HG_SUCCESS;
                };

        detail::mercury_bulk_transfer(ctx->m_request.m_handle,
                                      remote_bulk_handle, 
                                      local_bulk_handle, 
                                      ctx,
                                      completion_callback);
    }

    template <typename Request, typename... Args>
    void
    respond(request<Request>&& req, 
            Args&&... args) const {

        using output_type = typename Request::output_type;
        using mercury_output_type = typename Request::mercury_output_type;

        detail::mercury_respond(
                std::move(req), 
                mercury_output_type(
                    output_type(std::forward<Args>(args)...)));
    }

private:
    std::string
    make_address(const std::string& target_name, 
                 const std::size_t target_port) const {
        return std::string(m_transport_prefix) +
                           target_name +
                           std::string(":") +
                           std::to_string(target_port);
    }

private:
    /**
     * Register RPCs into Mercury so that they can be called by the clients 
     * of the asynchronous engine
     */
    void
    register_rpcs() {

        assert(m_hg_class);
        assert(m_hg_context);

        for(auto&& kv : detail::registered_requests()) {

            auto&& id = kv.first;
            auto&& descriptor = kv.second;

            DEBUG("**** registered: {}, {}, {}, {}, {}", 
                    descriptor->m_id,
                    descriptor->m_mercury_id,
                    descriptor->m_name,
                    reinterpret_cast<void*>(descriptor->m_mercury_input_cb),
                    reinterpret_cast<void*>(descriptor->m_mercury_output_cb));

            detail::register_mercury_rpc(m_hg_class, descriptor, m_listen);
        }
    }

    /**
     * Dedicated thread that drives Mercury progress
     */
    void
    progress_thread() {

        assert(m_hg_class);
        assert(m_hg_context);

        hg_return ret;
        unsigned int actual_count;

        while(!m_shutdown) {
            do {
                ret = HG_Trigger(m_hg_context,
                                 0,
                                 1,
                                 &actual_count);

                DEBUG4("HG_Trigger(context={}, timeout={}, max_count={}, "
                       "actual_count={}) = {}", fmt::ptr(m_hg_context), 0, 1,
                       actual_count, HG_Error_to_string(ret));

            } while((ret == HG_SUCCESS) &&
                    (actual_count != 0) &&
                    !m_shutdown);

            if(!m_shutdown) {
                ret = HG_Progress(m_hg_context, 100);

                DEBUG4("HG_Progress(context={}, timeout={}) = {}", 
                       fmt::ptr(m_hg_context), 100, HG_Error_to_string(ret));

                if(ret != HG_SUCCESS && ret != HG_TIMEOUT) {
                    FATAL("Unexpected return code {} from HG_Progress: {}",
                          ret, HG_Error_to_string(ret));
                }
            }
        }
    }

    std::atomic<bool> m_shutdown;
    hg_class_t* m_hg_class;
    hg_context_t* m_hg_context;
    bool m_listen;
    const transport m_transport;
    const char* const m_transport_prefix;
    std::thread m_runner;

    mutable std::mutex m_addr_cache_mutex;
    mutable std::unordered_map<std::string, 
                               std::shared_ptr<detail::address>> m_addr_cache;
}; // class async_engine


namespace detail {

#if 0
template <rpc ID>
hg_return_t
post_lookup_request(const char* address, 
                    execution_context<ID>* ctx) {

    DEBUG2("post_lookup_request(address={}, ctx={})",
           address, fmt::ptr(ctx));

    DEBUG("Creating lookup request");

    DEBUG2("  HG_Addr_lookup(hg_context={}, cb={}, arg={}, addr=\"{}\")",
           fmt::ptr(ctx->m_hg_context), (void*) lookup_completion_callback,
           fmt::ptr(ctx), address);

    return HG_Addr_lookup(
            // Mercury execution context
            const_cast<hg_context_t*>(ctx->m_hg_context),
            // pointer to callback
            lookup_completion_callback,
            // pointer to data passed to callback
            ctx,
            // name to lookup
            address,
            // pointer to returned operation ID
            HG_OP_ID_IGNORE);
}

template <rpc ID>
hg_return_t
lookup_completion_callback(const struct hg_cb_info* cbi) {

    auto* ctx = static_cast<execution_context<ID>*>(cbi->arg);

    DEBUG("Processing lookup request [ctx: {}]",
            static_cast<void*>(ctx));
//
//    if(cbi->ret != HG_SUCCESS) {
//        DEBUG("Lookup request failed");
//        return cbi->ret;
//    }
//
//    switch(ctx->m_rpc_descriptor->m_type) {
//        case rpc_type::send_data:
//            return post_send_data_request(cbi->info.lookup.addr, ctx);
//        default:
//            DEBUG("Unexpected RPC type {}", 
//                    static_cast<int>(ctx->m_rpc_descriptor->m_type));
//    }
//
//    DEBUG("Lookup request succeeded");
    return HG_SUCCESS;
}
#endif

} // namespace detail
} // namespace hermes

#endif // __HERMES_ASYNC_ENGINE_HPP__
