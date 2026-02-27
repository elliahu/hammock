#pragma once
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <regex>

#include "core/utilities.hpp"

namespace hammock::filesystem {
    /// Check if file exists
    inline bool fileExists(const std::string &filename) {
        std::ifstream f(filename.c_str());
        return !f.fail();
    }

    /// Reads file contents into char list
    inline std::vector<char> readFile(const std::string &filePath) {
        std::ifstream file{filePath, std::ios::ate | std::ios::binary};

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + filePath);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    /// Dump date into file
    inline void dump(const std::string &filename, const std::string &data) {
        std::ofstream outFile(filename);
        if (outFile.is_open()) {
            outFile << data;
            outFile.close();
        } else {
            throw std::runtime_error("could not dump into file!");
        }
    }

    /// Lists directory contents
    inline std::vector<std::string> ls(const std::string &directoryPath) {
        std::vector<std::string> fileList;

        try {
            for (const auto &entry: std::filesystem::directory_iterator(directoryPath)) {
                try {
                    if (entry.is_regular_file()) {
                        fileList.push_back(entry.path().string());
                    }
                } catch (const std::filesystem::filesystem_error &e) {
                    hammock::core::Logger::log(core::LOG_LEVEL_ERROR, "Error on entry: %s | %s\n",
                                               entry.path().string().c_str(), e.what());
                }
            }
        } catch (const std::filesystem::filesystem_error &e) {
            core::Logger::log(core::LOG_LEVEL_ERROR, "Error: Error accessing directory %s\n",
                              directoryPath.c_str());
            throw std::runtime_error("Error: Error accessing directory");
        }

        // Custom comparator for natural sorting
        auto naturalSort = [](const std::string &a, const std::string &b) {
            std::regex numRegex(R"(.*?(\d+))"); // Extracts the first number from the filename
            std::smatch matchA, matchB;

            int numA = std::regex_search(a, matchA, numRegex) ? std::stoi(matchA[1].str()) : 0;
            int numB = std::regex_search(b, matchB, numRegex) ? std::stoi(matchB[1].str()) : 0;

            return numA < numB;
        };

        std::sort(fileList.begin(), fileList.end(), naturalSort);
        return fileList;
    }

    /// Write contents into png image
    inline void writeImagePng(const std::string &filename, const int width, const int height, const int channels,
                              const void *data, const int strideInBytes) {
        if (auto ok = stbi_write_png(filename.c_str(), width, height, channels, data, strideInBytes); !ok) {
            throw std::runtime_error("Failed to write PNG image " + filename);
        }
    }

    /// Write contents into jpg image
    inline void writeImageJpg(const std::string &filename, const int width, const int height, const int channels,
                              const void *data, int quality) {
        if (auto ok = stbi_write_jpg(filename.c_str(), width, height, channels, data, quality); !ok) {
            throw std::runtime_error("Failed to write JPG image " + filename);
        }
    }

    /// Write contents into HDR image
    inline void writeImageHdr(const std::string &filename, const int width, const int height, const int channels,
                              const float *data) {
        if (auto ok = stbi_write_hdr(filename.c_str(), width, height, channels, data); !ok) {
            throw std::runtime_error("Failed to write HDR image " + filename);
        }
    }

    /// Write contents into bmp image
    inline void writeImageBmp(const std::string &filename, const int width, const int height, const int channels,
                              const void *data) {
        if (auto ok = stbi_write_bmp(filename.c_str(), width, height, channels, data); !ok) {
            throw std::runtime_error("Failed to write BMP image " + filename);
        }
    }

    /// Write contents into bmp image
    inline void writeImageTga(const std::string &filename, const int width, const int height, const int channels,
                              const void *data) {
        if (auto ok = stbi_write_tga(filename.c_str(), width, height, channels, data); !ok) {
            throw std::runtime_error("Failed to write TGA image " + filename);
        }
    }

    inline std::uint16_t float32float16(float f) {
        std::uint32_t f32 = *(std::uint32_t *) &f;
        std::uint16_t f16 = 0;

        std::uint32_t sign = (f32 >> 16) & 0x8000; // Extract sign bit
        std::uint32_t exponent = ((f32 >> 23) & 0xFF) - 112; // Adjust exponent bias
        std::uint32_t mantissa = (f32 & 0x007FFFFF) >> 13; // Truncate mantissa

        if (exponent <= 0) {
            // Underflow case (denormals or zero)
            f16 = sign;
        } else if (exponent >= 31) {
            // Overflow case (inf or NaN)
            f16 = sign | 0x7C00 | (mantissa ? 1 : 0);
        } else {
            // Normal conversion
            f16 = sign | (exponent << 10) | mantissa;
        }

        return f16;
    }

