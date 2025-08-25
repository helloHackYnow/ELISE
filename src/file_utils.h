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

#endif //FILE_UTILS_H
