#include <json/json.h>

#include <fstream>

#include "json/value.h"

class Properties {
    Json::CharReaderBuilder readerBuilder;
    Json::Value root;

   public:
    Properties() { readerBuilder["emitUTF8"] = true; }

    Json::Value readProperties() {
        std::ifstream ifs("config.json");
        if (!Json::parseFromStream(readerBuilder, ifs, &root, nullptr)) {
            throw std::runtime_error("Failed to parse config file");
        }
        return root;
    }
};
