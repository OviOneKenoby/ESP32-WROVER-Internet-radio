#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>

class Preferences {
public:
    inline static std::map<std::string, std::map<std::string, std::string>> data;
    inline static size_t writes = 0;

    bool begin(const char* name, bool = false) {
        current = name ? name : "";
        return true;
    }
    void end() { current.clear(); }

    uint8_t getUChar(const char* key, uint8_t fallback = 0) const {
        const auto value = find(key);
        return value ? static_cast<uint8_t>(std::stoul(*value)) : fallback;
    }
    size_t putUChar(const char* key, uint8_t value) {
        data[current][key] = std::to_string(value);
        ++writes;
        return 1;
    }
    size_t getString(const char* key, char* destination, size_t maximum) const {
        if (!destination || maximum == 0) return 0;
        const auto value = find(key);
        if (!value) {
            destination[0] = '\0';
            return 0;
        }
        const size_t copied = std::min(maximum - 1, value->size());
        std::memcpy(destination, value->data(), copied);
        destination[copied] = '\0';
        return copied;
    }
    size_t putString(const char* key, const char* value) {
        data[current][key] = value ? value : "";
        ++writes;
        return data[current][key].size();
    }

    static void clear() { data.clear(); writes = 0; }
    static void seed(const char* space, const char* key, const char* value) {
        data[space][key] = value;
    }

private:
    const std::string* find(const char* key) const {
        const auto space = data.find(current);
        if (space == data.end()) return nullptr;
        const auto value = space->second.find(key);
        return value == space->second.end() ? nullptr : &value->second;
    }

    std::string current;
};
