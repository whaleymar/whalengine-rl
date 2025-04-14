#pragma once

#include <string>

class JsonDoc;
struct KeyValuePair;
typedef struct yyjson_val yyjson_val;
typedef struct yyjson_doc yyjson_doc;

struct JsonObjIter {
    size_t idx;      /**< next key's index */
    size_t max;      /**< maximum key index (obj.size) */
    yyjson_val* cur; /**< next key */
    yyjson_val* obj; /**< the object being iterated */
};

class JsonValue {
public:
    JsonValue() = default;

    bool isNull() const;
    bool isBool() const;
    bool isInt() const;
    bool isFloat() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    bool getBool(bool defaultValue = false) const;
    int64_t getInt(int64_t defaultValue = 0) const;
    double getFloat(double defaultValue = 0.0) const;
    std::string getString(const std::string& defaultValue = "") const;
    bool contains(const std::string& key) const;

    // Access object property by key
    JsonValue operator[](const std::string& key) const;

    // Access array element by index
    JsonValue operator[](size_t idx) const;

    // Get array size
    size_t size() const;

    // Helper for array iteration
    class ArrayIterator {
    public:
        explicit ArrayIterator(yyjson_val* arr, bool end = false);

        bool operator!=(const ArrayIterator& other) const;
        ArrayIterator& operator++();
        JsonValue operator*() const;

    private:
        yyjson_val* mArr;
        size_t mIdx;
        size_t mMax;
    };

    ArrayIterator begin() const;
    ArrayIterator end() const;

    // Helper for object iteration
    class ObjectIterator {
    public:
        explicit ObjectIterator(yyjson_val* obj, bool end = false);

        bool operator!=(const ObjectIterator& other) const;
        ObjectIterator& operator++();
        KeyValuePair operator*() const;

    private:
        // yyjson_obj_iter mIter;
        JsonObjIter mIter;
        size_t mIdx;
        size_t mMax;
        bool mEnd;

        yyjson_val* mCurrentKey = nullptr;
        yyjson_val* mCurrentVal = nullptr;
    };

    ObjectIterator beginObject() const;
    ObjectIterator endObject() const;

private:
    yyjson_val* mVal = nullptr;

    friend class JsonDoc;
    explicit JsonValue(yyjson_val* val) : mVal(val) {}
};

struct KeyValuePair {
    std::string key;
    JsonValue value;
};

class JsonDoc {
public:
    JsonDoc() = default;
    ~JsonDoc();

    JsonDoc(JsonDoc&& other) noexcept;

    JsonDoc& operator=(JsonDoc&& other) noexcept;

    // No copy
    JsonDoc(const JsonDoc&) = delete;
    JsonDoc& operator=(const JsonDoc&) = delete;

    bool isValid() const { return mDoc != nullptr; }

    JsonValue getRoot() const;
    static JsonDoc fromFile(const std::string& filePath);
    static JsonDoc fromString(const std::string& jsonStr);

private:
    yyjson_doc* mDoc = nullptr;
    std::string mData;
};
