#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <windows.h>

namespace fs = std::filesystem;

#define GREEN "\033[32m"
#define RESET "\033[0m"

std::string findMaxonFolder() {
    char* appdataPath = nullptr;
    size_t len = 0;
    if (_dupenv_s(&appdataPath, &len, "APPDATA") != 0 || appdataPath == nullptr) {
        std::cerr << "Error: Unable to retrieve APPDATA environment variable." << std::endl;
        return "";
    }

    std::string basePath = std::string(appdataPath) + "\\Maxon\\";
    std::string folderPrefix = "Maxon Cinema 4D 2024";
    free(appdataPath);

    for (const auto& entry : fs::directory_iterator(basePath)) {
        if (entry.is_directory()) {
            std::string folderName = entry.path().filename().string();
            if (folderName.find(folderPrefix) == 0) {
                return entry.path().string();
            }
        }
    }
    return "";
}

void showProgressBar(float progress) {
    int barWidth = 50;
    std::cout << GREEN << "[";
    int pos = static_cast<int>(barWidth * progress);
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << static_cast<int>(progress * 100.0) << "%\r" << RESET;
    std::cout.flush();
}

size_t countTotalFiles(const fs::path& folder) {
    size_t total = 0;
    for (const auto& entry : fs::recursive_directory_iterator(folder)) {
        if (fs::is_regular_file(entry)) total++;
    }
    return total;
}

void copyFolderWithProgress(const fs::path& source, const fs::path& destination, size_t& copiedFiles, size_t totalFiles) {
    try {
        fs::create_directories(destination);
        for (const auto& entry : fs::recursive_directory_iterator(source)) {
            auto destPath = destination / fs::relative(entry.path(), source);
            if (entry.is_directory()) {
                fs::create_directories(destPath);
            }
            else {
                fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
                copiedFiles++;
                showProgressBar(static_cast<float>(copiedFiles) / totalFiles);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error copying from " << source << ": " << e.what() << std::endl;
    }
}

int main() {
    SetConsoleTitleA("Maxon Cinema 4D 2024 Plugin Installer");
    std::string baseMaxonPath = findMaxonFolder();
    if (baseMaxonPath.empty()) {
        std::cerr << "No valid Maxon Cinema 4D folder found!" << std::endl;
        return 1;
    }

    fs::path pluginsPath = fs::path(baseMaxonPath) / "plugins";
    fs::path libraryPath = fs::path(baseMaxonPath) / "library";
    fs::path currentPath = fs::current_path();

    fs::path libPath = currentPath / "lib";
    fs::path extrasPath = currentPath / "Extras";

    size_t totalFiles = 0;
    size_t copiedFiles = 0;

    // Count total files from lib folder
    if (fs::exists(libPath) && fs::is_directory(libPath)) {
        for (const auto& entry : fs::directory_iterator(libPath)) {
            if (entry.is_directory()) {
                totalFiles += countTotalFiles(entry.path());
            }
        }
    }

    // Count total files from Extras folder
    if (fs::exists(extrasPath) && fs::is_directory(extrasPath)) {
        totalFiles += countTotalFiles(extrasPath);
    }

    if (totalFiles == 0) {
        std::cerr << "No files to copy!" << std::endl;
        return 1;
    }

    std::cout << "Starting copy with total files: " << totalFiles << std::endl;

    // Copy plugins from lib to plugins folder
    if (fs::exists(libPath) && fs::is_directory(libPath)) {
        for (const auto& entry : fs::directory_iterator(libPath)) {
            if (entry.is_directory()) {
                std::cout << "Copying plugin: " << entry.path().filename().string() << std::endl;
                copyFolderWithProgress(entry.path(), pluginsPath / entry.path().filename(), copiedFiles, totalFiles);
            }
        }
    }
    else {
        std::cerr << "'lib' folder not found!" << std::endl;
    }

    // Copy Extras to Library
    if (fs::exists(extrasPath) && fs::is_directory(extrasPath)) {
        std::cout << "Copying Extras to Library..." << std::endl;
        copyFolderWithProgress(extrasPath, libraryPath, copiedFiles, totalFiles);
    }
    else {
        std::cerr << "'Extras' folder not found!" << std::endl;
    }

    std::cout << std::endl << GREEN << "Plugin Installed Successfully!" << RESET << std::endl;
    std::cout << GREEN << "Developed By - !EAGLE" << RESET << std::endl;

    system("pause");
}
