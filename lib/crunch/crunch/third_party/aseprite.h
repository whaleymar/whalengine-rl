#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <vector>

// NOTE: Aseprite parser class written using the file specs: https://github.com/aseprite/aseprite/blob/main/docs/ase-file-specs.md#header,
// and referencing NoelFB's C# aseprite parser: https://gist.github.com/NoelFB/778d190e5d17f1b86ebf39325346fcc5.

namespace ase {

struct Color {
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t A;

    void print() const;
};

class BufferReader {
    uint mReadCounter;
    char* mBuffer;

public:
    BufferReader(char* __buffer) {
        mBuffer = __buffer;
        mReadCounter = 0;
    }

    // Counter getter
    uint readCount() { return mReadCounter; }

    // Funcs clean up and match aseprite docs
    void BYTE(uint8_t* __dest);
    void SHORT(int16_t* __dest);
    void DWORD(uint32_t* __dest);
    void WORD(uint16_t* __dest);
    void STRING(std::vector<char>* __str);
    void PIXEL(std::vector<Color>* __dest, int __len);
    void _U_CHAR_ARR(unsigned char __start[], int __len);
    void SEEK(uint __jump) { mReadCounter += __jump; }
    void DECODE(std::vector<Color>* __dest, int __in_len, int __out_len);
};

struct UserData {
    std::vector<char> text;
    Color color;
};

struct Tag {
    enum LoopDirections { Forward = 0, Reverse = 1, PingPong = 2 };

    std::vector<char> name;
    LoopDirections loopDirection;
    uint16_t from;
    uint16_t to;
    UserData userData;
};

class Layer {
    // Tileset type no supported
public:
    enum Flags : uint16_t { Visible = 1, Editable = 2, LockMovement = 4, Background = 8, PreferLinkedCels = 16, Collapsed = 32, Reference = 64 };

    enum Types { Normal = 0, Group = 1, Tilemap = 2 };
    Flags flags;
    Types type;
    uint16_t childLevel;
    uint16_t blendMode;
    float opacity;
    std::vector<char> name;

    UserData userData;

    bool isOn(Flags flag) const { return (flags & flag) > 0; }
};

class Cel {
public:
    Layer layer;
    std::vector<Color> pixels;

    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    float opacity;

    UserData userData;
};

class Frame {
public:
    // Header struct
    struct Header {
        uint32_t size;  // In bytes
        uint16_t magicNumber;
        uint16_t chunkCountOld;
        uint16_t frameDuration;
        // 2 Bytes of unused data that is set to zero goes here
        uint32_t chunkCountNew;
    };

    Header header;

    std::vector<Cel> celArray;

    Frame() {}

    std::vector<Color> getRaster(int rasterWidth, int rasterHeight) const;
};

class Aseprite {
public:
    // Header struct
    struct Header {
        uint32_t fileSize;
        uint16_t magicNumber;  // TODO: add a check to make sure this is (0xA5E0)
        uint16_t frameCount;
        uint16_t width;
        uint16_t height;
        uint16_t colorDepth;
        // there are 114 bytes left in the header that we dont use
    };

    Header header;
    std::vector<Frame> frameArray;
    std::vector<Layer> layerArray;
    std::vector<Tag> tagArray;

    enum Chunks {
        OldPaletteA = 0x0004,
        OldPaletteB = 0x0011,
        FrameLayer = 0x2004,
        FrameCel = 0x2005,
        CelExtra = 0x2006,
        ColorProfile = 0x2007,
        Mask = 0x2016,
        Path = 0x2017,
        FrameTags = 0x2018,
        Palette = 0x2019,
        ChunkUserData = 0x2020,
        Slice = 0x2022
    };

    Aseprite(const char* filePath);
};

struct Animation {
    std::string name;
    std::vector<float> frameDurations;

    void addFrame(const Frame& frame);
    void saveXml(std::ofstream& xml) const;
};

}  // namespace ase
