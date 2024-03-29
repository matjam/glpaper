#include <algorithm>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <random>
#include <string>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <spdlog/spdlog.h>

#include "config.hpp"
#include "getwindow.hpp"
#include "scaling.hpp"

int main()
{
  std::string current_wallpaper;
  std::string next_wallpaper;
  sf::Image current_wallpaper_image;
  sf::Image next_wallpaper_image;
  sf::Texture current_wallpaper_texture;
  sf::Texture next_wallpaper_texture;
  sf::Sprite current_wallpaper_sprite;
  sf::Sprite next_wallpaper_sprite;

  spdlog::info("starting glpaper...");

  Config config;

  if (config.get_debug()) {
    spdlog::set_level(spdlog::level::debug);
  } else {
    spdlog::set_level(spdlog::level::info);
  }

  // make sure we can get the root window
  auto render_window = getRenderWindow();
  render_window->setFramerateLimit(config.get_framerate_limit());
  if (!render_window) {
    spdlog::error("Error: couldn't get root window");
    return EXIT_FAILURE;
  }

  auto wallpapers = config.get_wallpapers();
  if (wallpapers.empty()) {
    spdlog::error("Error: no wallpapers found");
    return EXIT_FAILURE;
  }

  // take the first wallpaper in the deque and set it as the next wallpaper, then put
  // it at the back of the deque.

  next_wallpaper = wallpapers.front();
  wallpapers.pop_front();
  wallpapers.push_back(next_wallpaper);

  spdlog::info("next wallpaper: {}", next_wallpaper);

  // load the next wallpaper into an image and set it as the next wallpaper sprite. We
  // use an image because we need to swap the wallpaper textures later by loading the
  // next wallpaper into a texture and setting it as the next wallpaper sprite.
  if (!next_wallpaper_image.loadFromFile(next_wallpaper)) {
    spdlog::error("Error: couldn't load next wallpaper");
    return EXIT_FAILURE;
  }
  next_wallpaper_texture.loadFromImage(next_wallpaper_image);
  next_wallpaper_sprite.setTexture(next_wallpaper_texture, true);
  spdlog::debug("scaling first wallpaper sprite to fit render window");
  scale(render_window, &next_wallpaper_sprite, config.get_scale_mode());

  // when we initially start, we want to fade in the next wallpaper, so we set the alpha
  // to 0.
  float alpha = 0;
  next_wallpaper_sprite.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));

  // time_until_next_wallpaper is the time until the next wallpaper is displayed. We set
  // it to the time between wallpapers initially.
  sf::Time time_until_next_wallpaper = sf::seconds(config.get_delay_seconds());

  // clock allows us to track the time until the next wallpaper is displayed.
  sf::Clock clock;

  bool fading_in = true;
  bool IsRunning = true;
  while (IsRunning) {
    // Clear the view
    render_window->clear(sf::Color::Black);

    // if the current_wallpaper_sprite exists then we don't draw it. Otherwise, we always draw it.
    if (current_wallpaper_sprite.getTexture()) {
      render_window->draw(current_wallpaper_sprite);
    }

    // if we are fading in the next wallpaper, we draw it. Otherwise, we don't.
    if (fading_in) {
      render_window->draw(next_wallpaper_sprite);
      alpha += config.get_fade_speed();

      // if the alpha is 255, we are done fading in the next wallpaper. We set the time until
      // the next wallpaper to the time between wallpapers. We swap in the next wallpaper as
      // the current wallpaper.
      if (alpha >= 255.0f) {
        spdlog::info("finished fading in wallpaper {}", next_wallpaper);

        fading_in = false;
        alpha = 0.0;

        // the image that we loaded for the next wallpaper is now the current wallpaper, so we
        // load the next wallpaper image into the current wallpaper texture and set it as the
        // current wallpaper sprite.

        current_wallpaper_texture.loadFromImage(next_wallpaper_image);
        current_wallpaper_sprite.setTexture(current_wallpaper_texture, true);  // should not be necessary
        spdlog::debug("scaling current wallpaper sprite to fit render window");
        scale(render_window, &current_wallpaper_sprite, config.get_scale_mode());
        current_wallpaper_sprite.setColor(sf::Color(255, 255, 255, 255));  // fully opaque
        next_wallpaper_sprite.setColor(sf::Color(255, 255, 255, 0));  // fully transparent

        time_until_next_wallpaper = sf::seconds(config.get_delay_seconds());

        spdlog::info("next wallpaper: {}", next_wallpaper);
        spdlog::info("delaying {} seconds", time_until_next_wallpaper.asSeconds());
      }
    } else {
      // if we are not fading in a wallpaper, we wait until the time until the next wallpaper
      // is 0, then we fade in the next wallpaper.
      if (time_until_next_wallpaper.asSeconds() <= 0) {
        time_until_next_wallpaper = sf::seconds(config.get_delay_seconds());

        if (config.has_changed()) {
          spdlog::info("config file has changed, reloading");
          config.reload();
          render_window->setFramerateLimit(config.get_framerate_limit());
          if (config.get_debug()) {
            spdlog::set_level(spdlog::level::debug);
          } else {
            spdlog::set_level(spdlog::level::info);
          }
        }

        // we now set the current wallpaper to the nextwallpaper, and the next wallpaper to
        // the next wallpaper in the queue.

        next_wallpaper = wallpapers.front();
        wallpapers.pop_front();
        wallpapers.push_back(next_wallpaper);

        spdlog::info("fading in new wallpaper {}", next_wallpaper);
        fading_in = true;
        alpha = 0.0;

        // load the next wallpaper into a texture and set it as the next wallpaper sprite
        if (!next_wallpaper_image.loadFromFile(next_wallpaper)) {
          spdlog::error("Error: couldn't load next wallpaper");
          return EXIT_FAILURE;
        }
        next_wallpaper_texture.loadFromImage(next_wallpaper_image);
        next_wallpaper_sprite.setTexture(next_wallpaper_texture, true);
        spdlog::debug("scaling next wallpaper sprite to fit render window");
        scale(render_window, &next_wallpaper_sprite, config.get_scale_mode());

      } else {
        time_until_next_wallpaper -= clock.restart();
      }
    }

    // if the next wallpaper sprite has a nonzero alpha, we should render it.
    if (alpha > 0.0f) {
      if (alpha > 255.0f) {
        alpha = 255.0f;
      }

      next_wallpaper_sprite.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));
      render_window->draw(next_wallpaper_sprite);
    }

    if (!fading_in && time_until_next_wallpaper.asSeconds() > 0) {
      spdlog::debug("waiting {} seconds until next wallpaper", time_until_next_wallpaper.asSeconds());
      sf::sleep(sf::seconds(time_until_next_wallpaper.asSeconds()));
    }

    // Display the view on screen
    render_window->display();
  }

  return EXIT_SUCCESS;
}
