#pragma once

class IResource {
    virtual bool load(const char* path) = 0;  // returns true if an error occurred
};
