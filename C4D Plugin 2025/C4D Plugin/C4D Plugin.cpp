#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <windows.h>
#include <cstdlib>
#include <ctime>

namespace fs = std::filesystem;

#define RESET "\033[0m"

// Forward declaration for random color function
std::string randomColor();

// Generate a random color ANSI code
std::string randomColor() {
    int colors[] = { 31, 32, 33, 34, 35, 36, 37 }; // Red, Green, Yellow, Blue, Magenta, Cyan, White
    return "\033[" + std::to_string(colors[rand() % 7]) + "m";
}

std::string findMaxonFolder() {
    char* appdataPath = nullptr;
    size_t len = 0;
    if (_dupenv_s(&appdataPath, &len, "APPDATA") != 0 || appdataPath == nullptr) {
        std::cerr << randomColor() << "Error: Unable to retrieve APPDATA environment variable." << RESET << std::endl;
        return "";
    }

    std::string basePath = std::string(appdataPath) + "\\Maxon\\";
    std::string folderPrefix = "Maxon Cinema 4D 2025";
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
    std::cout << randomColor() << "[";
    int pos = static_cast<int>(barWidth * progress);
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << static_cast<int>(progress * 100.0) << "%" << RESET << "\r";
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
        std::cerr << randomColor() << "Error copying from " << source << ": " << e.what() << RESET << std::endl;
    }
}

int main() {
    srand(static_cast<unsigned>(time(0))); // Seed random colors
    SetConsoleTitleA("Maxon Cinema 4D 2025 Plugin Installer");

    std::string baseMaxonPath = findMaxonFolder();
    if (baseMaxonPath.empty()) {
        std::cerr << randomColor() << "No valid Maxon Cinema 4D folder found!" << RESET << std::endl;
        return 1;
    }

    fs::path pluginsPath = fs::path(baseMaxonPath) / "plugins";
    fs::path libraryPath = fs::path(baseMaxonPath) / "library";
    fs::path currentPath = fs::current_path();

    fs::path libPath = currentPath / "lib";
    fs::path extrasPath = currentPath / "Extras";
    fs::path coreSourcePath = currentPath / "Core";

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

    // Count total files from Core folder
    if (fs::exists(coreSourcePath) && fs::is_directory(coreSourcePath)) {
        totalFiles += countTotalFiles(coreSourcePath);
    }

    if (totalFiles == 0) {
        std::cerr << randomColor() << "No files to copy!" << RESET << std::endl;
        return 1;
    }

    std::cout << randomColor() << "Starting copy with total files: " << totalFiles << RESET << std::endl;

    // Copy plugins from lib to plugins folder
    if (fs::exists(libPath) && fs::is_directory(libPath)) {
        for (const auto& entry : fs::directory_iterator(libPath)) {
            if (entry.is_directory()) {
                std::cout << randomColor() << "Copying plugin: " << entry.path().filename().string() << RESET << std::endl;
                copyFolderWithProgress(entry.path(), pluginsPath / entry.path().filename(), copiedFiles, totalFiles);
            }
        }
    }
    else {
        std::cerr << randomColor() << "'lib' folder not found!" << RESET << std::endl;
    }

    // Copy Extras to Library
    if (fs::exists(extrasPath) && fs::is_directory(extrasPath)) {
        std::cout << randomColor() << "Copying Extras to Library..." << RESET << std::endl;
        copyFolderWithProgress(extrasPath, libraryPath, copiedFiles, totalFiles);
    }
    else {
        std::cerr << randomColor() << "'Extras' folder not found!" << RESET << std::endl;
    }

    // Copy Core to C:\Program Files\Cinema 4D 2025\plugins
    fs::path programPluginsPath = "C:\\Program Files\\Maxon Cinema 4D 2025\\plugins";
    if (!fs::exists(programPluginsPath)) {
        fs::create_directories(programPluginsPath);
    }

    if (fs::exists(coreSourcePath) && fs::is_directory(coreSourcePath)) {
        std::cout << randomColor() << "Copying Core files to Program Files plugin folder..." << RESET << std::endl;
        try {
            for (const auto& entry : fs::recursive_directory_iterator(coreSourcePath)) {
                if (fs::is_regular_file(entry)) {
                    fs::path destFile = programPluginsPath / fs::relative(entry.path(), coreSourcePath);
                    fs::create_directories(destFile.parent_path());
                    fs::copy_file(entry.path(), destFile, fs::copy_options::overwrite_existing);
                    copiedFiles++;
                    showProgressBar(static_cast<float>(copiedFiles) / totalFiles);
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << randomColor() << "Error copying Core files: " << e.what() << RESET << std::endl;
        }
    }
    else {
        std::cerr << randomColor() << "'Core' folder not found!" << RESET << std::endl;
    }

    std::cout << std::endl << randomColor() << "Plugin Installed Successfully!" << RESET << std::endl;
    std::cout << randomColor() << "Developed By - !EAGLE" << RESET << std::endl;

    system("pause");
}
