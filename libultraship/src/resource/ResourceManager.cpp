#include "ResourceManager.h"
#include <spdlog/spdlog.h>
#include "resource/File.h"
#include "resource/archive/Archive.h"
#include <algorithm>
#include <thread>
#include "utils/StringHelper.h"
#include "utils/glob.h"
#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#include "xxhash_utils.h"

#ifdef __vita__
#include <vitasdk.h>
#endif

namespace Ship {

ResourceManager::ResourceManager() {
}

void ResourceManager::Init(const std::vector<std::string>& otrFiles, const std::unordered_set<uint32_t>& validHashes,
                           int32_t reservedThreadCount) {
    mResourceLoader = std::make_shared<ResourceLoader>();
    mArchiveManager = std::make_shared<ArchiveManager>();
    GetArchiveManager()->Init(otrFiles, validHashes);
#ifndef __vita__
    // the extra `- 1` is because we reserve an extra thread for spdlog
    size_t threadCount = std::max(1, (int32_t)(std::thread::hardware_concurrency() - reservedThreadCount - 1));
    mThreadPool = std::make_shared<BS::thread_pool>(threadCount);

    if (!DidLoadSuccessfully()) {
        // Nothing ever unpauses the thread pool since nothing will ever try to load the archive again.
        printf("Archive didn't load successfully!\n");
        mThreadPool->pause();
    }
#endif
}

ResourceManager::~ResourceManager() {
    SPDLOG_INFO("destruct ResourceManager");
}

bool ResourceManager::DidLoadSuccessfully() {
    return mArchiveManager != nullptr && mArchiveManager->IsArchiveLoaded();
}

std::shared_ptr<File> ResourceManager::LoadFileProcess(const std::string& filePath,
                                                       std::shared_ptr<ResourceInitData> initData) {
    auto file = mArchiveManager->LoadFile(filePath, initData);
    if (file != nullptr) {
        SPDLOG_TRACE("Loaded File {} on ResourceManager", file->InitData->Path);
    } else {
        SPDLOG_TRACE("Could not load File {} in ResourceManager", filePath);
    }
    return file;
}

std::shared_ptr<IResource> ResourceManager::LoadResourceProcess(const std::string& filePath, bool loadExact,
                                                                std::shared_ptr<ResourceInitData> initData) {
    // Check for and remove the OTR signature
    if (OtrSignatureCheck(filePath.c_str())) {
        const auto newFilePath = filePath.substr(7);
        return LoadResourceProcess(newFilePath, false, initData);
    }

    // Attempt to load the alternate version of the asset, if we fail then we continue trying to load the standard
    // asset.
    /*if (!loadExact && mAltAssetsEnabled && !filePath.starts_with(IResource::gAltAssetPrefix)) {
        const auto altPath = IResource::gAltAssetPrefix + filePath;
        auto altResource = LoadResourceProcess(altPath, loadExact, initData);

        if (altResource != nullptr) {
            return altResource;
        }
    }*/

    // While waiting in the queue, another thread could have loaded the resource.
    // In a last attempt to avoid doing work that will be discarded, let's check if the cached version exists.
    auto cacheLine = CheckCache(filePath, loadExact);
    auto cachedResource = GetCachedResource(cacheLine);
    if (cachedResource != nullptr) {
        return cachedResource;
    }

    // Check for resource load errors which can indicate an alternate asset.
    // If we are attempting to load an alternate asset, we can return null
    /*if (!loadExact && mAltAssetsEnabled && filePath.starts_with(IResource::gAltAssetPrefix)) {
        if (std::holds_alternative<ResourceLoadError>(cacheLine)) {
            try {
                // If we have attempted to cache an alternate asset, but failed, we return nullptr and rely on the
                // calling function to return a regular asset. If we have NOT attempted load already, attempt the load.
                auto loadError = std::get<ResourceLoadError>(cacheLine);
                if (loadError != ResourceLoadError::NotCached) {
                    return nullptr;
                }
            } catch (std::bad_variant_access const& e) {
                // Ignore the exception. This should never happen. The last check should've returned the resource.
            }
        }
    }*/

    // Get the file from the OTR
    auto file = LoadFileProcess(filePath, initData);
    if (file == nullptr) {
        SPDLOG_TRACE("Failed to load resource file at path {}", filePath);
    }

    // Transform the raw data into a resource
    auto resource = GetResourceLoader()->LoadResource(file);

    // Another thread could have loaded the resource while we were processing, so we want to check before setting to
    // the cache.
    uint64_t hash = XXH3_64bits(filePath.c_str(), filePath.size());
    cachedResource = GetCachedResource(hash, true);
    {
        //const std::lock_guard<std::mutex> lock(mMutex);

        if (cachedResource != nullptr) {
            // If another thread has already loaded this resource, discard the work we already did and return from
            // cache.
            resource = cachedResource;
        }

        // Set the cache to the loaded resource
        if (resource != nullptr) {
            mResourceCache[hash] = resource;
        } else {
            mResourceCache[hash] = ResourceLoadError::NotFound;
        }
    }

    if (resource != nullptr) {
        SPDLOG_TRACE("Loaded Resource {} on ResourceManager", filePath);
    } else {
        SPDLOG_TRACE("Resource load FAILED {} on ResourceManager", filePath);
    }

    return resource;
}

#ifdef __vita__
std::shared_ptr<IResource>
ResourceManager::LoadResourceAsync(const std::string& filePath, bool loadExact,
#else
std::shared_future<std::shared_ptr<IResource>>
ResourceManager::LoadResourceAsync(const std::string& filePath, bool loadExact, BS::priority_t priority,
#endif
                                   std::shared_ptr<ResourceInitData> initData) {
    // Check for and remove the OTR signature
    if (OtrSignatureCheck(filePath.c_str())) {
        auto newFilePath = filePath.substr(7);
#ifdef __vita__
        return LoadResourceAsync(newFilePath, loadExact);
#else
        return LoadResourceAsync(newFilePath, loadExact, priority);
#endif
    }

    // Check the cache before queueing the job.
    auto cacheCheck = GetCachedResource(filePath, loadExact);
    if (cacheCheck) {
        return cacheCheck;
    }

    const auto newFilePath = std::string(filePath);
    
	
#ifdef __vita__
	return LoadResourceProcess(newFilePath, loadExact, initData);
    //return mThreadPool->submit(
    //    std::bind(&ResourceManager::LoadResourceProcess, this, newFilePath, loadExact, initData));
#else
    return mThreadPool->submit_task(
        std::bind(&ResourceManager::LoadResourceProcess, this, newFilePath, loadExact, initData), priority);
#endif
}

std::shared_ptr<IResource> ResourceManager::LoadResource(const std::string& filePath, bool loadExact,
                                                         std::shared_ptr<ResourceInitData> initData) {
#ifdef __vita__
    auto resource = LoadResourceAsync(filePath, loadExact, initData);
#else
    auto resource = LoadResourceAsync(filePath, loadExact, BS::pr::highest, initData).get();
#endif
    if (resource == nullptr) {
        SPDLOG_ERROR("Failed to load resource file at path {}", filePath);
    }
    return resource;
}

std::variant<ResourceManager::ResourceLoadError, std::shared_ptr<IResource>>
ResourceManager::CheckCache(const std::string& filePath, bool loadExact) {
    /*if (!loadExact && mAltAssetsEnabled && !filePath.starts_with(IResource::gAltAssetPrefix)) {
        const auto altPath = IResource::gAltAssetPrefix + filePath;
        auto altCacheResult = CheckCache(altPath, loadExact);

        // If the type held at this cache index is a resource, then we return it.
        // Else we attempt to load standard definition assets.
        if (std::holds_alternative<std::shared_ptr<IResource>>(altCacheResult)) {
            return altCacheResult;
        }
    }*/

    //const std::lock_guard<std::mutex> lock(mMutex);

    auto resourceCacheFind = mResourceCache.find(XXH3_64bits(filePath.c_str(), filePath.size()));
    if (resourceCacheFind == mResourceCache.end()) {
        return ResourceLoadError::NotCached;
    }

    return resourceCacheFind->second;
}

std::variant<ResourceManager::ResourceLoadError, std::shared_ptr<IResource>>
ResourceManager::CheckCache(uint64_t hash, bool loadExact) {
    /*if (!loadExact && mAltAssetsEnabled && !filePath.starts_with(IResource::gAltAssetPrefix)) {
        const auto altPath = IResource::gAltAssetPrefix + filePath;
        auto altCacheResult = CheckCache(altPath, loadExact);

        // If the type held at this cache index is a resource, then we return it.
        // Else we attempt to load standard definition assets.
        if (std::holds_alternative<std::shared_ptr<IResource>>(altCacheResult)) {
            return altCacheResult;
        }
    }*/

    //const std::lock_guard<std::mutex> lock(mMutex);

    auto resourceCacheFind = mResourceCache.find(hash);
    if (resourceCacheFind == mResourceCache.end()) {
        return ResourceLoadError::NotCached;
    }

    return resourceCacheFind->second;
}

std::shared_ptr<IResource>ResourceManager::GetCachedResource(const std::string& filePath, bool loadExact) {
    // Gets the cached resource based on filePath.
    return GetCachedResource(CheckCache(filePath, loadExact));
}

std::shared_ptr<IResource> ResourceManager::GetCachedResource(uint64_t hash, bool loadExact) {
    // Gets the cached resource based on hash.
    return GetCachedResource(CheckCache(hash, loadExact));
}

std::shared_ptr<IResource>
ResourceManager::GetCachedResource(std::variant<ResourceLoadError, std::shared_ptr<IResource>> cacheLine) {
    // Gets the cached resource based on a cache line std::variant from the cache map.
    if (std::holds_alternative<std::shared_ptr<IResource>>(cacheLine)) {
        try {
            auto resource = std::get<std::shared_ptr<IResource>>(cacheLine);

            if (resource.use_count() <= 0) {
                return nullptr;
            }

            if (resource->IsDirty()) {
                return nullptr;
            }

            return resource;
        } catch (std::bad_variant_access const& e) {
            // Ignore the exception
        }
    }

    return nullptr;
}

#ifdef __vita__
std::shared_ptr<std::vector<std::shared_ptr<IResource>>>
ResourceManager::LoadDirectoryAsync(const std::string& searchMask) {
#else
std::shared_ptr<std::vector<std::shared_future<std::shared_ptr<IResource>>>>
ResourceManager::LoadDirectoryAsync(const std::string& searchMask, BS::priority_t priority) {
#endif
#ifdef __vita__
    auto fileList = GetArchiveManager()->ListFiles(searchMask);
    auto loadedList = std::make_shared<std::vector<std::shared_ptr<IResource>>>();
    for (size_t i = 0; i < fileList->size(); i++) {
        loadedList->push_back(LoadResourceAsync(fileList->operator[](i), false));
    }
    return loadedList;
#else
    auto loadedList = std::make_shared<std::vector<std::shared_future<std::shared_ptr<IResource>>>>();
    auto fileList = GetArchiveManager()->ListFiles(searchMask);
    loadedList->reserve(fileList->size());

    for (size_t i = 0; i < fileList->size(); i++) {
        auto fileName = std::string(fileList->operator[](i));
        auto future = LoadResourceAsync(fileName, false, priority);
        loadedList->push_back(future);
    }
	
	return loadedList;
#endif
}

std::shared_ptr<std::vector<std::shared_ptr<IResource>>> ResourceManager::LoadDirectory(const std::string& searchMask) {
#ifdef __vita__
    return LoadDirectoryAsync(searchMask);
#else
    auto futureList = LoadDirectoryAsync(searchMask, true);
    auto loadedList = std::make_shared<std::vector<std::shared_ptr<IResource>>>();

    for (size_t i = 0; i < futureList->size(); i++) {
        const auto future = futureList->at(i);
        const auto resource = future.get();
        loadedList->push_back(resource);
    }

    return loadedList;
#endif
}

void ResourceManager::DirtyDirectory(const std::string& searchMask) {
    auto list = GetArchiveManager()->ListFiles(searchMask);

    for (const auto& key : *list.get()) {
        auto resource = GetCachedResource(key);
        // If it's a resource, we will set the dirty flag, else we will just unload it.
        if (resource != nullptr) {
            resource->Dirty();
        } else {
            UnloadResource(key);
        }
    }
}

void ResourceManager::UnloadDirectory(const std::string& searchMask) {
    auto list = GetArchiveManager()->ListFiles(searchMask);

    for (const auto& key : *list.get()) {
        UnloadResource(key);
    }
}

std::shared_ptr<ArchiveManager> ResourceManager::GetArchiveManager() {
    return mArchiveManager;
}

std::shared_ptr<ResourceLoader> ResourceManager::GetResourceLoader() {
    return mResourceLoader;
}

size_t ResourceManager::UnloadResource(const std::string& filePath) {
    // Store a shared pointer here so that erase doesn't destruct the resource.
    // The resource will attempt to load other resources on the destructor, and this will fail because we already hold
    // the mutex.
    std::variant<ResourceLoadError, std::shared_ptr<IResource>> value = nullptr;
    size_t ret = 0;
    {
        //const std::lock_guard<std::mutex> lock(mMutex);
        uint64_t hash = XXH3_64bits(filePath.c_str(), filePath.size());
        value = mResourceCache[hash];
        ret = mResourceCache.erase(hash);
    }

    return ret;
}

bool ResourceManager::OtrSignatureCheck(const char* fileName) {
    static const char* sOtrSignature = "__OTR__";
    return strncmp(fileName, sOtrSignature, strlen(sOtrSignature)) == 0;
}

bool ResourceManager::IsAltAssetsEnabled() {
    return mAltAssetsEnabled;
}

void ResourceManager::SetAltAssetsEnabled(bool isEnabled) {
    mAltAssetsEnabled = isEnabled;
}

} // namespace Ship
