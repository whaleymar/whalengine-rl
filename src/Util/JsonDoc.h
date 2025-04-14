#pragma once

#include <string>
#include "Util/IResource.h"
#include "Util/Types.h"

class JsonDoc;
struct JsonKVPair;
typedef struct yyjson_val yyjson_val;
typedef struct yyjson_doc yyjson_doc;

struct JsonObjIter {
    u64 idx;         /**< next key's index */
    u64 max;         /**< maximum key index (obj.size) */
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

    bool getBoolOr(bool defaultValue = false) const;
    s32 getIntOr(s32 defaultValue = 0) const;
    f64 getFloatOr(f64 defaultValue = 0.0) const;
    std::string getStringOr(const std::string& defaultValue = "") const;
    bool getBool() const;
    s32 getInt() const;
    f64 getFloat() const;
    f64 getNumber() const;  // works for floats and ints
    std::string getString() const;
    bool contains(const std::string& key) const;

    // Access object property by key
    JsonValue operator[](const std::string& key) const;

    // Access array element by index
    JsonValue operator[](u64 idx) const;

    bool operator()() const { return mVal != nullptr; }

    // Get array size
    u64 size() const;

    // Helper for array iteration
    class ArrayIterator {
    public:
        explicit ArrayIterator(yyjson_val* arr, bool end = false);

        bool operator!=(const ArrayIterator& other) const;
        ArrayIterator& operator++();
        JsonValue operator*() const;

    private:
        yyjson_val* mArr;
        u64 mIdx;
        u64 mMax;
    };

    ArrayIterator begin() const;
    ArrayIterator end() const;

    // Helper for object iteration
    class ObjectIterator {
    public:
        explicit ObjectIterator(yyjson_val* obj, bool end = false);

        bool operator!=(const ObjectIterator& other) const;
        ObjectIterator& operator++();
        JsonKVPair operator*() const;

    private:
        // yyjson_obj_iter mIter;
        JsonObjIter mIter;
        u64 mIdx;
        u64 mMax;
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

struct JsonKVPair {
    std::string key;
    JsonValue value;
};

class JsonDoc : public IResource {
public:
    JsonDoc() = default;
    virtual ~JsonDoc();

    JsonDoc(JsonDoc&& other) noexcept;

    JsonDoc& operator=(JsonDoc&& other) noexcept;

    // No copy
    JsonDoc(const JsonDoc&) = delete;
    JsonDoc& operator=(const JsonDoc&) = delete;

    JsonValue operator*() const { return getRoot(); }

    bool load(const char* path) override;

    bool isValid() const { return mDoc != nullptr; }

    JsonValue getRoot() const;
    static JsonDoc fromFile(const std::string& filePath);
    static JsonDoc fromString(const std::string& jsonStr);

private:
    yyjson_doc* mDoc = nullptr;
    std::string mData;
};