    /// Channel flags to select which channel will be used
    enum ChannelFlags : std::uint32_t {
        CHANNEL_R = 1 << 0, // 0x01
        CHANNEL_G = 1 << 1, // 0x02
        CHANNEL_B = 1 << 2, // 0x04
        CHANNEL_A = 1 << 3 // 0x08
    };

    inline const void *readImageHdr32Bit(const std::string &filename, std::uint32_t channelMask, std::uint32_t &width,
                                         std::uint32_t &height, bool flipY = false) {
        if (flipY) stbi_set_flip_vertically_on_load(true);
        int c, w, h;
        const float *data = stbi_loadf(filename.c_str(), &w, &h, &c, 4);
        width = w;
        height = h;
        stbi_set_flip_vertically_on_load(false);

        if (!data) {
            throw std::runtime_error("Failed to load HDR 32Bit image.");
        }

        // Count how many channels are selected and create mapping
        std::uint32_t readChannels = 0;
        int channelMapping[4];
        for (int i = 0; i < 4; ++i) {
            if (channelMask & (1 << i)) {
                channelMapping[readChannels++] = i;
            }
        }

        if (readChannels == 0) {
            stbi_image_free(const_cast<float *>(data));
            throw std::runtime_error("No channels selected.");
        }

        size_t pixelCount = static_cast<size_t>(width) * height;
        // Allocate buffer for 32-bit floats
        float *processedData = new float[pixelCount * readChannels];

        for (size_t i = 0; i < pixelCount; ++i) {
            for (std::uint32_t c = 0; c < readChannels; ++c) {
                processedData[i * readChannels + c] = data[i * 4 + channelMapping[c]];
            }
        }

        stbi_image_free(const_cast<float *>(data));
        return processedData; // 32-bit float buffer
    }

    inline const void *readImageHdr16Bit(const std::string &filename, std::uint32_t channelMask, std::uint32_t &width,
                                         std::uint32_t &height, bool flipY = false) {
        if (flipY) stbi_set_flip_vertically_on_load(true);
        int c, w, h;
        const float *data = stbi_loadf(filename.c_str(), &w, &h, &c, 4);
        width = w;
        height = h;
        stbi_set_flip_vertically_on_load(false);

        if (!data) {
            throw std::runtime_error("Failed to load HDR 16Bit image.");
        }

        // Count how many channels are selected and create mapping
        std::uint32_t readChannels = 0;
        int channelMapping[4];
        for (int i = 0; i < 4; ++i) {
            if (channelMask & (1 << i)) {
                channelMapping[readChannels++] = i;
            }
        }

        if (readChannels == 0) {
            stbi_image_free(const_cast<float *>(data));
            throw std::runtime_error("No channels selected.");
        }

        size_t pixelCount = static_cast<size_t>(width) * height;
        // Allocate buffer for 16-bit floats
        std::uint16_t *processedData = new std::uint16_t[pixelCount * readChannels];

        for (size_t i = 0; i < pixelCount; ++i) {
            for (std::uint32_t c = 0; c < readChannels; ++c) {
                processedData[i * readChannels + c] = float32float16(data[i * 4 + channelMapping[c]]);
            }
        }

        stbi_image_free(const_cast<float *>(data));
        return processedData; // 16-bit float buffer
    }

    inline const void *readImageSdr8Bit(const std::string &filename, std::uint32_t channelMask, std::uint32_t &width,
                                        std::uint32_t &height, bool flipY = false) {
        if (flipY) stbi_set_flip_vertically_on_load(true);
        int c, w, h;
        const unsigned char *data = stbi_load(filename.c_str(), &w, &h, &c, 4);
        width = w;
        height = h;
        stbi_set_flip_vertically_on_load(false);

        if (!data) {
            throw std::runtime_error("Failed to load SDR 8Bit image.");
        }

        // Count how many channels are selected and create mapping
        std::uint32_t readChannels = 0;
        int channelMapping[4];
        for (int i = 0; i < 4; ++i) {
            if (channelMask & (1 << i)) {
                channelMapping[readChannels++] = i;
            }
        }

        if (readChannels == 0) {
            stbi_image_free(const_cast<unsigned char *>(data));
            throw std::runtime_error("No channels selected.");
        }

        size_t pixelCount = static_cast<size_t>(width) * height;
        unsigned char *processedData = new unsigned char[pixelCount * readChannels];

        for (size_t i = 0; i < pixelCount; ++i) {
            for (std::uint32_t c = 0; c < readChannels; ++c) {
                processedData[i * readChannels + c] = data[i * 4 + channelMapping[c]];
            }
        }

        stbi_image_free(const_cast<unsigned char *>(data));
        return processedData;
    }

