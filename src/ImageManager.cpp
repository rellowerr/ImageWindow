#include "ImageManager.hpp"
#include "ImageOverlay.hpp"

#include <Geode/utils/file.hpp>

using namespace geode::prelude;

ImageManager* ImageManager::s_instance = nullptr;

ImageManager* ImageManager::get() {
    if (!s_instance) {
        s_instance = new ImageManager();
    }
    return s_instance;
}

ImageManager::ImageManager() {
    init();
}

ImageManager::~ImageManager() {
    clearCache();
    if (m_activeTexture) {
        m_activeTexture->release();
        m_activeTexture = nullptr;
    }
}

void ImageManager::init() {
    auto saveDir = Mod::get()->getSaveDir();
    m_imagesDir = saveDir / "images";

    std::error_code ec;
    if (!std::filesystem::exists(m_imagesDir, ec)) {
        std::filesystem::create_directories(m_imagesDir, ec);
    }

    m_activeImageName = Mod::get()->getSavedValue<std::string>("active-image", "");
    refreshImages();
}

void ImageManager::clearCache() {
    for (auto& [path, tex] : m_textureCache) {
        if (tex) {
            tex->release();
        }
    }
    m_textureCache.clear();
}

std::string ImageManager::formatFileSize(uintmax_t bytes) {
    if (bytes < 1024) {
        return fmt::format("{} B", bytes);
    } else if (bytes < 1024 * 1024) {
        return fmt::format("{:.1f} KB", static_cast<double>(bytes) / 1024.0);
    } else {
        return fmt::format("{:.1f} MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    }
}

void ImageManager::refreshImages() {
    m_images.clear();

    std::error_code ec;
    if (!std::filesystem::exists(m_imagesDir, ec)) {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(m_imagesDir, ec)) {
        if (!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().string();
        for (auto& c : ext) c = static_cast<char>(std::tolower(c));

        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".webp") {
            ImageItem item;
            item.filename = entry.path().filename().string();
            item.fullPath = entry.path();
            item.fileSize = entry.file_size(ec);

            // Probe dimensions if possible
            auto* tex = loadTexture(item.fullPath);
            if (tex) {
                item.dimensions = tex->getContentSize();
            }

            m_images.push_back(item);
        }
    }

    // Sort alphabetically by filename
    std::sort(m_images.begin(), m_images.end(), [](const ImageItem& a, const ImageItem& b) {
        return a.filename < b.filename;
    });

    // Validate active image selection
    if (!m_activeImageName.empty()) {
        bool found = false;
        for (const auto& img : m_images) {
            if (img.filename == m_activeImageName) {
                found = true;
                break;
            }
        }
        if (!found) {
            m_activeImageName = m_images.empty() ? "" : m_images.front().filename;
            Mod::get()->setSavedValue("active-image", m_activeImageName);
        }
    } else if (!m_images.empty()) {
        m_activeImageName = m_images.front().filename;
        Mod::get()->setSavedValue("active-image", m_activeImageName);
    }

    // Reload active texture
    if (m_activeTexture) {
        m_activeTexture->release();
        m_activeTexture = nullptr;
    }

    if (!m_activeImageName.empty()) {
        auto activePath = m_imagesDir / m_activeImageName;
        m_activeTexture = loadTexture(activePath);
        if (m_activeTexture) {
            m_activeTexture->retain();
        }
    }
}

std::string ImageManager::getActiveImageName() {
    return m_activeImageName;
}

void ImageManager::setActiveImage(std::string const& filename) {
    m_activeImageName = filename;
    Mod::get()->setSavedValue("active-image", filename);

    if (m_activeTexture) {
        m_activeTexture->release();
        m_activeTexture = nullptr;
    }

    if (!filename.empty()) {
        auto activePath = m_imagesDir / filename;
        m_activeTexture = loadTexture(activePath);
        if (m_activeTexture) {
            m_activeTexture->retain();
        }
    }

    ImageOverlay::updateAll();
}

bool ImageManager::importImage(std::filesystem::path const& sourcePath) {
    if (!std::filesystem::exists(sourcePath)) {
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(m_imagesDir, ec);

    auto targetFilename = sourcePath.filename();
    auto targetPath = m_imagesDir / targetFilename;

    // Check if the user selected a file already inside the library folder
    if (std::filesystem::equivalent(sourcePath, targetPath, ec)) {
        setActiveImage(targetFilename.string());
        return true;
    }

    // Deduplicate filename if necessary
    int counter = 1;
    while (std::filesystem::exists(targetPath, ec)) {
        auto stem = sourcePath.stem().string();
        auto ext = sourcePath.extension().string();
        targetFilename = fmt::format("{} ({}){}", stem, counter++, ext);
        targetPath = m_imagesDir / targetFilename;
    }

    std::filesystem::copy_file(sourcePath, targetPath, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        log::error("Failed to copy image: {}", ec.message());
        return false;
    }

    refreshImages();
    setActiveImage(targetFilename.string());
    return true;
}

bool ImageManager::deleteImage(std::string const& filename) {
    auto path = m_imagesDir / filename;
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        std::filesystem::remove(path, ec);
    }

    // Invalidate cached texture for this file
    auto cached = m_textureCache.find(path.string());
    if (cached != m_textureCache.end()) {
        if (cached->second) {
            cached->second->release();
        }
        m_textureCache.erase(cached);
    }

    refreshImages();
    ImageOverlay::updateAll();
    return true;
}

void ImageManager::openImagesFolder() {
    geode::utils::file::openFolder(m_imagesDir);
}

bool ImageManager::pickAndImportImage() {
    file::FilePickOptions options;
    options.filters.push_back({
        "Images",
        { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.webp" }
    });

    file::pick(file::PickMode::OpenFile, options)
        .listen([this](Result<std::filesystem::path>* result) {
            if (result && result->isOk()) {
                this->importImage(result->unwrap());
            }
        });
    return true;
}

cocos2d::CCTexture2D* ImageManager::loadTexture(std::filesystem::path const& path) {
    auto pathStr = path.string();
    auto cached = m_textureCache.find(pathStr);
    if (cached != m_textureCache.end() && cached->second) {
        return cached->second;
    }

    auto dataRes = geode::utils::file::readBinary(path);
    if (!dataRes) {
        return nullptr;
    }

    auto const& data = dataRes.unwrap();
    if (data.empty()) {
        return nullptr;
    }

    auto* image = new CCImage();
    if (!image->initWithImageData(const_cast<unsigned char*>(data.data()), static_cast<int>(data.size()))) {
        image->release();
        return nullptr;
    }

    auto* texture = new CCTexture2D();
    if (!texture->initWithImage(image)) {
        image->release();
        texture->release();
        return nullptr;
    }
    image->release();

    m_textureCache[pathStr] = texture;
    return texture;
}

cocos2d::CCTexture2D* ImageManager::getActiveTexture() {
    if (!m_activeTexture && !m_activeImageName.empty()) {
        auto activePath = m_imagesDir / m_activeImageName;
        m_activeTexture = loadTexture(activePath);
        if (m_activeTexture) {
            m_activeTexture->retain();
        }
    }
    return m_activeTexture;
}
