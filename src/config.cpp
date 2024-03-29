#include <algorithm>
#include <filesystem>
#include <list>
#include <random>
#include <string>

#include "config.hpp"

#include <spdlog/spdlog.h>
#include <toml.hpp>

std::string replace_tilde_with_home(const std::string& path)
{
  if (path[0] == '~') {
    return std::string(std::getenv("HOME")) + path.substr(1);
  }
  return path;
}

Config::Config()
{
  reload();
}

Config::~Config() {}

void Config::reload()
{
  std::string xdg_config_home;

  if (std::getenv("XDG_CONFIG_HOME") == nullptr) {
    xdg_config_home = std::string(std::getenv("HOME")) + "/.config";
  } else {
    xdg_config_home = std::getenv("XDG_CONFIG_HOME");
  }

  std::list<std::string> search_paths = {
      xdg_config_home.append("/glpaper/glpaper.toml"),
      xdg_config_home.append("/glpaper.toml"),
      "~/.config/glpaper/glpaper.toml",
      "~/.config/glpaper.toml",
      "/etc/xdg/glpaper.toml",
  };

  for (const auto& path : search_paths) {
    auto fixed_path = replace_tilde_with_home(path);

    if (std::filesystem::exists(fixed_path)) {
      spdlog::info("loading config from {}", fixed_path);
      _config = toml::parse(fixed_path);
      _last_write_time = std::filesystem::last_write_time(fixed_path);
      _loaded_config_path = fixed_path;

      cache();
      return;
    }
  }

  spdlog::info("no config file found, using defaults");
}

bool Config::has_changed()
{
  // check if the config file has changed since we last loaded it
  auto last_write_time = std::filesystem::last_write_time(_loaded_config_path);
  return last_write_time != _last_write_time;
}

void Config::cache()
{
  // cache often used values from the config file

  _debug = toml::find_or<bool>(_config, "debug", false);

  if (_config.contains("fade_speed")) {
    if (_config.at("fade_speed").is_floating()) {
      _fade_speed = toml::find<float>(_config, "fade_speed");
    } else if (_config.at("fade_speed").is_integer()) {
      _fade_speed = static_cast<float>(toml::find<int>(_config, "fade_speed"));
    } else {
      _fade_speed = 1.0f;
      spdlog::warn("invalid fade speed: {}", _fade_speed);
    }
  } else {
    _fade_speed = 1.0f;
  }

  _framerate_limit = toml::find_or<unsigned int>(_config, "framerate_limit", 60);

  if ((_framerate_limit < 1) || (_framerate_limit > 240)) {
    spdlog::warn("invalid framerate limit: {}", _framerate_limit);
    _framerate_limit = 60;
  }

  _delay_seconds = static_cast<float>(toml::find_or<int>(_config, "delay", 60));

  if (_delay_seconds < 1) {
    spdlog::warn("invalid delay: {}", _delay_seconds);
  }

  if (_delay_seconds > 3600 * 24) {
    spdlog::warn("delay is pretty long, you sure you want {} seconds?", _delay_seconds);
  }

  auto scale_mode = toml::find_or<std::string>(_config, "scale_mode", "vertical");

  if (scale_mode == "horizontal") {
    _scale_mode = WallpaperScaleMode::HorizontalFit;
  } else if (scale_mode == "vertical") {
    _scale_mode = WallpaperScaleMode::VerticalFit;
  } else if (scale_mode == "stretched") {
    _scale_mode = WallpaperScaleMode::StretchedFit;
  } else {
    spdlog::warn("invalid scale mode: {}", scale_mode);
    _scale_mode = WallpaperScaleMode::VerticalFit;
  }
}

std::string Config::get_wallpapers_path()
{
  return replace_tilde_with_home(toml::find_or<std::string>(_config, "wallpapers", "~/Downloads/wp"));
}

bool Config::get_shuffle_wallpapers()
{
  return toml::find_or<bool>(_config, "shuffle", true);
}

std::deque<std::string> Config::get_wallpapers()
{
  std::deque<std::string> wallpapers;

  // iterate over the directory and push the file paths into the vector if the file
  // is a png or jpg.

  std::filesystem::path p(get_wallpapers_path());

  if (!std::filesystem::exists(p)) {
    spdlog::critical("wallpapers path does not exist: {}", p.string());
    return wallpapers;
  }

  spdlog::debug("wallpapers path: {}", p.string());

  for (const auto& entry : std::filesystem::directory_iterator {p}) {
    if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
      wallpapers.push_back(entry.path().string());
    }
  }

  if (get_shuffle_wallpapers()) {
    // shuffle the vector so that the wallpapers are displayed in a random order.
    std::shuffle(wallpapers.begin(), wallpapers.end(), std::mt19937(std::random_device()()));
  }

  // log the wallpapers we found to debug log
  for (const auto& wallpaper : wallpapers) {
    spdlog::debug("wallpaper: {}", wallpaper);
  }

  return wallpapers;
}

WallpaperScaleMode Config::get_scale_mode()
{
  return _scale_mode;
}

float Config::get_fade_speed()
{
  return _fade_speed;
}

unsigned int Config::get_framerate_limit()
{
  return _framerate_limit;
}

float Config::get_delay_seconds()
{
  return _delay_seconds;
}

bool Config::get_debug()
{
  return _debug;
}