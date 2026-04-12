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
                                                                std::shared_ptr<ResourceInitData> initData, uint64_t hash) {
    // Check for and remove the OTR signature
    if (!hash) {
		if (OtrSignatureCheck(filePath.c_str())) {
			const auto newFilePath = filePath.substr(7);
			hash = XXH3_64bits(newFilePath.c_str(), newFilePath.size());
			return LoadResourceProcess(newFilePath, false, initData, hash);
		}
		
		hash = XXH3_64bits(filePath.c_str(), filePath.size());
	}
	
	auto cachedResource = CheckCache(hash, loadExact);
	if (cachedResource != nullptr) {
		return cachedResource;
	}

    // Get the file from the OTR
    auto file = LoadFileProcess(filePath, initData);
    if (file == nullptr) {
        SPDLOG_TRACE("Failed to load resource file at path {}", filePath);
    }

    // Transform the raw data into a resource
    auto resource = GetResourceLoader()->LoadResource(file);
    if (resource != nullptr)
		mResourceCache[hash] = resource;

    if (resource != nullptr) {
        SPDLOG_TRACE("Loaded Resource {} on ResourceManager", filePath);
    } else {
        SPDLOG_TRACE("Resource load FAILED {} on ResourceManager", filePath);
    }

    return resource;
}

std::shared_ptr<IResource> ResourceManager::LoadResourceAsync(const char *filePath, bool loadExact, std::shared_ptr<ResourceInitData> initData, size_t sz) {
	uint64_t hash = XXH3_64bits(filePath, sz);

    // Check the cache before queueing the job.
    auto cacheCheck = GetCachedResource(hash, loadExact);
    if (cacheCheck) {
        return cacheCheck;
    }

    return LoadResourceProcess(filePath, loadExact, initData, hash);
}

std::shared_ptr<IResource>
ResourceManager::LoadResourceAsync(const std::string& filePath, bool loadExact, std::shared_ptr<ResourceInitData> initData) {
    // Check for and remove the OTR signature
    if (OtrSignatureCheck(filePath.c_str())) {
        return LoadResourceAsync(&filePath.c_str()[7], loadExact, initData, filePath.size() - 7);
    }

    // Check the cache before queueing the job.
	uint64_t hash = XXH3_64bits(filePath.c_str(), filePath.size());
    auto cacheCheck = GetCachedResource(hash, loadExact);
    if (cacheCheck) {
        return cacheCheck;
    }

	return LoadResourceProcess(filePath, loadExact, initData, hash);
}

std::shared_ptr<IResource> ResourceManager::LoadResource(const std::string& filePath, bool loadExact, std::shared_ptr<ResourceInitData> initData) {
    return LoadResourceAsync(filePath, loadExact, initData);
}

std::shared_ptr<IResource> ResourceManager::CheckCache(const std::string& filePath, bool loadExact) {
    auto resourceCacheFind = mResourceCache.find(XXH3_64bits(filePath.c_str(), filePath.size()));
    if (resourceCacheFind == mResourceCache.end()) {
        return nullptr;
    }

    return resourceCacheFind->second;
}

std::shared_ptr<IResource> ResourceManager::CheckCache(uint64_t hash, bool loadExact) {
    auto resourceCacheFind = mResourceCache.find(hash);
    if (resourceCacheFind == mResourceCache.end()) {
        return nullptr;
    }

    return resourceCacheFind->second;
}

std::shared_ptr<IResource>ResourceManager::GetCachedResource(const std::string& filePath, bool loadExact) {
    // Gets the cached resource based on filePath.
    return CheckCache(filePath, loadExact);
}

std::shared_ptr<IResource> ResourceManager::GetCachedResource(uint64_t hash, bool loadExact) {
    // Gets the cached resource based on hash.
    return CheckCache(hash, loadExact);
}

std::shared_ptr<std::vector<std::shared_ptr<IResource>>>
ResourceManager::LoadDirectoryAsync(const std::string& searchMask) {
    auto fileList = GetArchiveManager()->ListFiles(searchMask);
    auto loadedList = std::make_shared<std::vector<std::shared_ptr<IResource>>>();
    for (size_t i = 0; i < fileList->size(); i++) {
        loadedList->push_back(LoadResourceAsync(fileList->operator[](i), false));
    }
    return loadedList;
}

std::shared_ptr<std::vector<std::shared_ptr<IResource>>> ResourceManager::LoadDirectory(const std::string& searchMask) {
    return LoadDirectoryAsync(searchMask);
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
    std::shared_ptr<IResource> value = nullptr;
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
    return fileName[0] == '_';
}

bool ResourceManager::IsAltAssetsEnabled() {
    return mAltAssetsEnabled;
}

void ResourceManager::SetAltAssetsEnabled(bool isEnabled) {
    mAltAssetsEnabled = isEnabled;
}

} // namespace Ship
