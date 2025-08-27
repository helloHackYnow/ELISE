//
// Created by victor on 30/05/25.
//

#ifndef FILE_UTILS_H
#define FILE_UTILS_H
#include <fstream>
#include <string>

inline std::string ensure_extension(const std::string& path, const std::string& extension) {

    int ext_len = extension.length();

    if (path.length() < ext_len) {
        return path + extension;
    }

    if (path.substr(path.length() - ext_len) == extension) {
        return path;
    }

    return path + extension;
}

inline std::vector<unsigned char> loadFileToVector(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    // Get the size of the file
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Create a vector with the appropriate size
    std::vector<unsigned char> buffer(size);

    // Read file data into the vector
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Error reading file: " + filename);
    }

    return buffer;
}

inline void saveVectorToFile(const std::string& filename, const std::vector<unsigned char>& data) {
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("Unable to open file for writing: " + filename);
    }

    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!file) {
            throw std::runtime_error("Error writing to file: " + filename);
        }
    } else {
        // ensure the file is truncated when data is empty (std::ios::trunc already does that)
        file.flush();
        if (!file) throw std::runtime_error("Error flushing empty file: " + filename);
    }
}

#endif //FILE_UTILS_H
