#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

static void replace_with_link(const fs::path &original, const fs::path &duplicate)
{
    fs::path temporary = duplicate;
    temporary += ".hardlink";
    fs::create_hard_link(original, temporary);
    try {
        fs::rename(temporary, duplicate);
    } catch (...) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        throw;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " directory\n";
        return 1;
    }

    try {
        std::vector<fs::path> files;
        for (const auto &entry : fs::recursive_directory_iterator(argv[1])) {
            if (fs::is_regular_file(entry.symlink_status())) {
                files.push_back(entry.path());
            }
        }

        std::unordered_map<std::string, fs::path> originals;
        for (const auto &path : files) {
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Cannot open " + path.string());
            }
            file.exceptions(std::ios::badbit);
            std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            auto result = originals.emplace(contents, path);
            if (result.second) {
                continue;
            }
            const auto &original = result.first->second;
            if (!fs::equivalent(original, path)) {
                replace_with_link(original, path);
                std::cout << path << " => " << original << '\n';
            }
        }
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
