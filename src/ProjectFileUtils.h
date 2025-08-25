//
// Created by victor on 24/08/2025.
//

#ifndef ELISE_PROJECTUTILS_H
#define ELISE_PROJECTUTILS_H
#include <string>
#include <vector>
#include <stdexcept>
#include "mz.h"
#include "mz_strm.h"
#include "mz_zip.h"
#include "mz_strm_os.h"
#include "mz_strm_buf.h"
#include "mz_strm_mem.h"


namespace EliseProject {


struct EliseProjectFileData {
    std::vector<unsigned char> json_data;
    std::vector<unsigned char> mp3_data;
};

void* createZipHandle(const std::string &zipPath);
std::vector<unsigned char> readEntryToVector(void* zip_handle, const std::string& entryPath);
void closeZipHandle(void* zip_handle);

struct ZipWriter {
    void* zip_handle = nullptr;
    void* mem_stream = nullptr;
};
ZipWriter initZip();
void addFileToZip(ZipWriter& zw, const std::string& filename, const std::vector<unsigned char>& data);
void writeZipToDisk(ZipWriter& zw, const std::string& path);

EliseProjectFileData load(const std::string& path);
void save(const std::string& path, const EliseProjectFileData& data);

}
#endif //ELISE_PROJECTUTILS_H
