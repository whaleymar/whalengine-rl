#include "aseprite.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <zlib.h>

// NOTE: builds with: g++ aseprite.cpp -o build -std=c++11 -lz

namespace ase {

void Color::print() const {
    printf("{R,G,B,A} = {%u, %u, %u, %u}. \n", R, G, B, A);
}

Aseprite::Aseprite(const char* filePath) {
    FILE* file;
    long fileSize;
    char* buffer;
    size_t result;

    // Open file
    file = fopen(filePath, "rb");  // mode = "rb" means read (r) and binary file (b)
    if (file == NULL) {
        fputs("File error at aseprite", stderr);
        exit(1);
    }

    // File length
    fseek(file, 0, SEEK_END);  // find end
    fileSize = ftell(file);    // how many bytes between file start and us (we at end)
    fseek(file, 0, SEEK_SET);  // back to start

    // Allocate buffer
    buffer = (char*)malloc(sizeof(char) * fileSize);
    if (buffer == NULL) {
        fputs("Memory error at aseprite", stderr);
        exit(2);
    }

    // Read into buffer
    result = fread(buffer, 1, fileSize, file);
    if (result != static_cast<size_t>(fileSize)) {
        fputs("Reading error at aseprite", stderr);
        exit(3);
    }

    // Copy relevent data to header struct
    BufferReader br = BufferReader(buffer);

    br.DWORD(&header.fileSize);
    br.WORD(&header.magicNumber);
    br.WORD(&header.frameCount);
    br.WORD(&header.width);
    br.WORD(&header.height);
    br.WORD(&header.colorDepth);
    br.SEEK(114);  // Jump to end of header data

    // Store frames
    frameArray.reserve(header.frameCount);
    for (int i = 0; i < header.frameCount; i++) {
        // TODO: make work for more than 1 frame
        Frame frame;
        // Collect frame header data
        br.DWORD(&frame.header.size);
        br.WORD(&frame.header.magicNumber);
        br.WORD(&frame.header.chunkCountOld);
        br.WORD(&frame.header.frameDuration);
        br.SEEK(2);
        br.DWORD(&frame.header.chunkCountNew);

        // Loop through chunks
        int chunkNum = 0;
        if (frame.header.chunkCountNew != 0) {
            chunkNum = frame.header.chunkCountNew;
        } else if (frame.header.chunkCountOld != 0xFFFF) {
            chunkNum = frame.header.chunkCountOld;
        }
        int lastChunk = 0;
        for (int c = 0; c < chunkNum; c++) {
            uint chunkStart = br.readCount();
            uint chunkSize;
            br.DWORD(&chunkSize);
            uint chunkEnd = chunkStart + chunkSize;

            uint16_t chunkType;
            br.WORD(&chunkType);

            // Chunks
            if (chunkType == Chunks::FrameTags) {
                uint16_t numTags;
                // chunk header
                br.WORD(&numTags);
                tagArray.reserve(numTags);
                br.SEEK(8);  // skip useless data

                for (int t = 0; t < numTags; t++) {
                    Tag tag;

                    // read into tag
                    br.WORD(&tag.from);
                    br.WORD(&tag.to);
                    uint8_t loopDir;
                    br.BYTE(&loopDir);
                    tag.loopDirection = static_cast<Tag::LoopDirections>(loopDir);
                    br.SEEK(8);

                    // Color (supposedly depreciated)
                    uint8_t chan[4];
                    for (int channel = 0; c < 3; c++) {
                        br.BYTE(&chan[channel]);
                    }
                    chan[3] = 255;  // alpha
                    tag.userData.color = (Color){.R = chan[0], .G = chan[1], .B = chan[2], .A = chan[3]};

                    br.SEEK(1);
                    br.STRING(&tag.name);

                    // add tag
                    tagArray.push_back(tag);
                    lastChunk = Chunks::FrameTags;
                }

                // check we are at the right spot
                if (br.readCount() != chunkEnd) {
                    printf("Tag chunk reading error, we are at: %u, but should be at: %u \n", br.readCount(), chunkEnd);
                    exit(1);
                }
            } else if (chunkType == Chunks::FrameLayer) {
                Layer layer;

                // read into layer
                uint16_t flags;
                br.WORD(&flags);
                layer.flags = static_cast<Layer::Flags>(flags);

                uint16_t type;
                br.WORD(&type);
                layer.type = static_cast<Layer::Types>(type);

                br.WORD(&layer.childLevel);
                br.SEEK(4);
                br.WORD(&layer.blendMode);

                uint8_t opacity;
                br.BYTE(&opacity);
                layer.opacity = (float)(opacity / 255.0f);

                br.SEEK(3);
                br.STRING(&layer.name);

                if (layer.type == 2) {
                    br.SEEK(4);
                    printf("%s", "WARNING: Tileset type aseprite layers not supported.");
                }

                // check we are at the right spot
                if (br.readCount() != chunkEnd) {
                    printf("Layer chunk reading error, we are at: %u, but should be at: %u \n", br.readCount(), chunkEnd);
                    exit(1);
                }

                // add layer
                layerArray.push_back(layer);
                lastChunk = Chunks::FrameLayer;
            } else if (chunkType == Chunks::FrameCel) {
                Cel cel;

                // read into cel
                uint16_t layerIndex;
                br.WORD(&layerIndex);
                cel.layer = layerArray[layerIndex];

                br.SHORT(&cel.x);
                br.SHORT(&cel.y);

                uint8_t opacity;
                br.BYTE(&opacity);
                cel.opacity = (float)(opacity / 255.0f);

                uint16_t type;
                br.WORD(&type);

                br.SEEK(7);

                if (type == 0 || type == 2) {
                    br.WORD(&cel.width);
                    br.WORD(&cel.height);
                    if (header.colorDepth == 32) {
                        uint bytes = cel.width * cel.height * 4;
                        cel.pixels.reserve(cel.width * cel.height);
                        if (type == 0) {
                            br.PIXEL(&cel.pixels, cel.width * cel.height);
                        } else if (type == 2) {
                            uint len = chunkEnd - br.readCount();
                            br.DECODE(&cel.pixels, len, bytes);
                        }
                    } else {
                        printf("WARNING: Grayscale and Index modes are not supported! Found in file %s\n", filePath);
                    }
                } else {
                    printf("WARNING: Linked cels and/or Compressed Tilemap cels are not supported!");
                    br.SEEK(chunkEnd - br.readCount());
                }
                // check we are at the right spot
                if (br.readCount() != chunkEnd) {
                    printf("WARNING: Cel chunk reading error for file %s, we are at: %u, but should be at: %u \n", filePath, br.readCount(),
                           chunkEnd);
                    exit(1);
                }

                // add cel
                frame.celArray.push_back(cel);
                lastChunk = Chunks::FrameCel;
            } else if (chunkType == Chunks::ChunkUserData) {
                UserData userData;
                uint32_t flags;
                br.DWORD(&flags);
                if (flags & 1) {
                    br.STRING(&userData.text);
                }
                if (flags * 2) {
                    uint8_t chan[4];
                    for (int channel = 0; channel < 4; channel++) {
                        br.BYTE(&chan[channel]);
                    }
                    userData.color = (Color){.R = chan[0], .G = chan[1], .B = chan[2], .A = chan[3]};
                }

                switch (lastChunk) {
                case Chunks::FrameCel:
                    // Not implemented
                    break;
                case Chunks::FrameLayer:
                    // Not implemented
                    break;
                case Chunks::FrameTags:
                    // Not implemented
                    printf("Tag user data found \n");  // never seem to see userdata chunks
                    break;
                default:
                    break;
                }

                // check we are at the right spot
                if (br.readCount() != chunkEnd) {
                    printf("WARNING: Cel chunk reading error, we are at: %u, but should be at: %u \n", br.readCount(), chunkEnd);
                    exit(1);
                }

            } else {
                br.SEEK(chunkEnd - br.readCount());
            }
        }

        // Add frame to array
        frameArray.push_back(frame);
    }

    // Release
    fclose(file);
    free(buffer);
}

void BufferReader::BYTE(uint8_t* __dest) {
    std::memcpy(__dest, mBuffer + mReadCounter, 1);
    mReadCounter += 1;
}

void BufferReader::SHORT(int16_t* __dest) {
    std::memcpy(__dest, mBuffer + mReadCounter, 2);
    mReadCounter += 2;
}
void BufferReader::DWORD(uint32_t* __dest) {
    std::memcpy(__dest, mBuffer + mReadCounter, 4);
    mReadCounter += 4;
}

void BufferReader::WORD(uint16_t* __dest) {
    std::memcpy(__dest, mBuffer + mReadCounter, 2);
    mReadCounter += 2;
}
void BufferReader::STRING(std::vector<char>* __str) {
    uint16_t len;
    WORD(&len);
    for (int c = 0; c < len; c++) {
        uint8_t int_char;
        BYTE(&int_char);
        __str->push_back((char)int_char);
    }
    __str->push_back('\0');
}
void BufferReader::PIXEL(std::vector<Color>* __dest, int __len) {
    for (int p = 0; p < __len; p++) {
        Color color;
        uint8_t chan[4];
        for (int c = 0; c < 4; c++) {
            BYTE(&chan[c]);
        }
        color = (Color){.R = chan[0], .G = chan[1], .B = chan[2], .A = chan[3]};
        __dest->push_back(color);
    }
}
void BufferReader::_U_CHAR_ARR(unsigned char __start[], int __len) {
    for (int c = 0; c < __len; c++) {
        std::memcpy(&(__start[c]), mBuffer + mReadCounter, 1);  // Might be wrong
        mReadCounter += 1;
    }
}

void BufferReader::DECODE(std::vector<Color>* __dest, int __in_len, int __out_len) {
    // __out_len is number of pixels * 4 bytes per pixel and __in_len is length of compressed data.
    // Copied from https://gist.github.com/arq5x/5315739 and http://zlib.net/zlib_how.html
    unsigned char* in = new unsigned char[__in_len];
    unsigned char* out = new unsigned char[__out_len];

    _U_CHAR_ARR(in, __in_len);

    // zlib struct
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;

    infstream.avail_in = __in_len;
    infstream.next_in = (Bytef*)in;
    infstream.avail_out = __out_len;
    infstream.next_out = (Bytef*)out;

    // actual decompression
    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    for (int p = 0; p < __out_len / 4; p++) {
        Color color;
        uint8_t chan[4];
        for (int c = 0; c < 4; c++) {
            chan[c] = (uint8_t)out[4 * p + c];
        }
        color = (Color){.R = chan[0], .G = chan[1], .B = chan[2], .A = chan[3]};
        __dest->push_back(color);
    }

    delete[] in;
    delete[] out;
}

std::vector<Color> Frame::getRaster(int rasterWidth, int rasterHeight) const {
    if (celArray.size() == 0) {
        return {};
    }

    std::vector<Color> base(rasterWidth * rasterHeight, Color{0, 0, 0, 0});
    for (const auto& cel : celArray) {
        if (!cel.layer.isOn(Layer::Flags::Visible)) {
            continue;
        }

        if (cel.opacity == 0.0f) {
            continue;
        }

        // Cel is not guaranteed to be <= the raster size for some reason
        // const int celWidthClamped = cel.width > rasterWidth ? rasterWidth : cel.width;
        // const int celHeightClamped = cel.height > rasterHeight ? rasterHeight : cel.height;

        for (size_t i = 0; i < cel.pixels.size(); i++) {
            // if cel size is too big, I need to skip i values which are outside of the canvas
            const int celColRaw = i % cel.width;  // 0 indexed
            const int celRowRaw = i / cel.width;  // 0 indexed
            if ((celColRaw + cel.x) >= rasterWidth || (celRowRaw + cel.y) >= rasterHeight) {
                continue;  // cel pixel is outside of the canvas
            }
            const Color& color = cel.pixels[i];
            // I don't feel like implementing alpha blending right now (I don't need it). I'll just have non-transparent pixels override what's below
            // them
            if (color.A > 0) {
                const int celRow = i / cel.width;
                const int celCol = i % cel.width;
                const int ix = (celRow + cel.y) * rasterWidth + celCol + cel.x;
                // debugging:
                // if (ix < 0 || ix >= rasterWidth * rasterHeight) {
                //     printf("ix of %d is out of bounds for raster image of size %dx%d\n", ix, rasterWidth, rasterHeight);
                //     printf("Cel width is %d, Cel Height is %d. Cel X is %d, Cel Y is %d\n", cel.width, cel.height, cel.x, cel.y);
                //     printf("Calculated Cel row was %d and column was %d\n", celRow, celCol);
                //     printf("i is %zu\n", i);
                // }
                // fflush(stdout);
                assert(ix >= 0 && ix < rasterWidth * rasterHeight && "index out of bounds");
                base[ix] = color;
            }
        }
    }
    return base;
}

}  // namespace ase

// int main(int argc, char** argv) {
//     const char* asepritePath = "test.aseprite";
//     ase::Aseprite file = ase::Aseprite(asepritePath);
//     std::cout << "frame count: " << file.header.frameCount << std::endl;
// }
