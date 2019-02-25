#ifndef __HERMES_HANDLE_HPP__
#define __HERMES_HANDLE_HPP__

// C includes
#include <mercury.h>

// C++ includes
#include <type_traits>
#include <vector>
#include <memory>
#include <future>
#include <cstdint>
#include <numeric>

// project includes
#if __cplusplus == 201103L
#include <hermes/make_unique.hpp>
#endif // __cplusplus == 201103L


namespace hermes {

// defined in this file
template <typename Request>
class rpc_handle;

// defined elsewhere
class async_engine;

namespace detail {

struct address;
template <typename Request> struct execution_context;

} // namespace detail

template <typename Request>
class rpc_handle {

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
    using ExecutionContext = detail::execution_context<Request>;

    // XXX: we use SFINAE to make sure that the type of the input 
    // is Request::input_type
    template <typename InputData,
              typename Enable = typename 
                  std::is_same<InputData, 
                               typename Request::input_type>::type>
    rpc_handle(const hg_context_t * const hg_context,
               const std::vector<std::shared_ptr<detail::address>>& targets,
               InputData&& input) {

        m_ctxs.reserve(targets.size());
        m_futures.reserve(targets.size());

        for(auto&& addr : targets) {
            HERMES_DEBUG2("Creating execution_context for RPC");

            m_ctxs.emplace_back(
                compat::make_unique<ExecutionContext>(
                    this, 
                    hg_context, 
                    addr, 
                    std::forward<InputData>(input)));

            HERMES_DEBUG2("Creating future for RPC");

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
        HERMES_DEBUG2("{}()", __func__);

        if(Request::requires_response) {
            (void) get();
        }
    }

    std::vector<Output>
    get() const {

        if(!Request::requires_response) {
            throw std::runtime_error("This request type does not expect a "
                                     "response");
        }

        assert(m_futures.size() == m_ctxs.size());

        HERMES_DEBUG("Getting RPC results (pending: {})", m_futures.size());

        constexpr const auto TIMEOUT = std::chrono::seconds(100);
        constexpr const auto RETRIES = 0;

        std::vector<Output> result_set;
        std::vector<bool> retrieved(m_futures.size(), false);
        std::vector<std::uint8_t> retries(m_futures.size(), RETRIES);

        std::size_t pending_requests = 
            std::accumulate(m_futures.begin(),
                            m_futures.end(),
                            0,
                            [](std::size_t prev, 
                               const std::future<Output>& fut) -> std::size_t {
                                return prev + (fut.valid() ? 1 : 0);
                            });

        while(pending_requests > 0) {

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

                        HERMES_DEBUG2("Mercury request timed out, {}",
                                      m_ctxs[i]->m_status == 
                                          detail::request_status::timeout ?
                                          fmt::format(
                                              "reposting request ({} of {})", 
                                              RETRIES - retries[i], RETRIES) :
                                          "cancelling");

                        // cancel the pending Mercury request, this causes
                        // its completion callback to be invoked and we can
                        // use it to either repost the request or make it 
                        // gracefully dispose of itself depending on the value
                        // of request_status
                        hg_return_t ret = HG_Cancel(m_ctxs[i]->m_handle);

                        if(ret != HG_SUCCESS) {
                            HERMES_WARNING("Failed to cancel RPC: {}", 
                                           HG_Error_to_string(ret));
                        }
                        break;
                    }

                    case std::future_status::ready:
                    {
                        // the request "completed", i.e. we can retrieve either
                        // a valid result or an error condition
                        HERMES_DEBUG2("RPC completed. Retrieving result.");
                        result_set.emplace_back(m_futures[i].get());
                        retrieved[i] = true;
                        --pending_requests;
                        break;
                    }

                    default:
                    {
                        HERMES_DEBUG2("RPC future is in an inconsistent state. "
                                      "Aborting.");
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

} // namespace hermes


#endif // __HERMES_HANDLE_HPP__

