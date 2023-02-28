#pragma once

#include <spdlog/spdlog.h>

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/wincolor_sink.h"

using namespace spdlog;

class LoggerRegister {
    std::vector<spdlog::sink_ptr> sinks;
    std::shared_ptr<logger> logger;

    void initConSoleLogger() {
        auto sink = std::make_shared<sinks::wincolor_stdout_sink_mt>();
        sink->set_level(level::trace);
        sinks.push_back(sink);
    }
    void initFileLogger() {
        auto sink = std::make_shared<sinks::rotating_file_sink_mt>(
            "log/log", 1024 * 1024 * 10, 100);
        sink->set_level(level::trace);
        sinks.push_back(sink);
    }
    void initLogger() {
        initConSoleLogger();
        initFileLogger();
        logger = std::make_shared<spdlog::logger>("logger", begin(sinks),
                                                  end(sinks));
        register_logger(logger);
    }

   public:
    LoggerRegister() {
        flush_every(std::chrono::seconds(1));
        flush_on(level::debug);
        initLogger();
    }

    std::shared_ptr<spdlog::logger> getLogger() {
        auto ptr = spdlog::get("logger");
        if (ptr == nullptr)
            throw std::runtime_error("cant get logger,logger is nullptr");
        return ptr;
    }

    void setLevel(const std::string& level) {
        if (level == "trace") {
            set_level(spdlog::level::trace);
        } else if (level == "debug") {
            set_level(spdlog::level::debug);
        } else if (level == "info") {
            set_level(spdlog::level::info);
        } else if (level == "warn") {
            set_level(spdlog::level::warn);
        } else if (level == "err") {
            set_level(spdlog::level::err);
        } else if (level == "critical") {
            set_level(spdlog::level::critical);
        } else if (level == "off") {
            set_level(spdlog::level::off);
        } else {
            set_level(spdlog::level::info);
        }
    }
};
