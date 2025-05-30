//
// Created by victor on 30/05/25.
//

#ifndef FILE_UTILS_H
#define FILE_UTILS_H
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

#endif //FILE_UTILS_H
