#include "ExporterArchiveO2R.h"
#include "Utils/StringHelper.h"
#include <ship/utils/StrHash64.h>
#include <filesystem>
#include <spdlog/spdlog.h>

ExporterArchiveO2R::ExporterArchiveO2R(const std::string& path, bool enableWriting) {
    mPath = path;
    mZip = nullptr;
}

ExporterArchiveO2R::~ExporterArchiveO2R() {
    Unload();
}

bool ExporterArchiveO2R::Load(bool enableWriting) {
    int openErr;
    zip_t* archive = zip_open(mPath.c_str(), ZIP_CHECKCONS, &openErr);

    if (archive == nullptr) {
        zip_error_t error;
        zip_error_init_with_code(&error, openErr);
        SPDLOG_ERROR("Failed to open ZIP (O2R) file. Error: {}", zip_error_strerror(&error));
        zip_error_fini(&error);
        return false;
    }

    SPDLOG_INFO("Loaded ZIP (O2R) archive: {}", mPath.c_str());
    mZip = archive;

    return true;
}

bool ExporterArchiveO2R::Unload() {
    printf("Unload\n");
    int err;
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        err = zip_close(mZip);
    }
    if (err < 0) {
        zip_error_t* zipError = zip_get_error(mZip);
        SPDLOG_ERROR("Failed to close ZIP (O2R) file. Error: {}", zip_error_strerror(zipError));
        printf("fail\n");
        zip_error_fini(zipError);
        return false;
    }

    return true;
}

int ExporterArchiveO2R::CreateArchive([[maybe_unused]] size_t fileCapacity) {
    int openErr;
    zip_t* zip;
    
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        zip = zip_open(mPath.c_str(), ZIP_CREATE, &openErr);
    }
    
    if (zip == nullptr) {
        zip_error_t error;
        zip_error_init_with_code(&error, openErr);
        SPDLOG_ERROR("Failed to create ZIP (O2R) file. Error: {}", zip_error_strerror(&error));
        zip_error_fini(&error);
        return -1;
    }
    mZip = zip;
    SPDLOG_INFO("Loaded ZIP (O2R) archive: {}", mPath.c_str());
    return 0;
}

bool ExporterArchiveO2R::AddFile(const std::string& filePath, void* fileData, size_t fileSize) {
    zip_source_t* source = zip_source_buffer(mZip, fileData, fileSize, 0);

    if (source == nullptr) {
        zip_error_t* zipError = zip_get_error(mZip);
        SPDLOG_ERROR("Failed to create ZIP source. Error: {}", zip_error_strerror(zipError));
        zip_source_free(source);
        zip_error_fini(zipError);
        return false;
    }

    if (zip_file_add(mZip, filePath.c_str(), source, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0) {
        zip_error_t* zipError = zip_get_error(mZip);
        SPDLOG_ERROR("Failed to add file to ZIP. Error: {}", zip_error_strerror(zipError));
        zip_source_free(source);
        zip_error_fini(zipError);
        return false;
    }

    return true;
}
