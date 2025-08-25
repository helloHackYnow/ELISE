//
// Created by victor on 24/08/2025.
//

#include "ProjectFileUtils.h"

#include <iostream>

namespace EliseProject {

void* createZipHandle(const std::string &zipPath) {
    void* os_stream = nullptr;
    void* buf_stream = nullptr;
    void* zip_handle = nullptr;

    // 1. Create OS file stream
    os_stream = mz_stream_os_create();
    if (!os_stream)
        throw std::runtime_error("Failed to create OS stream");

    // Open the actual file on the OS stream
    if (mz_stream_open(os_stream, zipPath.c_str(), MZ_OPEN_MODE_READ) != MZ_OK) {
        mz_stream_os_delete(&os_stream);
        throw std::runtime_error("Failed to open zip file: " + zipPath);
    }

    // 2. Create buffered stream
    buf_stream = mz_stream_buffered_create();
    if (!buf_stream) {
        mz_stream_close(os_stream);
        mz_stream_os_delete(&os_stream);
        throw std::runtime_error("Failed to create buffered stream");
    }

    // Attach the OS stream as the base
    mz_stream_set_base(buf_stream, os_stream);

    // Do NOT open the buffered stream — it wraps the already-open base

    // 3. Create ZIP handle
    zip_handle = mz_zip_create();
    if (!zip_handle) {
        mz_stream_buffered_delete(&buf_stream); // deletes the base as well
        throw std::runtime_error("Failed to create ZIP handle");
    }

    // 4. Open ZIP archive (takes ownership of the buffered stream)
    if (mz_zip_open(zip_handle, buf_stream, MZ_OPEN_MODE_READ) != MZ_OK) {
        mz_zip_delete(&zip_handle);
        mz_stream_buffered_delete(&buf_stream); // deletes base too
        throw std::runtime_error("Failed to open ZIP archive: " + zipPath);
    }

    return zip_handle; // zip_handle now owns buf_stream and os_stream
}


std::vector<unsigned char> readEntryToVector(void* zip_handle, const std::string& entryPath) {
    // 1. Locate the entry by path
    if (mz_zip_locate_entry(zip_handle, entryPath.c_str(), 0) != MZ_OK) {
        throw std::runtime_error("Entry not found in ZIP: " + entryPath);
    }

    // 2. Get entry info to pre-allocate vector
    mz_zip_file* file_info = nullptr;
    if (mz_zip_entry_get_info(zip_handle, &file_info) != MZ_OK || !file_info) {
        throw std::runtime_error("Failed to get entry info: " + entryPath);
    }

    std::vector<unsigned char> buffer;
    buffer.reserve(static_cast<size_t>(file_info->uncompressed_size));

    // 3. Open entry for reading
    if (mz_zip_entry_read_open(zip_handle, 0, nullptr) != MZ_OK) {
        throw std::runtime_error("Failed to open entry for reading: " + entryPath);
    }

    // 4. Read entry data in chunks
    constexpr size_t CHUNK_SIZE = 4096;
    unsigned char temp[CHUNK_SIZE];
    int bytesRead = 0;

    while ((bytesRead = mz_zip_entry_read(zip_handle, temp, CHUNK_SIZE)) > 0) {
        buffer.insert(buffer.end(), temp, temp + bytesRead);
    }

    // 5. Close entry
    if (mz_zip_entry_read_close(zip_handle, nullptr, nullptr, nullptr) != MZ_OK) {
        throw std::runtime_error("Failed to close ZIP entry: " + entryPath);
    }

    return buffer;
}

void closeZipHandle(void* zip_handle) {
    if (!zip_handle) return;

    // Close the ZIP archive
    if (mz_zip_close(zip_handle) != MZ_OK) {
        throw std::runtime_error("Failed to close ZIP archive");
    }

    // Delete the ZIP handle (this also deletes the internal buffered stream)
    mz_zip_delete(&zip_handle);

    // zip_handle is now null and all streams are cleaned up
}

ZipWriter initZip() {
    ZipWriter zw;

    // Create memory stream
    zw.mem_stream = mz_stream_mem_create();
    if (!zw.mem_stream) throw std::runtime_error("Failed to create memory stream");

    mz_stream_open(zw.mem_stream, nullptr, MZ_OPEN_MODE_CREATE);

    // Create ZIP handle
    zw.zip_handle = mz_zip_create();
    if (!zw.zip_handle) {
        mz_stream_mem_delete(&zw.mem_stream);
        throw std::runtime_error("Failed to create ZIP handle");
    }

    if (mz_zip_open(zw.zip_handle, zw.mem_stream, MZ_OPEN_MODE_WRITE) != MZ_OK) {
        mz_zip_delete(&zw.zip_handle);
        mz_stream_mem_delete(&zw.mem_stream);
        throw std::runtime_error("Failed to open ZIP archive");
    }

    return zw;
}

void addFileToZip(ZipWriter &zw, const std::string &filename, const std::vector<unsigned char> &data) {
    mz_zip_file file_info = {0};
    file_info.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
    file_info.filename = const_cast<char*>(filename.c_str());
    file_info.flag = MZ_ZIP_FLAG_UTF8;

    if (mz_zip_entry_write_open(zw.zip_handle, &file_info, MZ_COMPRESS_LEVEL_DEFAULT, 0, NULL) != MZ_OK)
        throw std::runtime_error("Failed to open ZIP entry: " + filename);

    if (!data.empty()) {
        mz_zip_entry_write(zw.zip_handle, data.data(), static_cast<int>(data.size()));
    }

    // Let minizip-ng compute CRC internally
    if (mz_zip_entry_write_close(zw.zip_handle, 0,
                                 static_cast<int64_t>(data.size()),
                                 static_cast<int64_t>(data.size())) != MZ_OK)
        throw std::runtime_error("Failed to close ZIP entry: " + filename);
}

void writeZipToDisk(ZipWriter &zw, const std::string &path) {
    if (!zw.zip_handle || !zw.mem_stream) throw std::runtime_error("ZIP not initialized");

    // Close ZIP handle
    if (mz_zip_close(zw.zip_handle) != MZ_OK) throw std::runtime_error("Failed to close ZIP");
    mz_zip_delete(&zw.zip_handle);

    // Write memory stream to disk
    void* os_stream = mz_stream_os_create();
    if (!os_stream) throw std::runtime_error("Failed to create OS stream");

    if (mz_stream_open(os_stream, path.c_str(), MZ_OPEN_MODE_CREATE) != MZ_OK) {
        mz_stream_os_delete(&os_stream);
        throw std::runtime_error("Failed to open output file: " + path);
    }

    const void* buffer = nullptr;
    if (mz_stream_mem_get_buffer(zw.mem_stream, &buffer) != MZ_OK) {
        mz_stream_os_delete(&os_stream);
    }
    auto size = mz_stream_mem_tell(zw.mem_stream);

    if (mz_stream_write(os_stream, buffer, static_cast<int>(size)) != size) {
        mz_stream_os_delete(&os_stream);
        throw std::runtime_error("Failed to write ZIP data to disk");
    }

    mz_stream_close(os_stream);
    mz_stream_os_delete(&os_stream);

    // Cleanup memory stream
    mz_stream_mem_delete(&zw.mem_stream);
}


EliseProjectFileData load(const std::string& path) {
    EliseProjectFileData data;

    auto handle = createZipHandle(path);

    data.json_data = readEntryToVector(handle, "show.elise");
    data.mp3_data = readEntryToVector(handle, "song.mp3");

    closeZipHandle(handle);
    return data;
}

void save(const std::string &path, const EliseProjectFileData &data) {
    auto zw = initZip();

    addFileToZip(zw, "show.elise", data.json_data);
    addFileToZip(zw, "song.mp3", data.mp3_data);

    writeZipToDisk(zw, path);
}
}
