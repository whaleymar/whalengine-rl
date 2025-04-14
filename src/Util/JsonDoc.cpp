#include "Util/JsonDoc.h"
#include <cassert>
#include <yyjson.h>

bool JsonValue::isNull() const {
    return !mVal || yyjson_is_null(mVal);
}

bool JsonValue::isBool() const {
    return mVal && yyjson_is_bool(mVal);
}

bool JsonValue::isInt() const {
    return mVal && yyjson_is_int(mVal);
}

bool JsonValue::isFloat() const {
    return mVal && yyjson_is_real(mVal);
}

bool JsonValue::isString() const {
    return mVal && yyjson_is_str(mVal);
}

bool JsonValue::isArray() const {
    return mVal && yyjson_is_arr(mVal);
}

bool JsonValue::isObject() const {
    return mVal && yyjson_is_obj(mVal);
}

bool JsonValue::getBoolOr(bool defaultValue) const {
    return mVal && yyjson_is_bool(mVal) ? yyjson_get_bool(mVal) : defaultValue;
}

s32 JsonValue::getIntOr(s32 defaultValue) const {
    return mVal && yyjson_is_int(mVal) ? yyjson_get_int(mVal) : defaultValue;
}

f64 JsonValue::getFloatOr(f64 defaultValue) const {
    return mVal && yyjson_is_real(mVal) ? yyjson_get_real(mVal) : defaultValue;
}

std::string JsonValue::getStringOr(const std::string& defaultValue) const {
    return mVal && yyjson_is_str(mVal) ? yyjson_get_str(mVal) : defaultValue;
}

bool JsonValue::getBool() const {
    assert(mVal && yyjson_is_bool(mVal));
    return unsafe_yyjson_get_bool(mVal);
}

s32 JsonValue::getInt() const {
    assert(mVal && yyjson_is_int(mVal));
    return unsafe_yyjson_get_int(mVal);
}

f64 JsonValue::getFloat() const {
    assert(mVal && yyjson_is_real(mVal));
    return unsafe_yyjson_get_real(mVal);
}

f64 JsonValue::getNumber() const {
    assert(mVal && yyjson_is_real(mVal) || yyjson_is_int(mVal));
    if (yyjson_is_real(mVal)) {
        return unsafe_yyjson_get_real(mVal);
    } else {
        return unsafe_yyjson_get_int(mVal);
    }
}

std::string JsonValue::getString() const {
    assert(mVal && yyjson_is_str(mVal));
    return unsafe_yyjson_get_str(mVal);
}

bool JsonValue::contains(const std::string& key) const {
    return mVal && yyjson_is_obj(mVal) && yyjson_obj_get(mVal, key.c_str()) != nullptr;
}

JsonValue JsonValue::operator[](const std::string& key) const {
    if (!mVal || !yyjson_is_obj(mVal)) {
        return JsonValue(nullptr);
    }
    return JsonValue(yyjson_obj_get(mVal, key.c_str()));
}

// Access array element by index
JsonValue JsonValue::operator[](u64 idx) const {
    if (!mVal || !yyjson_is_arr(mVal)) {
        return JsonValue(nullptr);
    }
    return JsonValue(yyjson_arr_get(mVal, idx));
}

// Get array size
u64 JsonValue::size() const {
    if (!mVal || !yyjson_is_arr(mVal)) {
        return 0;
    }
    return yyjson_arr_size(mVal);
}

JsonValue::ArrayIterator::ArrayIterator(yyjson_val* arr, bool end) : mArr(arr), mIdx(end ? yyjson_arr_size(arr) : 0), mMax(yyjson_arr_size(arr)) {}

bool JsonValue::ArrayIterator::operator!=(const ArrayIterator& other) const {
    return mIdx != other.mIdx;
}

JsonValue::ArrayIterator& JsonValue::ArrayIterator::operator++() {
    if (mIdx < mMax)
        ++mIdx;
    return *this;
}

JsonValue JsonValue::ArrayIterator::operator*() const {
    return JsonValue(yyjson_arr_get(mArr, mIdx));
}

