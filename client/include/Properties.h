#pragma once

#include <json/json.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <type_traits>

#include "json/value.h"

class Properties {
    static std::unique_ptr<Json::Value> root;
    static std::filesystem::path path;

   public:
    Properties() = delete;
    Properties(const Properties&) = delete;
    Properties& operator=(const Properties&) = delete;
    Properties(Properties&&) = delete;
    Properties& operator=(Properties&&) = delete;

    static const Json::Value& readProperties() {
        if (root == nullptr) {
            Json::CharReaderBuilder readerBuilder;
            readerBuilder["emitUTF8"] = true;
            root = std::make_unique<Json::Value>();

            std::ifstream ifs(path);
            if (ifs.fail()) {
                throw std::runtime_error("Failed to open config file");
            }
            if (!Json::parseFromStream(readerBuilder, ifs, root.get(),
                                       nullptr)) {
                throw std::runtime_error("Failed to parse config file");
            }
        }
        return *root;
    }

    static void writeProperties(const std::filesystem::path& path,
                                const Json::Value& value) {
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["emitUTF8"] = true;
        Json::StreamWriter* writer = writerBuilder.newStreamWriter();
        std::ofstream ofs(path);
        if (ofs.fail()) {
            throw std::runtime_error("Failed to open config file");
        }
        if (writer->write(value, &ofs)) {
            throw std::runtime_error("Failed to write config file");
        }
    }
};
inline std::unique_ptr<Json::Value> Properties::root = nullptr;
inline std::filesystem::path Properties::path = "config.json";
