#pragma once

#include <Geode/Geode.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

struct ImageItem {
    std::string filename;
    std::filesystem::path fullPath;
    cocos2d::CCSize dimensions = { 0.f, 0.f };
    uintmax_t fileSize = 0;
};

class ImageManager {
private:
    static ImageManager* s_instance;

    std::filesystem::path m_imagesDir;
    std::vector<ImageItem> m_images;
    std::string m_activeImageName;
    std::unordered_map<std::string, cocos2d::CCTexture2D*> m_textureCache;
    cocos2d::CCTexture2D* m_activeTexture = nullptr;

    ImageManager();
    ~ImageManager();

public:
    static ImageManager* get();

    void init();
    std::filesystem::path const& getImagesDir() const { return m_imagesDir; }
    std::vector<ImageItem> const& getImages() const { return m_images; }

    void refreshImages();
    std::string getActiveImageName();
    void setActiveImage(std::string const& filename);

    bool importImage(std::filesystem::path const& sourcePath);
    bool deleteImage(std::string const& filename);
    void openImagesFolder();
    bool pickAndImportImage();

    cocos2d::CCTexture2D* loadTexture(std::filesystem::path const& path);
    cocos2d::CCTexture2D* getActiveTexture();
    void clearCache();

    // Helper to format file sizes nicely (e.g. 1.2 MB, 450 KB)
    static std::string formatFileSize(uintmax_t bytes);
};
