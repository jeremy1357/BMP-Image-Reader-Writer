#ifndef EZBMP_H
#define EZBMP_H
#pragma once
#include <fstream>
#include <stdint.h>
#include <string>
#include <vector>

namespace bmp {
static const int BMP_HEADER_SIZE_BYTES = 14;
static const int BMP_DIB_HEADER_SIZE_BYTES = 40;
static const int BMP_V4DIB_HEADER_SIZE_BYTES = 108;
uint8_t bmpPadding[4] = { 0, 0, 0, 0 };

enum BMPErrorCode {
    FAILED_TO_OPEN = 1,
    FILE_NOT_BMP,
    BMP_DATA_CORRUPT,
    T1_GREATER,
    NO_PIXEL_DATA
};

#pragma pack(push, 1)
struct BMPHeader {
    uint8_t signature[2]      = { 0, 0 }; // Must be B M
    uint32_t fileSize         = 0;
    uint16_t reserved1        = 0;
    uint16_t reserved2        = 0;
    uint32_t pixelArrayOffset = 0;
};
#pragma pack(pop)

namespace InfoHeaderFormats {
    struct InfoHeader {
        uint32_t dibHeaderSize        = 0;
        uint32_t widthPixels          = 0;
        uint32_t heightPixels         = 0;
        uint16_t numColorPlanes       = 0; // Always 1
        uint16_t bitsPerPixel         = 0;
        uint32_t compressionMethod    = 0;
        uint32_t imageSize            = 0;
        uint32_t horizontalResolution = 0;
        uint32_t verticalResolution   = 0;
        uint32_t numColorsInTable     = 0;
        uint32_t importantColors      = 0;
    };
    struct V4InfoHeader {
        uint32_t dibHeaderSize          = 0;
        uint32_t widthPixels            = 0;
        uint32_t heightPixels           = 0;
        uint16_t numColorPlanes         = 0;
        uint16_t bitsPerPixel           = 0;
        uint32_t compressionMethod      = 0;
        uint32_t imageSize              = 0;
        uint32_t horizontalResolution   = 0;
        uint32_t verticalResolution     = 0;
        uint32_t numColorsInTable       = 0;
        uint32_t importantColors        = 0;
        uint32_t redChannelBitMask      = 0;
        uint32_t greenChannelBitMask    = 0;
        uint32_t blueChannelBitMask     = 0;
        uint32_t alphaChannelBitMask    = 0;
        uint32_t windowsColorSpace      = 0;
        uint32_t colorSpaceEndpoints[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        uint32_t redGamma               = 0;
        uint32_t greenGamma             = 0;
        uint32_t blueGamma              = 0;
    };
}

template <typename T1>
class BMPFileUserDefined {
public:
    BMPHeader header;
    T1 infoHeader;
    std::vector<uint8_t> colorTable;
    std::vector<uint8_t> pixelData;

    inline int read_bmp(const std::string& filePath)
    {
        std::ifstream file(filePath, std::ios::in | std::ios::binary);
        if (file.fail()) {
            return BMPErrorCode::FAILED_TO_OPEN;
        }
        file.seekg(0, std::ios::end);
        uint64_t fileSize = file.tellg();
        if (fileSize == 0) {
            return BMPErrorCode::FAILED_TO_OPEN;
        }
        file.seekg(std::ios::beg);
        std::vector<uint8_t> imageData(fileSize);
        file.read((char*)(&imageData.at(0)), fileSize);
        file.close();
        if (imageData.size() < BMP_HEADER_SIZE_BYTES + sizeof(T1)) {
            return BMP_DATA_CORRUPT;
        }
        std::memcpy(&this->header, &imageData.at(0), BMP_HEADER_SIZE_BYTES);
        if (header.signature[0] != 'B' || header.signature[1] != 'M') {
            return BMPErrorCode::FILE_NOT_BMP;
        }

        uint32_t actualInfoHeaderSize = imageData[17] << 24 | imageData[16] << 16 | imageData[15] << 8 | imageData[14];
        // Copy in info header depending on size specificed by T1.
        // Can be deduced to fit certain formats. Ex) Standard to v4
        // If user is trying to read an info header thats larger then actual then set to actual value
        int requestedT1Size = sizeof(T1);
        try {
            if (requestedT1Size > actualInfoHeaderSize) {
                throw requestedT1Size;
            }
        } catch (int x) {
            requestedT1Size = actualInfoHeaderSize;
        }

        std::memcpy(&this->infoHeader, &imageData.at(BMP_HEADER_SIZE_BYTES), requestedT1Size);

        int bytesPerPixel = infoHeader.bitsPerPixel / 8;
        int totalPixels = infoHeader.heightPixels * infoHeader.widthPixels;
        int componentsPerRow = infoHeader.widthPixels * bytesPerPixel;
        int totalComponentsInImage = totalPixels * bytesPerPixel;
        int colorTableSize = 0;

        // If true a color table is used.
        // A color table is used when the bits per pixel is less than or equal to 8
        if (infoHeader.bitsPerPixel <= 8) {
            int colorTableOffset = infoHeader.dibHeaderSize + BMP_HEADER_SIZE_BYTES;
            colorTableSize = header.pixelArrayOffset - colorTableOffset;
            this->colorTable.resize(colorTableSize);
            std::copy(imageData.begin() + colorTableOffset,
                imageData.begin() + colorTableSize + colorTableOffset,
                &this->colorTable.at(0));
        }

        // A row of pixel data must be contained within a multiple of 4 bytes.
        int bytePaddingRemainder = componentsPerRow % 4;
        int paddingToAdd = 0;
        if (bytePaddingRemainder != 0) {
            paddingToAdd = 4 - bytePaddingRemainder;
        }
        this->pixelData.resize(totalComponentsInImage);
        // Use actual info header size from the info header and not the size of T1.
        imageData.erase(imageData.begin(), imageData.begin() + BMP_HEADER_SIZE_BYTES + infoHeader.dibHeaderSize + colorTableSize);

        // Load in pixel data and ignore padding.
        for (size_t i = 0; i < infoHeader.heightPixels; i++) {
            int startIndex = (i * componentsPerRow) + (i * paddingToAdd);
            int endIndex = ((i + 1) * componentsPerRow) + (i * paddingToAdd) - 1;
            auto startIter = imageData.begin() + startIndex;
            auto endIter = imageData.begin() + endIndex;
            std::copy(startIter, endIter, &this->pixelData[i * componentsPerRow]);
        }
    }

