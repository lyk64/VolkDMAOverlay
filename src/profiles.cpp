#include "include/VolkDMAOverlay/profiles.hh"
#include <VolkLog/log.hh>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <utility>

static constexpr volk::log::Logger logger{ "PROFILES" };

volk::config::ProfileStore::ProfileStore(std::filesystem::path directory, std::string extension)
    : directory{ std::move(directory) }, extension{ std::move(extension) } {}

std::vector<std::string> volk::config::ProfileStore::list() const {
    std::vector<std::string> names;

    try {
        std::filesystem::create_directories(directory);

        for (const auto& entry : std::filesystem::directory_iterator(directory))
            if (entry.is_regular_file() && entry.path().extension() == extension)
                names.push_back(entry.path().stem().string());
    }
    catch (const std::filesystem::filesystem_error& e) {
        logger.warn("couldn't list profiles in {}: {}", directory.string(), e.what());
    }

    std::ranges::sort(names);
    return names;
}

std::filesystem::path volk::config::ProfileStore::path_for(std::string_view name) const {
    auto file = std::filesystem::path{ name }.filename();
    if (file.stem().empty())
        return {};

    file.replace_extension(extension);
    return directory / file;
}

std::optional<std::string> volk::config::ProfileStore::read_file(std::string_view name) const {
    const auto path = path_for(name);
    if (path.empty()) {
        logger.warn("rejected profile name: '{}'", name);
        return std::nullopt;
    }

    std::ifstream in{ path, std::ios::binary };
    if (!in) {
        logger.warn("couldn't read profile: {}", path.string());
        return std::nullopt;
    }

    return std::string{ std::istreambuf_iterator<char>{ in }, std::istreambuf_iterator<char>{} };
}

bool volk::config::ProfileStore::write_file(std::string_view name, std::string_view text) const {
    const auto path = path_for(name);
    if (path.empty()) {
        logger.warn("rejected profile name: '{}'", name);
        return false;
    }

    try {
        std::filesystem::create_directories(directory);
    }
    catch (const std::filesystem::filesystem_error& e) {
        logger.warn("couldn't create profile folder {}: {}", directory.string(), e.what());
        return false;
    }

    std::ofstream out{ path, std::ios::binary };
    if (!out) {
        logger.warn("couldn't write profile: {}", path.string());
        return false;
    }

    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    return out.good();
}