    inline const void *readVolumeHdr32Bit(const std::vector<std::string> &filenames, std::uint32_t channelMask,
                                          std::uint32_t &width, std::uint32_t &height, std::uint32_t &depth, bool flipY = false) {
        if (filenames.empty()) {
            throw std::runtime_error("No images provided for volume texture.");
        }

        depth = static_cast<std::uint32_t>(filenames.size());

        // Count how many channels are selected and create mapping
        std::uint32_t readChannels = 0;
        int channelMapping[4];
        for (int i = 0; i < 4; ++i) {
            if (channelMask & (1 << i)) {
                channelMapping[readChannels++] = i;
            }
        }

        if (readChannels == 0) {
            throw std::runtime_error("No channels selected.");
        }

        // Load first image to get dimensions
        if (flipY) stbi_set_flip_vertically_on_load(true);
        int c, w, h;
        const float *firstData = stbi_loadf(filenames[0].c_str(), &w, &h, &c, 4);
        width = w;
        height = h;
        stbi_set_flip_vertically_on_load(false);

        if (!firstData) {
            throw std::runtime_error("Failed to load first HDR 32Bit image: " + filenames[0]);
        }

        size_t pixelCountPerSlice = static_cast<size_t>(width) * height;
        size_t totalPixelCount = pixelCountPerSlice * depth;

        // Allocate buffer for entire volume
        float *volumeData = new float[totalPixelCount * readChannels];

        // Process first image
        for (size_t i = 0; i < pixelCountPerSlice; ++i) {
            for (std::uint32_t c = 0; c < readChannels; ++c) {
                volumeData[i * readChannels + c] = firstData[i * 4 + channelMapping[c]];
            }
        }
        stbi_image_free(const_cast<float *>(firstData));

        // Load and process remaining images
        for (std::uint32_t sliceIdx = 1; sliceIdx < depth; ++sliceIdx) {
            if (flipY) stbi_set_flip_vertically_on_load(true);
            const float *sliceData = stbi_loadf(filenames[sliceIdx].c_str(), &w, &h, &c, 4);
            stbi_set_flip_vertically_on_load(false);

            if (!sliceData) {
                delete[] volumeData;
                throw std::runtime_error("Failed to load HDR 32Bit image: " + filenames[sliceIdx]);
            }

            // Verify dimensions match
            if (w != static_cast<int>(width) || h != static_cast<int>(height)) {
                stbi_image_free(const_cast<float *>(sliceData));
                delete[] volumeData;
                throw std::runtime_error("Image dimensions mismatch at slice " + std::to_string(sliceIdx));
            }

            // Copy slice data to volume buffer
            size_t sliceOffset = sliceIdx * pixelCountPerSlice * readChannels;
            for (size_t i = 0; i < pixelCountPerSlice; ++i) {
                for (std::uint32_t c = 0; c < readChannels; ++c) {
                    volumeData[sliceOffset + i * readChannels + c] = sliceData[i * 4 + channelMapping[c]];
                }
            }

            stbi_image_free(const_cast<float *>(sliceData));
        }

        return volumeData;
    }

    inline const void *readVolumeHdr16Bit(const std::vector<std::string> &filenames, std::uint32_t channelMask,
                                          std::uint32_t &width, std::uint32_t &height, std::uint32_t &depth, bool flipY = false) {
        if (filenames.empty()) {
            throw std::runtime_error("No images provided for volume texture.");
        }

        depth = static_cast<std::uint32_t>(filenames.size());

        // Count how many channels are selected and create mapping
        std::uint32_t readChannels = 0;
        int channelMapping[4];
        for (int i = 0; i < 4; ++i) {
            if (channelMask & (1 << i)) {
                channelMapping[readChannels++] = i;
            }
        }

        if (readChannels == 0) {
            throw std::runtime_error("No channels selected.");
        }

        // Load first image to get dimensions
        if (flipY) stbi_set_flip_vertically_on_load(true);
        int c, w, h;
        const float *firstData = stbi_loadf(filenames[0].c_str(), &w, &h, &c, 4);
        width = w;
        height = h;
        stbi_set_flip_vertically_on_load(false);

        if (!firstData) {
            throw std::runtime_error("Failed to load first HDR 16Bit image: " + filenames[0]);
        }

        size_t pixelCountPerSlice = static_cast<size_t>(width) * height;
        size_t totalPixelCount = pixelCountPerSlice * depth;

        // Allocate buffer for entire volume
        std::uint16_t *volumeData = new std::uint16_t[totalPixelCount * readChannels];

        // Process first image
        for (size_t i = 0; i < pixelCountPerSlice; ++i) {
            for (std::uint32_t c = 0; c < readChannels; ++c) {
                volumeData[i * readChannels + c] = float32float16(firstData[i * 4 + channelMapping[c]]);
            }
        }
        stbi_image_free(const_cast<float *>(firstData));

        // Load and process remaining images
        for (std::uint32_t sliceIdx = 1; sliceIdx < depth; ++sliceIdx) {
            if (flipY) stbi_set_flip_vertically_on_load(true);
            const float *sliceData = stbi_loadf(filenames[sliceIdx].c_str(), &w, &h, &c, 4);
            stbi_set_flip_vertically_on_load(false);

            if (!sliceData) {
                delete[] volumeData;
                throw std::runtime_error("Failed to load HDR 16Bit image: " + filenames[sliceIdx]);
            }

            // Verify dimensions match
            if (w != static_cast<int>(width) || h != static_cast<int>(height)) {
                stbi_image_free(const_cast<float *>(sliceData));
                delete[] volumeData;
                throw std::runtime_error("Image dimensions mismatch at slice " + std::to_string(sliceIdx));
            }

            // Copy slice data to volume buffer
            size_t sliceOffset = sliceIdx * pixelCountPerSlice * readChannels;
            for (size_t i = 0; i < pixelCountPerSlice; ++i) {
                for (std::uint32_t c = 0; c < readChannels; ++c) {
                    volumeData[sliceOffset + i * readChannels + c] = float32float16(
                        sliceData[i * 4 + channelMapping[c]]);
                }
            }

            stbi_image_free(const_cast<float *>(sliceData));
        }

        return volumeData;
    }

