#pragma once

#include <ozo/asio.h>

#include <boost/asio/dispatch.hpp>

namespace ozo::detail {
/**
 * @brief Safely wraps handler with a given Executor
 *
 * Comparing to asio::bind_executor this object dispatches handler to its
 * associated executor.
 */
template <typename Executor, typename Handler>
struct wrap_executor {
    Executor ex;
    Handler handler;

    wrap_executor(const Executor& ex, Handler handler)
    : ex(ex), handler(std::move(handler)) {}

    template <typename ...Args>
    void operator() (Args&& ...args) {
        auto bound = detail::bind(std::move(handler), std::forward<Args>(args)...);
        auto bound_ex = detail::resolve_asio_executor(asio::get_associated_executor(bound));
        asio::dispatch(bound_ex, [bound = std::move(bound)]() mutable {
            bound();
        });
    }

    using executor_type = Executor;

    executor_type get_executor() const noexcept { return ex;}

    using allocator_type = asio::associated_allocator_t<Handler>;

    allocator_type get_allocator() const noexcept { return asio::get_associated_allocator(handler);}
};

template <typename Executor, typename Handler>
wrap_executor(const Executor&, Handler) -> wrap_executor<Executor, Handler>;

} // namespace ozo::detail