    inline int create_bmp(const std::string& fileName, bool addPadding)
    {
        std::ofstream file;
        file.open(fileName, std::ios::out | std::ios::binary);
        if (file.fail()) {
            return BMPErrorCode::FAILED_TO_OPEN;
        }
        file.write(reinterpret_cast<const char*>(&header), BMP_HEADER_SIZE_BYTES);
        if (sizeof(T1) > infoHeader.dibHeaderSize) {
            return T1_GREATER;
        }
        file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(T1));
        if (colorTable.empty() == false) {
            file.write(reinterpret_cast<const char*>(colorTable.data()), colorTable.size());
        }
        if (pixelData.empty() == false) {
            if (addPadding) {
                int componentsInRow = infoHeader.widthPixels * (infoHeader.bitsPerPixel / 8);
                int bytePaddingRemainder = componentsInRow % 4;
                int paddingToAdd = 0;
                if (bytePaddingRemainder != 0) {
                    paddingToAdd = 4 - bytePaddingRemainder;
                }
                for (size_t i = 0; i < infoHeader.heightPixels; i++) {
                    file.write(reinterpret_cast<const char*>(&pixelData.at(i * componentsInRow)), componentsInRow);
                    file.write(reinterpret_cast<const char*>(&bmpPadding[0]), paddingToAdd);
                }
            } else {
                file.write(reinterpret_cast<const char*>(pixelData.data()), pixelData.size());
            }
        } else {
            file.close();
            return BMPErrorCode::NO_PIXEL_DATA;
        }
        file.close();
        return 0;
    }
};

int read_bmp(const std::string& filePath, int& width, int& height,
    std::vector<uint8_t>& pixelData)
{
    std::ifstream image(filePath, std::ios::in | std::ios::binary);
    if (image.fail()) {
        return BMPErrorCode::FAILED_TO_OPEN;
    }
    uint64_t fileSize = 0;
    image.seekg(0, std::ios::end);
    fileSize = image.tellg();
    image.seekg(std::ios::beg);
    std::vector<uint8_t> imageData(fileSize);

    image.read((char*)(&imageData.at(0)), fileSize);
    image.close();
    if (imageData[0] != 'B' && imageData[1] != 'M') {
        return BMPErrorCode::FILE_NOT_BMP;
    }

    int offsetToPixelArrayLocation = 10;
    int offsetToWidth = offsetof(InfoHeaderFormats::InfoHeader, widthPixels) + BMP_HEADER_SIZE_BYTES;
    int offsetToHeight = offsetof(InfoHeaderFormats::InfoHeader, heightPixels) + BMP_HEADER_SIZE_BYTES;

    int pixelArrayOffset = imageData[offsetToPixelArrayLocation + 4] << 24
        | imageData[offsetToPixelArrayLocation + 3] << 16
        | imageData[offsetToPixelArrayLocation + 2] << 8
        | imageData[offsetToPixelArrayLocation];
    width = (imageData[offsetToWidth + 4] << 24
        | imageData[offsetToWidth + 3] << 16
        | imageData[offsetToWidth + 2] << 8
        | imageData[offsetToWidth]);
    height = (imageData[offsetToHeight + 4] << 24
        | imageData[offsetToHeight + 3] << 16
        | imageData[offsetToHeight + 2] << 8
        | imageData[offsetToHeight]);

    pixelData.resize(fileSize - pixelArrayOffset);
    std::copy(&imageData.at(pixelArrayOffset), &imageData.at(fileSize - 1),
        &pixelData.at(0));

    return 0;
}

std::string get_error(const int errorCode)
{
    switch (errorCode) {
    case BMPErrorCode::FAILED_TO_OPEN:
        return "File failed to either open or be created";
    case BMPErrorCode::BMP_DATA_CORRUPT:
        return "Data stored in bmp is not valid";
    case BMPErrorCode::FILE_NOT_BMP:
        return "File requested is not a bmp";
    case BMPErrorCode::T1_GREATER:
        return "Attempting to read infoheader size thats greater then the actual size";
    case BMPErrorCode::NO_PIXEL_DATA:
        return "No pixel data passed to create bmp with";
    default:
        return "Unknown";
    }
}
}
#endif