JsonValue::ArrayIterator JsonValue::begin() const {
    return ArrayIterator(mVal);
}

JsonValue::ArrayIterator JsonValue::end() const {
    return ArrayIterator(mVal, true);
}

yyjson_api_inline bool whal_yyjson_obj_iter_init(yyjson_val* obj, JsonObjIter* iter) {
    if (yyjson_likely(yyjson_is_obj(obj) && iter)) {
        iter->idx = 0;
        iter->max = unsafe_yyjson_get_len(obj);
        iter->cur = unsafe_yyjson_get_first(obj);
        iter->obj = obj;
        return true;
    }
    if (iter)
        memset(iter, 0, sizeof(yyjson_obj_iter));
    return false;
}

yyjson_api_inline yyjson_val* whal_yyjson_obj_iter_next(JsonObjIter* iter) {
    if (iter && iter->idx < iter->max) {
        yyjson_val* key = iter->cur;
        iter->idx++;
        iter->cur = unsafe_yyjson_get_next(key + 1);
        return key;
    }
    return NULL;
}

JsonValue::ObjectIterator::ObjectIterator(yyjson_val* obj, bool end) : mIdx(0), mMax(obj ? yyjson_obj_size(obj) : 0), mEnd(end) {
    if (!end && obj && yyjson_obj_size(obj) > 0) {
        whal_yyjson_obj_iter_init(obj, &mIter);
        // Get the first key-value pair
        mCurrentKey = whal_yyjson_obj_iter_next(&mIter);
        if (mCurrentKey) {
            mCurrentVal = yyjson_obj_iter_get_val(mCurrentKey);
        }
    } else {
        mEnd = true;
    }
}

bool JsonValue::ObjectIterator::operator!=(const ObjectIterator& other) const {
    return mEnd != other.mEnd;
}

JsonValue::ObjectIterator& JsonValue::ObjectIterator::operator++() {
    mCurrentKey = whal_yyjson_obj_iter_next(&mIter);
    if (mCurrentKey) {
        mCurrentVal = yyjson_obj_iter_get_val(mCurrentKey);
        mIdx++;
    } else {
        mEnd = true;
    }
    return *this;
}

JsonKVPair JsonValue::ObjectIterator::operator*() const {
    if (mCurrentKey && mCurrentVal) {
        return {yyjson_get_str(mCurrentKey), JsonValue(mCurrentVal)};
    }
    return {"", JsonValue(nullptr)};
}

JsonValue::ObjectIterator JsonValue::beginObject() const {
    return ObjectIterator(mVal);
}

JsonValue::ObjectIterator JsonValue::endObject() const {
    return ObjectIterator(mVal, true);
}

JsonDoc::~JsonDoc() {
    if (mDoc)
        yyjson_doc_free(mDoc);
}

JsonDoc::JsonDoc(JsonDoc&& other) noexcept : mDoc(other.mDoc), mData(std::move(other.mData)) {
    other.mDoc = nullptr;
}

JsonDoc& JsonDoc::operator=(JsonDoc&& other) noexcept {
    if (this != &other) {
        if (mDoc)
            yyjson_doc_free(mDoc);
        mDoc = other.mDoc;
        mData = std::move(other.mData);
        other.mDoc = nullptr;
    }
    return *this;
}

bool JsonDoc::load(const char* path) {
    mDoc = yyjson_read_file(path, 0, NULL, NULL);
    return !isValid();
}

JsonValue JsonDoc::getRoot() const {
    return mDoc ? JsonValue(yyjson_doc_get_root(mDoc)) : JsonValue(nullptr);
}

JsonDoc JsonDoc::fromFile(const std::string& filePath) {
    JsonDoc doc;
    doc.mDoc = yyjson_read_file(filePath.c_str(), 0, NULL, NULL);
    return doc;
}

JsonDoc JsonDoc::fromString(const std::string& jsonStr) {
    JsonDoc doc;
    doc.mData = jsonStr;
    u64 len = jsonStr.length();
    doc.mDoc = yyjson_read(doc.mData.c_str(), len, 0);
    return doc;
}
