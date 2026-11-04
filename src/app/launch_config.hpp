#pragma once
#include <cstdint>
#include <string>
#include <tomlplusplus/toml.hpp>

#include "core/utilities.hpp"
#include "utils/filesystem.hpp"

namespace hammock::app {
    /// @struct LaunchConfig
    struct LaunchConfig {
        struct {
            std::string displayName{ "Default Application Name" };
            std::uint32_t resolutionWidth{ 800 };
            std::uint32_t resolutionHeight{ 600 };
            std::string version{ "0.0.0" };
        } application;

        struct {
        } assets;
        struct {
            std::string fontFile{ "../../../data/fonts/GoogleSansCode-Medium.ttf" };
            float resolutionScale{ 1.0f };
        } rendering;
    };

    [[nodiscard]] inline LaunchConfig parseLaunchConfig(const std::filesystem::path& path) {
        if (!filesystem::directoryExists(path)) {
            core::Logger::log(
                core::LOG_LEVEL_ERROR, "Launch config directory does not exist: %s\n", path.string().c_str());
            throw std::runtime_error("Launch config directory does not exist: " + path.string());
        }

        if (!filesystem::fileExists(path / "hammock.toml")) {
            core::Logger::log(core::LOG_LEVEL_ERROR,
                "Launch config file does not exist: %s\n",
                (path / "hammock.toml").string().c_str());
            throw std::runtime_error(
                "Launch config file does not exist: " + (path / "hammock.toml").string());
        }

        core::Logger::log(
            core::LOG_LEVEL_INFO, "Parsing launch config from: %s", (path / "hammock.toml").string().c_str());

        LaunchConfig config;
        toml::table data = toml::parse_file((path / "hammock.toml").string<char>());
        // Display name
        config.application.displayName =
            data["application"]["display_name"].value_or(config.application.displayName);

        // Resolution
        config.application.resolutionWidth =
            data["application"]["window_width"].value_or(config.application.resolutionWidth);
        if (config.application.resolutionWidth <= 0) {
            core::Logger::log(core::LOG_LEVEL_ERROR,
                "Invalid resolution width in launch config: %d\n",
                config.application.resolutionWidth);
            throw std::runtime_error("Invalid resolution width in launch config: " +
                                     std::to_string(config.application.resolutionWidth));
        }
        config.application.resolutionHeight =
            data["application"]["window_height"].value_or(config.application.resolutionHeight);
        if (config.application.resolutionHeight <= 0) {
            core::Logger::log(core::LOG_LEVEL_ERROR,
                "Invalid resolution height in launch config: %d\n",
                config.application.resolutionHeight);
            throw std::runtime_error("Invalid resolution height in launch config: " +
                                     std::to_string(config.application.resolutionHeight));
        }

        // Version
        config.application.version = data["application"]["version"].value_or(config.application.version);
        if (config.application.version.empty()) {
            core::Logger::log(
                core::LOG_LEVEL_WARN, "Application version is not specified in launch config", "");
        }

        // Resolution scale
        config.rendering.resolutionScale =
            data["rendering"]["resolution_scale"].value_or(config.rendering.resolutionScale);

        // Font
        config.rendering.fontFile = data["rendering"]["font_file"].value_or("");
        if (config.rendering.fontFile.empty()) {
            core::Logger::log(
                core::LOG_LEVEL_WARN, "Font file is not specified in launch config, using default one", "");
        } else if (!filesystem::fileExists(path / config.rendering.fontFile)) {
            core::Logger::log(core::LOG_LEVEL_ERROR,
                "Font file does not exist: %s\n",
                (path / config.rendering.fontFile).string().c_str());
            throw std::runtime_error(
                "Font file does not exist: " + (path / config.rendering.fontFile).string());
        }
        config.rendering.fontFile = (path / config.rendering.fontFile).string();

        return config;
    }
}  // namespace hammock::app