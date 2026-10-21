#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "App/App.h"
#include "DemoStatus.h"

namespace {
constexpr int width = 294, height = 126;
volatile sig_atomic_t stop = 0;
struct Frame {
    std::array<std::uint32_t, width * height> pixels{};
    bool dirty = false;
};
struct Pointer {
    lv_point_t point{};
    lv_indev_state_t state = LV_INDEV_STATE_RELEASED;
};
void pointer_read(lv_indev_drv_t* driver, lv_indev_data_t* data) {
    const auto* pointer = static_cast<const Pointer*>(driver->user_data);
    data->point = pointer->point;
    data->state = pointer->state;
}
void on_signal(int) {
    stop = 1;
}
void flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* colors) {
    auto* frame = static_cast<Frame*>(driver->user_data);
    for (int y = area->y1; y <= area->y2; ++y)
        for (int x = area->x1; x <= area->x2; ++x)
            frame->pixels[y * width + x] = 0xff000000u | (lv_color_to32(*colors++) & 0xffffffu);
    frame->dirty = true;
    lv_disp_flush_ready(driver);
}
} // namespace
int main(int argc, char** argv) {
    const bool random_status = argc >= 2 && std::strcmp(argv[1], "--random-status") == 0;
    std::uint32_t seed = 1;
    bool valid_args = argc == 1 || (random_status && argc <= 3);
    if (valid_args && argc == 3) {
        char* end = nullptr;
        errno = 0;
        const auto value = std::strtoul(argv[2], &end, 10);
        valid_args = !errno && argv[2][0] >= '0' && argv[2][0] <= '9' && !*end && value <= UINT32_MAX;
        if (valid_args)
            seed = static_cast<std::uint32_t>(value);
    }
    if (!valid_args) {
        std::fprintf(stderr, "Usage: %s [--random-status [SEED_0_TO_4294967295]]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (std::signal(SIGINT, on_signal) == SIG_ERR || std::signal(SIGTERM, on_signal) == SIG_ERR) {
        std::perror("signal");
        return EXIT_FAILURE;
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL init: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Window* window =
        SDL_CreateWindow("App preview", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE) : nullptr;
    SDL_Texture* texture =
        renderer ? SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height)
                 : nullptr;
    int result = EXIT_FAILURE;
    if (texture) {
        lv_init();
        Frame frame;
        lv_color_t pixels[width * 20];
        lv_disp_draw_buf_t draw;
        lv_disp_draw_buf_init(&draw, pixels, nullptr, width * 20);
        lv_disp_drv_t driver;
        lv_disp_drv_init(&driver);
        driver.hor_res = width;
        driver.ver_res = height;
        driver.draw_buf = &draw;
        driver.flush_cb = flush;
        driver.user_data = &frame;
        auto* display = lv_disp_drv_register(&driver);
        if (display) {
            Pointer pointer;
            lv_indev_drv_t pointer_driver;
            lv_indev_drv_init(&pointer_driver);
            pointer_driver.type = LV_INDEV_TYPE_POINTER;
            pointer_driver.read_cb = pointer_read;
            pointer_driver.user_data = &pointer;
            auto* indev = lv_indev_drv_register(&pointer_driver);
            if (indev) {
                App app;
                if (app.init()) {
                    std::puts("Host controls: arrows/wheel = focus (+1 in editor), Enter/middle click = press/release, "
                              "hold 2s to Start, Esc/right click = back/cancel, Ctrl+Enter = commit, Q = quit. "
                              "Power/time are simulated.");
                    if (random_status)
                        std::printf("Random DEMO status: seed=%u, new battery/charging/Wi-Fi/recording/satellites "
                                    "every 3s; publication every 200ms.\n",
                                    seed);
                    bool enter_down = false;
                    const auto started = SDL_GetTicks64();
                    auto last = started;
                    auto last_status = started;
                    app.update_status(random_status ? DemoStatus::sample_random(0, seed) : DemoStatus::sample(0));
                    result = EXIT_SUCCESS;
                    while (!stop) {
                        SDL_Event event;
                        while (SDL_PollEvent(&event)) {
                            if (event.type == SDL_QUIT)
                                stop = 1;
                            else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                                const auto key = event.key.keysym.sym;
                                if (key == SDLK_q)
                                    stop = 1;
                                else if (key == SDLK_RIGHT || key == SDLK_DOWN)
                                    app.on_input(App::InputAction::NextFocus);
                                else if (key == SDLK_LEFT || key == SDLK_UP)
                                    app.on_input(App::InputAction::PreviousFocus);
                                else if (key == SDLK_ESCAPE)
                                    app.on_input(App::InputAction::Back);
                                else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                                    if (event.key.keysym.mod & KMOD_CTRL)
                                        app.on_input(App::InputAction::Commit);
                                    else if (!enter_down) {
                                        enter_down = true;
                                        app.on_input(App::InputAction::Press);
                                    }
                                }
                            } else if (event.type == SDL_KEYUP
                                       && (event.key.keysym.sym == SDLK_RETURN
                                           || event.key.keysym.sym == SDLK_KP_ENTER)) {
                                if (enter_down) {
                                    enter_down = false;
                                    app.on_input(App::InputAction::Release);
                                }
                            } else if (event.type == SDL_MOUSEWHEEL && event.wheel.y != 0) {
                                app.on_input(event.wheel.y > 0 ? App::InputAction::PreviousFocus
                                                               : App::InputAction::NextFocus);
                            } else if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN
                                       || event.type == SDL_MOUSEBUTTONUP) {
                                const int x = event.type == SDL_MOUSEMOTION ? event.motion.x : event.button.x;
                                const int y = event.type == SDL_MOUSEMOTION ? event.motion.y : event.button.y;
                                pointer.point = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y)};
                                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
                                    app.on_input(App::InputAction::Back);
                                else if (event.type != SDL_MOUSEMOTION && event.button.button == SDL_BUTTON_MIDDLE)
                                    app.on_input(event.type == SDL_MOUSEBUTTONDOWN ? App::InputAction::Press
                                                                                   : App::InputAction::Release);
                                else if (event.type != SDL_MOUSEMOTION && event.button.button == SDL_BUTTON_LEFT) {
                                    pointer.state = event.type == SDL_MOUSEBUTTONDOWN ? LV_INDEV_STATE_PRESSED
                                                                                      : LV_INDEV_STATE_RELEASED;
                                    lv_timer_ready(indev->driver->read_timer);
                                }
                            } else if (event.type == SDL_WINDOWEVENT
                                       && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                                enter_down = false;
                                app.cancel_input();
                                pointer.state = LV_INDEV_STATE_RELEASED;
                                lv_indev_reset(indev, nullptr);
                            }
                        }
                        const auto now = SDL_GetTicks64();
                        lv_tick_inc(static_cast<std::uint32_t>(now - last));
                        last = now;
                        app.update();
                        lv_timer_handler();
                        if (now - last_status >= 200) {
                            app.update_status(random_status ? DemoStatus::sample_random(now - started, seed)
                                                            : DemoStatus::sample(now - started));
                            last_status = now;
                        }
                        SDL_SetWindowTitle(window, app.current_page());
                        if (frame.dirty) {
                            if (SDL_UpdateTexture(texture, nullptr, frame.pixels.data(), width * 4) != 0
                                || SDL_RenderClear(renderer) != 0
                                || SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0) {
                                std::fprintf(stderr, "SDL render: %s\n", SDL_GetError());
                                result = EXIT_FAILURE;
                                break;
                            }
                            SDL_RenderPresent(renderer);
                            frame.dirty = false;
                        }
                        SDL_Delay(5);
                    }
                } else
                    std::fprintf(stderr, "App init failed\n");
            }
            if (indev)
                lv_indev_delete(indev);
            else
                std::fprintf(stderr, "LVGL pointer init failed\n");
            lv_disp_remove(display);
        }
    } else
        std::fprintf(stderr, "SDL window: %s\n", SDL_GetError());
    if (texture)
        SDL_DestroyTexture(texture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
