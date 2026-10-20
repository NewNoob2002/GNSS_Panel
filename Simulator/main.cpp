// HC32 带屏产品 PC 模拟器：共享 LVGL 应用（Simulator/App）+ HC32 平台层（Shared/Display/AppPlatform）
// + 模拟 HAL（Simulator/HAL，运行真实的关机策略 PowerPolicy）。窗口 294×126，1:1 像素。
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <array>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AppPlatform.h"
#include "HAL/HAL.h"
#include "P4App.h"
#include "mcu_version_config.h"

SystemInfo_t systemInfo;

namespace {
constexpr int kWidth = 294;
constexpr int kHeight = 126;
volatile std::sig_atomic_t stop = 0;

struct Frame {
    std::array<std::uint32_t, kWidth * kHeight> pixels{};
    bool dirty = false;
};

struct Pointer {
    lv_point_t point{};
    lv_indev_state_t state = LV_INDEV_STATE_RELEASED;
};

void
on_signal(int) {
    stop = 1;
}

void
flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* colors) {
    auto* frame = static_cast<Frame*>(driver->user_data);
    for (int y = area->y1; y <= area->y2; ++y) {
        for (int x = area->x1; x <= area->x2; ++x) {
            frame->pixels[y * kWidth + x] = 0xff000000u | (lv_color_to32(*colors++) & 0xffffffu);
        }
    }
    frame->dirty = true;
    lv_disp_flush_ready(driver);
}

void
pointer_read(lv_indev_drv_t* driver, lv_indev_data_t* data) {
    const auto* pointer = static_cast<const Pointer*>(driver->user_data);
    data->point = pointer->point;
    data->state = pointer->state;
}

// 主板版本：V1.5 带 MP2762A，V1.3 没有（GPIO 控制充电）；与 HAL::I2C_Scan 的推断一致。
void
set_board(bool v15) {
    systemInfo.online_device.mp2762 = v15;
    std::snprintf(systemInfo.hardware_version, sizeof(systemInfo.hardware_version), "%s", v15 ? "V1.5" : "V1.3");
}

// 模拟主控已上报的数据：模拟器中的初始状态。
void
init_system_info() {
    std::memset(&systemInfo, 0, sizeof(systemInfo));
    std::snprintf(systemInfo.software_version, sizeof(systemInfo.software_version), "%s", SOFTWARE_VERSION);
    set_board(true);
    systemInfo.online_device.bq40z50 = true;
    auto& battery = systemInfo.powerMonitor.batteryInfo;
    battery.sample_valid = true;
    battery.Percent = 76;
    battery.Voltage_f = 7.92f;
    battery.Temp_f = 32.0f;
    battery.Percent_f = 76.0f;
    battery.chargeStatus = notCharge;
    systemInfo.powerMonitor.ExternalPower = 1;
    auto& position = systemInfo.positionInfo;
    position.coordinate_status = position_fix;
    position.satellite_number_used = 18;
    position.satellite_number_track = 26;
    position.coordinate_lat = 31.230416;
    position.coordinate_lon = 121.473701;
    position.coordinate_alt = 12.5;
    systemInfo.work_mode = rover_mode;
    systemInfo.radioInfo = {radio_mode_rx, On_Off_Status_ON, 1, 3, 0}; // TRIMTALK，信道 3
    auto& record = systemInfo.recordInfo;
    record.record_type = 1;
    record.record_interval = 0x00;
    record.record_leftspace = 2048.0f;
    std::memcpy(record.record_name, "SIM0001", 7);
    auto& wifi = systemInfo.wifiInfo;
    wifi.wifi_status = wifi.wifi_on_off_set = On_Off_Status_ON;
    std::snprintf(wifi.wifi_ssid, sizeof(wifi.wifi_ssid), "%s-SIM", PRODUCT_NAME);
    wifi.wifi_ip[0] = 192;
    wifi.wifi_ip[1] = 168;
    wifi.wifi_ip[2] = 10;
    wifi.wifi_ip[3] = 1;
    systemInfo.starMapInfo.numberGPS = 8;
    systemInfo.starMapInfo.numberBDS = 14;
    systemInfo.starMapInfo.numberGLONASS = 6;
    systemInfo.starMapInfo.numberGALILEO = 4;
    systemInfo.starMapInfo.numberSBAS = 2;
    systemInfo.starMapInfo.numberQZSS = 3;
    systemInfo.starMapInfo.numberIRNSS = 1;
}

