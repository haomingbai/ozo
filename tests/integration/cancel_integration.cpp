#include <ozo/connection_info.h>
#include <ozo/cancel.h>
#include <ozo/execute.h>
#include <ozo/shortcuts.h>

#include <future>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define ASSERT_REQUEST_OK(ec, conn)\
    ASSERT_FALSE(ec) << ec.message() \
        << "|" << ozo::error_message(conn) \
        << "|" << ozo::get_error_context(conn) << std::endl

namespace {

namespace hana = boost::hana;

using namespace testing;

TEST(cancel, should_cancel_operation) {
    using namespace ozo::literals;
    using namespace std::chrono_literals;
    using namespace hana::literals;

    ozo::io_context io;
    auto timer = ozo::detail::get_operation_timer(io.get_executor());
    auto result = std::make_shared<std::promise<ozo::error_code>>();
    auto future = result->get_future();
    const ozo::connection_info conn_info(OZO_PG_TEST_CONNINFO);

    ozo::get_connection(conn_info[io], [&io, &timer, result](ozo::error_code ec, auto conn) mutable {
        if (ec) {
            result->set_value(ec);
            return;
        }

        timer.expires_after(1s);
        timer.async_wait([&io, handle = get_cancel_handle(conn)](ozo::error_code ec) mutable {
            if (!ec) {
                // Guard is needed since cancel will be served with external
                // system executor, so we need to preserve our io_context from
                // stop until all the operation processed properly
                auto guard = boost::asio::make_work_guard(io);
                ozo::cancel(std::move(handle), io, 5s,
                    [guard = std::move(guard)](ozo::error_code, std::string) mutable {});
            }
        });
        ozo::execute(conn, "SELECT pg_sleep(1000000)"_SQL,
            [result](ozo::error_code ec, auto) mutable {
                result->set_value(ec);
            });
    });

    io.run();
    EXPECT_EQ(future.get(), ozo::sqlstate::query_canceled);
}

TEST(cancel, should_stop_cancel_operation_on_zero_timeout) {
    using namespace ozo::literals;
    using namespace std::chrono_literals;
    using namespace hana::literals;

    ozo::io_context io;
    ozo::io_context dummy_io;
    auto timer = ozo::detail::get_operation_timer(io.get_executor());
    const ozo::connection_info conn_info(OZO_PG_TEST_CONNINFO);
    auto execute_result = std::make_shared<std::promise<ozo::error_code>>();
    auto execute_future = execute_result->get_future();
    auto cancel_result = std::make_shared<std::promise<ozo::error_code>>();
    auto cancel_future = cancel_result->get_future();

    ozo::get_connection(conn_info[io], [&io, &timer, &dummy_io, execute_result, cancel_result]
            (ozo::error_code ec, auto conn) mutable {
        if (ec) {
            execute_result->set_value(ec);
            cancel_result->set_value(ec);
            return;
        }

        timer.expires_after(1s);
        timer.async_wait([&io, cancel_result, handle = get_cancel_handle(conn, dummy_io.get_executor())](ozo::error_code ec) mutable {
            if (ec) {
                cancel_result->set_value(ec);
                return;
            }
            // Guard is needed since cancel will be served with external
            // system executor, so we need to preserve our io_context from
            // stop until all the operation processed properly
            auto guard = boost::asio::make_work_guard(io);
            ozo::cancel(std::move(handle), io, 0s,
                [cancel_result, guard = std::move(guard)](ozo::error_code ec, std::string) mutable {
                    cancel_result->set_value(ec);
                });
        });
        ozo::execute(conn, "SELECT pg_sleep(1000000)"_SQL, 2s,
            [execute_result](ozo::error_code ec, auto) mutable {
                execute_result->set_value(ec);
            });
    });

    io.run();
    EXPECT_EQ(execute_future.get(), boost::asio::error::timed_out);
    EXPECT_EQ(cancel_future.get(), boost::asio::error::timed_out);
}

} // namespace
