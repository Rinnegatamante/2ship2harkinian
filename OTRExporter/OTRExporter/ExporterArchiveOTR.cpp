#ifdef INCLUDE_MPQ_SUPPORT
#include "ExporterArchiveOTR.h"
#include "Utils/StringHelper.h"
#include <ship/utils/StrHash64.h>
#include <filesystem>

ExporterArchiveOtr::ExporterArchiveOtr(const std::string& path, bool enableWriting) {
    mPath = path;
    mMpq = nullptr;
}

ExporterArchiveOtr::~ExporterArchiveOtr() {
    Unload();
}

bool ExporterArchiveOtr::Load(bool enableWriting) {
    HANDLE mpqHandle = NULL;

    bool baseLoaded = false;
    std::string fullPath = std::filesystem::absolute(mPath).string();

    bool openArchiveSuccess;
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        openArchiveSuccess = SFileOpenArchive(fullPath.c_str(), 0, enableWriting ? 0 : MPQ_OPEN_READ_ONLY, &mpqHandle);
    }
    if (openArchiveSuccess) {
        printf("Opened mpq file %s\n", fullPath.c_str());
        mMpq = mpqHandle;
        mPath = fullPath;       
        baseLoaded = true;
    }

    if (!baseLoaded) {
        printf("No valid OTR file was provided.");
        return false;
    }

    return true;
}

bool ExporterArchiveOtr::Unload() {
    bool success = true;

    bool closeArchiveSuccess;
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        closeArchiveSuccess = SFileCloseArchive(mMpq);
    }
    if (!closeArchiveSuccess) {
        printf("Failed to close mpq\n");
        success = false;
    }

    mMpq = nullptr;

    return success;
}

int ExporterArchiveOtr::CreateArchive(size_t fileCapacity) {
    bool success;
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        success = SFileCreateArchive(mPath.c_str(), MPQ_CREATE_LISTFILE | MPQ_CREATE_ATTRIBUTES | MPQ_CREATE_ARCHIVE_V2,
                                     static_cast<DWORD>(fileCapacity), &mMpq);
    }
    int32_t error = GetLastError();

    if (success) {
        return 0;
    } else {
        printf("We tried to create an archive, but it has fallen and cannot get up.\n");
        return -1;
    }
}

bool ExporterArchiveOtr::AddFile(const std::string& filePath, void* fileData, size_t fileSize) {
    HANDLE hFile;
#ifdef _WIN32
    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);
    FILETIME t;
    SystemTimeToFileTime(&sysTime, &t);
    ULONGLONG theTime = static_cast<uint64_t>(t.dwHighDateTime) << (sizeof(t.dwHighDateTime) * 8) | t.dwLowDateTime;
#else
    time_t theTime;
    time(&theTime);
#endif

    std::string updatedPath = filePath;

    StringHelper::ReplaceOriginal(updatedPath, "\\", "/");

    bool createFileSuccess;
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        createFileSuccess =
            SFileCreateFile(mMpq, updatedPath.c_str(), theTime, static_cast<DWORD>(fileSize), 0, MPQ_FILE_COMPRESS, &hFile);
    }
    if (!createFileSuccess) {
        printf("Failed to create file.\n");
        return false;
    }

    bool writeFileSuccess;
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        writeFileSuccess = SFileWriteFile(hFile, fileData, static_cast<DWORD>(fileSize), MPQ_COMPRESSION_ZLIB);
    }
    if (!writeFileSuccess) {
        printf("Failed to write.\n");
        bool closeFileSuccess;
        {
            const std::lock_guard<std::mutex> lock(mMutex);
            closeFileSuccess = SFileCloseFile(hFile);
        }
        if (!closeFileSuccess) {
            printf("Failed to close.\n");
        }
        return false;
    }

    bool finishFileSuccess;
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        finishFileSuccess = SFileFinishFile(hFile);
    }
    if (!finishFileSuccess) {
        printf("Failed to finish file.\n");
        bool closeFileSuccess;
        {
            const std::lock_guard<std::mutex> lock(mMutex);
            closeFileSuccess = SFileCloseFile(hFile);
        }
        if (!closeFileSuccess) {
            printf("Failed to close after finish failure.\n");
        }
        return false;
    }
    // SFileFinishFile already frees the handle, so no need to close it again.

    mAddedFiles.push_back(updatedPath);
    mHashes[CRC64(updatedPath.c_str())] = updatedPath;

    return true;
}
#endif