void
print_controls() {
    std::puts("Controls: arrows/wheel = focus, Enter/middle click = press/release (hold 2s on Startup),\n"
              "          Esc/right click = back, Ctrl+Enter = commit, left click = pointer, Q = quit.\n"
              "HC32 events: H = host shutdown request, L = low-battery shutdown, F = forced shutdown,\n"
              "             T/G = battery temperature +5/-5 C, C = toggle charging, B/N = battery -10/+10%,\n"
              "             V = toggle board V1.5/V1.3 (MP2762A present or not; checked at the next power-on).");
}

// 模拟主控：取走面板下发的设定（*_change_flag），1 s 后应用 Wi-Fi；每秒刷新一次星图数据。
void
simulate_host(uint32_t now) {
    static uint32_t wifi_since = 0;
    static uint32_t last_stars = 0;
    auto& wifi = systemInfo.wifiInfo;
    if (wifi.wifi_change_flag) {
        wifi.wifi_change_flag = 0;
        wifi_since = now;
        std::printf("[sim] host: wifi set %s\n", wifi.wifi_on_off_set ? "on" : "off");
    }
    if (wifi.wifi_status != wifi.wifi_on_off_set && now - wifi_since >= 1000) {
        wifi.wifi_status = wifi.wifi_on_off_set;
    }
    auto& radio = systemInfo.radioInfo;
    if (radio.radio_change_flag) {
        radio.radio_change_flag = 0;
        std::printf("[sim] host: work mode %d, radio %s, protocol %u, channel %u, ntrip server %d\n",
                    systemInfo.work_mode, radio.radio_status ? "on" : "off", radio.radio_protocol, radio.radio_channel,
                    systemInfo.ntripInfo.NtripServer_status);
    }
    auto& record = systemInfo.recordInfo;
    if (record.record_change_flag) {
        record.record_change_flag = 0;
        record.record_op = 0;
        std::printf("[sim] host: record %s, type %u, interval 0x%02x\n", record.record_status ? "start" : "stop",
                    record.record_type, record.record_interval);
    }
    if (now - last_stars >= 1000) {
        last_stars = now;
        systemInfo.starMapInfo.syncStatus = true;
    }
}

// 模拟 HAL/主控事件，便于检查平台驱动的流程。
void
inject_event(SDL_Keycode key) {
    auto& power = systemInfo.powerMonitor;
    auto& battery = power.batteryInfo;
    switch (key) {
        case SDLK_h: power.LinuxPowerOff = true; break;
        case SDLK_l: power.LowBatteryPowerOff = true; break;
        case SDLK_f: power.Force_ShutDown = true; break;
        case SDLK_t: battery.Temp_f += 5.0f; break;
        case SDLK_g: battery.Temp_f -= 5.0f; break;
        case SDLK_c: battery.chargeStatus = battery.chargeStatus == notCharge ? normalCharge : notCharge; break;
        case SDLK_b: battery.Percent = battery.Percent >= 10 ? battery.Percent - 10 : 0; break;
        case SDLK_n: battery.Percent = battery.Percent <= 90 ? battery.Percent + 10 : 100; break;
        case SDLK_v:
            set_board(!systemInfo.online_device.mp2762);
            std::printf("[sim] board %s\n", systemInfo.hardware_version);
            return;
        default: return;
    }
    battery.Percent_f = battery.Percent; // SystemInfos 显示浮点电量
    std::printf("[sim] temperature %.1f C, battery %u%%, charging %d, shutdown flags host=%d low=%d forced=%d\n",
                static_cast<double>(battery.Temp_f), battery.Percent, battery.chargeStatus != notCharge,
                power.LinuxPowerOff, power.LowBatteryPowerOff, power.Force_ShutDown);
}
} // namespace

