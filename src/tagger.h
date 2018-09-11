#pragma once

#include <string>
#include <vector>

#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
#else
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#endif

// use "const std::string&" rather than "std::string_view" beacuse of "std::vector<std::string>"
bool write_tag(const std::string& album,
               const std::string& title,
               const std::vector<std::string>& artists,
               const void* img_data,
               std::size_t img_len,
               const std::string& img_mime,
               const fs::path& audio_path);
