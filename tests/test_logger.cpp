#include <catch2/catch_test_macros.hpp>
#include <logger/logger.hpp>

class TestSink : public Logger::Sink {
   public:
    void log(const Logger::LogDetails& details) override {
        last_message = details.message;
        last_level = details.level;
        call_count++;
    }
    std::string last_message;
    Logger::LogLevel last_level;
    int call_count = 0;
};

TEST_CASE("Logger levels and filtering", "[logger]") {
    auto& logger = Logger::Logger::instance();
    logger.reset();
    logger.clear_sinks();

    auto test_sink = std::make_shared<TestSink>();
    logger.add_sink(test_sink);

    SECTION("Basic logging works") {
        logger.set_level(Logger::LogLevel::Debug);
        LOG_INFO("Hello Test");
        CHECK(test_sink->last_message == "Hello Test");
        CHECK(test_sink->last_level == Logger::LogLevel::Info);
        CHECK(test_sink->call_count == 1);
    }

    SECTION("Level filtering stops low-level logs") {
        logger.set_level(Logger::LogLevel::Error);
        LOG_INFO("This should be filtered");
        CHECK(test_sink->call_count == 0);

        LOG_ERROR("This should pass");
        CHECK(test_sink->call_count == 1);
        CHECK(test_sink->last_message == "This should pass");
    }

    SECTION("Global level None disables everything") {
        logger.set_level(Logger::LogLevel::None);
        LOG_CRITICAL("Even this is hidden");
        CHECK(test_sink->call_count == 0);
    }
}

TEST_CASE("Multiple sinks", "[logger]") {
    auto& logger = Logger::Logger::instance();
    logger.reset();
    logger.clear_sinks();

    auto sink1 = std::make_shared<TestSink>();
    auto sink2 = std::make_shared<TestSink>();

    logger.add_sink(sink1);
    logger.add_sink(sink2);

    LOG_INFO("Broadcast");

    CHECK(sink1->call_count == 1);
    CHECK(sink2->call_count == 1);
    CHECK(sink1->last_message == "Broadcast");
    CHECK(sink2->last_message == "Broadcast");
}