int
main() {
    if (std::signal(SIGINT, on_signal) == SIG_ERR || std::signal(SIGTERM, on_signal) == SIG_ERR) {
        std::perror("signal");
        return EXIT_FAILURE;
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL init: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_Window* window = SDL_CreateWindow("HC32 " PRODUCT_NAME " simulator", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, kWidth, kHeight, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE) : nullptr;
    SDL_Texture* texture =
        renderer ? SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, kWidth, kHeight)
                 : nullptr;
    int result = EXIT_FAILURE;
    if (!texture) {
        std::fprintf(stderr, "SDL window: %s\n", SDL_GetError());
    } else {
        lv_init();
        static Frame frame;
        static lv_color_t buffer[kWidth * 20];
        static lv_disp_draw_buf_t draw;
        lv_disp_draw_buf_init(&draw, buffer, nullptr, kWidth * 20);
        static lv_disp_drv_t driver;
        lv_disp_drv_init(&driver);
        driver.hor_res = kWidth;
        driver.ver_res = kHeight;
        driver.draw_buf = &draw;
        driver.flush_cb = flush;
        driver.user_data = &frame;
        lv_disp_drv_register(&driver);

        static Pointer pointer;
        static lv_indev_drv_t pointer_driver;
        lv_indev_drv_init(&pointer_driver);
        pointer_driver.type = LV_INDEV_TYPE_POINTER;
        pointer_driver.read_cb = pointer_read;
        pointer_driver.user_data = &pointer;
        lv_indev_t* indev = lv_indev_drv_register(&pointer_driver);

        init_system_info();
        HAL::HAL_Init();
        App_Init();
        P4App* app = App_Get();
        print_controls();
        result = EXIT_SUCCESS;
        bool enter_down = false;
        while (!stop) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    stop = 1;
                } else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                    const SDL_Keycode key = event.key.keysym.sym;
                    if (key == SDLK_q) {
                        stop = 1;
                    } else if (key == SDLK_RIGHT || key == SDLK_DOWN) {
                        app->on_input(P4App::InputAction::NextFocus);
                    } else if (key == SDLK_LEFT || key == SDLK_UP) {
                        app->on_input(P4App::InputAction::PreviousFocus);
                    } else if (key == SDLK_ESCAPE) {
                        app->on_input(P4App::InputAction::Back);
                    } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                        if (event.key.keysym.mod & KMOD_CTRL) {
                            app->on_input(P4App::InputAction::Commit);
                        } else if (!enter_down) {
                            enter_down = true;
                            app->on_input(P4App::InputAction::Press);
                        }
                    } else {
                        inject_event(key);
                    }
                } else if (event.type == SDL_KEYUP
                           && (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER)) {
                    if (enter_down) {
                        enter_down = false;
                        app->on_input(P4App::InputAction::Release);
                    }
                } else if (event.type == SDL_MOUSEWHEEL && event.wheel.y != 0) {
                    app->on_input(event.wheel.y > 0 ? P4App::InputAction::PreviousFocus
                                                    : P4App::InputAction::NextFocus);
                } else if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN
                           || event.type == SDL_MOUSEBUTTONUP) {
                    const int x = event.type == SDL_MOUSEMOTION ? event.motion.x : event.button.x;
                    const int y = event.type == SDL_MOUSEMOTION ? event.motion.y : event.button.y;
                    pointer.point = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y)};
                    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
                        app->on_input(P4App::InputAction::Back);
                    } else if (event.type != SDL_MOUSEMOTION && event.button.button == SDL_BUTTON_MIDDLE) {
                        app->on_input(event.type == SDL_MOUSEBUTTONDOWN ? P4App::InputAction::Press
                                                                        : P4App::InputAction::Release);
                    } else if (event.type != SDL_MOUSEMOTION && event.button.button == SDL_BUTTON_LEFT) {
                        pointer.state =
                            event.type == SDL_MOUSEBUTTONDOWN ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
                        lv_timer_ready(indev->driver->read_timer);
                    }
                } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    enter_down = false;
                    app->cancel_input();
                    pointer.state = LV_INDEV_STATE_RELEASED;
                    lv_indev_reset(indev, nullptr);
                }
            }
            HAL::HAL_Update();
            simulate_host(lv_tick_get());
            App_Update();
            lv_timer_handler();
            SDL_SetWindowTitle(window, app->current_page());
            if (frame.dirty) {
                if (SDL_UpdateTexture(texture, nullptr, frame.pixels.data(), kWidth * 4) != 0
                    || SDL_RenderClear(renderer) != 0 || SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0) {
                    std::fprintf(stderr, "SDL render: %s\n", SDL_GetError());
                    result = EXIT_FAILURE;
                    break;
                }
                SDL_RenderPresent(renderer);
                frame.dirty = false;
            }
            SDL_Delay(5);
        }
        App_Deinit(); // 页面与 Account 在 LVGL 对象仍有效时清理
    }
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    return result;
}
