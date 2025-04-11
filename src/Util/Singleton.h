#pragma once

#define SINGLETON(CLASS)                                                                                                                             \
public:                                                                                                                                              \
    static CLASS& instance() {                                                                                                                       \
        static CLASS sInstance;                                                                                                                      \
        return sInstance;                                                                                                                            \
    }                                                                                                                                                \
                                                                                                                                                     \
private:                                                                                                                                             \
    CLASS() = default;                                                                                                                               \
    CLASS(const CLASS&) = delete;                                                                                                                    \
    void operator=(const CLASS&) = delete;

#define SINGLETON_CUSTOM(CLASS)                                                                                                                      \
public:                                                                                                                                              \
    static CLASS& instance() {                                                                                                                       \
        static CLASS sInstance;                                                                                                                      \
        return sInstance;                                                                                                                            \
    }                                                                                                                                                \
                                                                                                                                                     \
private:                                                                                                                                             \
    CLASS();                                                                                                                                         \
    CLASS(const CLASS&) = delete;                                                                                                                    \
    void operator=(const CLASS&) = delete;