    inline const void *readVolumeSdr8Bit(const std::vector<std::string> &filenames, std::uint32_t channelMask,
                                         std::uint32_t &width, std::uint32_t &height, std::uint32_t &depth, bool flipY = false) {
        if (filenames.empty()) {
            throw std::runtime_error("No images provided for volume texture.");
        }

        depth = static_cast<std::uint32_t>(filenames.size());

        // Count how many channels are selected and create mapping
        std::uint32_t readChannels = 0;
        int channelMapping[4];
        for (int i = 0; i < 4; ++i) {
            if (channelMask & (1 << i)) {
                channelMapping[readChannels++] = i;
            }
        }

        if (readChannels == 0) {
            throw std::runtime_error("No channels selected.");
        }

        // Load first image to get dimensions
        if (flipY) stbi_set_flip_vertically_on_load(true);
        int c, w, h;
        const unsigned char *firstData = stbi_load(filenames[0].c_str(), &w, &h, &c, 4);
        width = w;
        height = h;
        stbi_set_flip_vertically_on_load(false);

        if (!firstData) {
            throw std::runtime_error("Failed to load first SDR 8Bit image: " + filenames[0]);
        }

        size_t pixelCountPerSlice = static_cast<size_t>(width) * height;
        size_t totalPixelCount = pixelCountPerSlice * depth;

        // Allocate buffer for entire volume
        unsigned char *volumeData = new unsigned char[totalPixelCount * readChannels];

        // Process first image
        for (size_t i = 0; i < pixelCountPerSlice; ++i) {
            for (std::uint32_t c = 0; c < readChannels; ++c) {
                volumeData[i * readChannels + c] = firstData[i * 4 + channelMapping[c]];
            }
        }
        stbi_image_free(const_cast<unsigned char *>(firstData));

        // Load and process remaining images
        for (std::uint32_t sliceIdx = 1; sliceIdx < depth; ++sliceIdx) {
            if (flipY) stbi_set_flip_vertically_on_load(true);
            const unsigned char *sliceData = stbi_load(filenames[sliceIdx].c_str(), &w, &h, &c, 4);
            stbi_set_flip_vertically_on_load(false);

            if (!sliceData) {
                delete[] volumeData;
                throw std::runtime_error("Failed to load SDR 8Bit image: " + filenames[sliceIdx]);
            }

            // Verify dimensions match
            if (w != static_cast<int>(width) || h != static_cast<int>(height)) {
                stbi_image_free(const_cast<unsigned char *>(sliceData));
                delete[] volumeData;
                throw std::runtime_error("Image dimensions mismatch at slice " + std::to_string(sliceIdx));
            }

            // Copy slice data to volume buffer
            size_t sliceOffset = sliceIdx * pixelCountPerSlice * readChannels;
            for (size_t i = 0; i < pixelCountPerSlice; ++i) {
                for (std::uint32_t c = 0; c < readChannels; ++c) {
                    volumeData[sliceOffset + i * readChannels + c] = sliceData[i * 4 + channelMapping[c]];
                }
            }

            stbi_image_free(const_cast<unsigned char *>(sliceData));
        }

        return volumeData;
    }
}
