// 统一 PC SDL 模拟器入口：共享应用（App/）+ 可选后端：
// - SIM_BACKEND=Board（默认）：HC32 平台层（Vendor/HC32/Display/AppPlatform）+ 模拟 HAL（运行固件 PowerPolicy）
// - SIM_BACKEND=Demo：Linux 演示后端（Vendor/Linux/Backends/Demo，支持 --random-status [seed]）
// 窗口 294×126，1:1 像素。
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "App.h"
#if SIM_BACKEND_DEMO
#include "DemoStatus.h"
#else
#include "AppPlatform.h"
#include "HAL/HAL.h"
#include "SystemState.h"
#include "mcu_version_config.h"
#endif

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

struct CliOptions {
    bool random_status = false;
    std::uint32_t seed = 1;
};

void
on_signal(int) {
    stop = 1;
}

/**
 * @brief Parse and validate simulator command-line arguments.
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 * @param options Parsed CLI options output.
 * @return true if arguments are valid for the active backend, false otherwise.
 */
bool
parse_args(int argc, char** argv, CliOptions& options) {
#if SIM_BACKEND_DEMO
    options.random_status = argc >= 2 && std::strcmp(argv[1], "--random-status") == 0;
    bool valid_args = argc == 1 || (options.random_status && argc <= 3);
    if (valid_args && argc == 3) {
        char* end = nullptr;
        errno = 0;
        const auto value = std::strtoul(argv[2], &end, 10);
        valid_args = !errno && argv[2][0] >= '0' && argv[2][0] <= '9' && !*end && value <= UINT32_MAX;
        if (valid_args) {
            options.seed = static_cast<std::uint32_t>(value);
        }
    }
    if (!valid_args) {
        std::fprintf(stderr, "Usage: %s [--random-status [SEED_0_TO_4294967295]]\n", argv[0]);
        return false;
    }
    return true;
#else
    (void)options;
    if (argc != 1) {
        std::fprintf(stderr, "Usage: %s (note: --random-status is only supported with SIM_BACKEND=Demo)\n", argv[0]);
        return false;
    }
    return true;
#endif
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

#if SIM_BACKEND_DEMO
/**
 * @brief Print keyboard and mouse control instructions for the Demo backend.
 * @param options Parsed CLI options describing whether random status mode is active.
 */
void
print_controls(const CliOptions& options) {
    std::puts("Host controls: arrows/wheel = focus (+1 in editor), Enter/middle click = press/release, "
              "hold 2s to Start, Esc/right click = back/cancel, Ctrl+Enter = commit, Q = quit. "
              "Power/time are simulated.");
    if (options.random_status) {
        std::printf("Random DEMO status: seed=%u, new battery/charging/Wi-Fi/recording/satellites "
                    "every 3s; publication every 200ms.\n",
                    options.seed);
    }
}
#else
// 主板版本：V1.5 带 MP2762A，V1.3 没有（GPIO 控制充电）；与 HAL::I2C_Scan 的推断一致。
void
set_board(bool v15) {
    BoardInfo_t& board = State::Board();
    board.online_device.mp2762 = v15;
    std::snprintf(board.hardware_version, sizeof(board.hardware_version), "%s", v15 ? "V1.5" : "V1.3");
}

void report_host_data();

// 模拟器中的初始状态：板卡与电池（代替 HAL）和主控已上报的数据（代替面板协议）。
void
init_system_info() {
    State::Reset();
    BoardInfo_t& board = State::Board();
    std::snprintf(board.software_version, sizeof(board.software_version), "%s", SOFTWARE_VERSION);
    set_board(true);
    board.online_device.bq40z50 = true;
    auto& battery = State::Power().batteryInfo;
    battery.sample_valid = true;
    battery.Percent = 76;
    battery.Voltage_f = 7.92f;
    battery.Temp_f = 32.0f;
    battery.Percent_f = 76.0f;
    battery.chargeStatus = notCharge;
    State::Power().ExternalPower = 1;
    report_host_data();
}

// 模拟主控上报的数据（冷启动时已在线；驻留后开机时固件清除主控数据，见 simulate_host）。
void
report_host_data() {
    HostReport_t& host = State::Host();
    host.online = true;
    auto& position = host.position;
    position.coordinate_status = position_fix;
    position.satellite_number_used = 18;
    position.satellite_number_track = 26;
    position.coordinate_lat = 31.230416;
    position.coordinate_lon = 121.473701;
    position.coordinate_alt = 12.5;
    host.work = {rover_mode, On_Off_Status_ON,  radio_mode_rx,    1,
                 3,          On_Off_Status_OFF, On_Off_Status_OFF}; // TRIMTALK，信道 3
    auto& record = host.record;
    record.record_type = 1;
    record.record_interval = 0x00;
    record.record_leftspace = 2048.0f;
    std::memcpy(record.record_name, "SIM0001", 7);
    auto& wifi = host.wifi;
    wifi.wifi_status = On_Off_Status_ON;
    std::snprintf(wifi.wifi_ssid, sizeof(wifi.wifi_ssid), "%s-SIM", PRODUCT_NAME);
    wifi.wifi_ip[0] = 192;
    wifi.wifi_ip[1] = 168;
    wifi.wifi_ip[2] = 10;
    wifi.wifi_ip[3] = 1;
    auto& stars = host.starMap;
    stars.numberGPS = 8;
    stars.numberBDS = 14;
    stars.numberGLONASS = 6;
    stars.numberGALILEO = 4;
    stars.numberSBAS = 2;
    stars.numberQZSS = 3;
    stars.numberIRNSS = 1;
}

void
print_controls() {
    std::puts("Controls: arrows/wheel = focus, Enter/middle click = press/release (hold 2s on Startup),\n"
              "          Esc/right click = back, Ctrl+Enter = commit, left click = pointer, Q = quit.\n"
              "HC32 events: H = host shutdown request, L = low-battery shutdown, F = forced shutdown,\n"
              "             T/G = battery temperature +5/-5 C, C = toggle charging, B/N = battery -10/+10%,\n"
              "             V = toggle board V1.5/V1.3 (MP2762A present or not; checked at the next power-on).");
}

// 模拟主控：读走面板请求即确认（代替协议的完整读出），1 s 后应用 Wi-Fi；读到关机标志即确认关机
// （固件电源时序随后 8 s 断电进入驻留）；每秒刷新一次星图数据。
void
simulate_host(uint32_t now) {
    static uint32_t wifi_since = 0;
    static bool wifi_applying = false; // 已收到设定，1 s 后应用
    static uint32_t last_stars = 0;
    static uint32_t offline_since = 0;
    // 驻留后开机：主控重新上电启动，约 3 s 后重新上报数据。
    if (!systemInfo.host.online) {
        if (offline_since == 0) {
            offline_since = now;
        } else if (systemInfo.power.panel_power_on && now - offline_since >= 3000) {
            offline_since = 0;
            report_host_data();
            std::printf("[sim] host: online again\n");
        }
    }
    const PanelRequest_t& request = systemInfo.request;
    HostReport_t& host = State::Host();
    if (request.wifi_pending) {
        State::Confirm_Wifi(State::Wifi_Revision());
        wifi_since = now;
        wifi_applying = true;
        std::printf("[sim] host: wifi set %s\n", request.wifi_on ? "on" : "off");
    }
    if (wifi_applying && now - wifi_since >= 1000) {
        wifi_applying = false;
        host.wifi.wifi_status = request.wifi_on;
    }
    if (request.work_pending) {
        State::Confirm_Work(State::Work_Revision());
        const WorkConfig_t& work = host.work;
        std::printf("[sim] host: work mode %d, radio %s, protocol %u, channel %u, ntrip server %d\n", work.work_mode,
                    work.radio_status ? "on" : "off", work.radio_protocol, work.radio_channel, work.NtripServer_status);
    }
    if (request.record_op || request.record_settings_pending) {
        State::Confirm_Record(State::Record_Revision(), true);
        const RecordInfo_t& record = host.record;
        std::printf("[sim] host: record %s, type %u, interval 0x%02x\n", record.record_status ? "start" : "stop",
                    record.record_type, record.record_interval);
    }
    if (systemInfo.power.poweroff_flag) {
        State::Host_ShutdownAcknowledged();
        std::printf("[sim] host: shutdown acknowledged\n");
    }
    if (now - last_stars >= 1000) {
        last_stars = now;
        ++host.starMap.sequence;
    }
}

// 模拟 HAL/主控事件，便于检查平台驱动的流程。
void
inject_event(SDL_Keycode key) {
    // 模拟器代替 HAL 与主控写入电源域。
    auto& power = State::Power();
    auto& battery = power.batteryInfo;
    switch (key) {
        case SDLK_h: State::Host_ShutdownRequested(); break;
        case SDLK_l: power.LowBatteryPowerOff = true; break;
        case SDLK_f: State::Request_ForcedShutdown(); break;
        case SDLK_t: battery.Temp_f += 5.0f; break;
        case SDLK_g: battery.Temp_f -= 5.0f; break;
        case SDLK_c: battery.chargeStatus = battery.chargeStatus == notCharge ? normalCharge : notCharge; break;
        case SDLK_b: battery.Percent = battery.Percent >= 10 ? battery.Percent - 10 : 0; break;
        case SDLK_n: battery.Percent = battery.Percent <= 90 ? battery.Percent + 10 : 100; break;
        case SDLK_v:
            set_board(!systemInfo.board.online_device.mp2762);
            std::printf("[sim] board %s\n", systemInfo.board.hardware_version);
            return;
        default: return;
    }
    battery.Percent_f = battery.Percent; // SystemInfos 显示浮点电量
    std::printf("[sim] temperature %.1f C, battery %u%%, charging %d, shutdown flags host=%d low=%d forced=%d\n",
                static_cast<double>(battery.Temp_f), battery.Percent, battery.chargeStatus != notCharge,
                power.LinuxPowerOff, power.LowBatteryPowerOff, power.Force_ShutDown);
}
#endif
} // namespace

int
main(int argc, char** argv) {
    CliOptions options{};
    if (!parse_args(argc, argv, options)) {
        return EXIT_FAILURE;
    }
    if (std::signal(SIGINT, on_signal) == SIG_ERR || std::signal(SIGTERM, on_signal) == SIG_ERR) {
        std::perror("signal");
        return EXIT_FAILURE;
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL init: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
#if SIM_BACKEND_DEMO
    constexpr const char* kWindowTitle = "App preview";
#else
    constexpr const char* kWindowTitle = "HC32 " PRODUCT_NAME " simulator";
#endif
    SDL_Window* window =
        SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, kWidth, kHeight, SDL_WINDOW_SHOWN);
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
        lv_disp_t* display = lv_disp_drv_register(&driver);

        static Pointer pointer;
        static lv_indev_drv_t pointer_driver;
        lv_indev_drv_init(&pointer_driver);
        pointer_driver.type = LV_INDEV_TYPE_POINTER;
        pointer_driver.read_cb = pointer_read;
        pointer_driver.user_data = &pointer;
        lv_indev_t* indev = display ? lv_indev_drv_register(&pointer_driver) : nullptr;

        if (!indev) {
            std::fprintf(stderr, "LVGL display/pointer init failed\n");
        } else {
#if SIM_BACKEND_DEMO
            App demo_app;
            App* app = demo_app.init() ? &demo_app : nullptr;
            if (!app) {
                std::fprintf(stderr, "App init failed\n");
            } else {
                print_controls(options);
                const auto started = SDL_GetTicks64();
                auto last_status = started;
                app->update_status(options.random_status ? DemoStatus::sample_random(0, options.seed)
                                                         : DemoStatus::sample(0));
#else
            init_system_info();
            HAL::HAL_Init();
            App_Init();
            App* app = App_Get();
            {
                print_controls();
                bool cold_boot_wait = true;
#endif
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
                                app->on_input(App::InputAction::NextFocus);
                            } else if (key == SDLK_LEFT || key == SDLK_UP) {
                                app->on_input(App::InputAction::PreviousFocus);
                            } else if (key == SDLK_ESCAPE) {
                                app->on_input(App::InputAction::Back);
                            } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                                if (event.key.keysym.mod & KMOD_CTRL) {
                                    app->on_input(App::InputAction::Commit);
                                } else if (!enter_down) {
                                    enter_down = true;
#if !SIM_BACKEND_DEMO
                                    HAL::Sim_SetPowerKey(true); // 原始电平仅模拟按键，开机由 Startup 完整长按请求。
#endif
                                    app->on_input(App::InputAction::Press);
                                }
                            } else {
#if !SIM_BACKEND_DEMO
                                inject_event(key);
#endif
                            }
                        } else if (event.type == SDL_KEYUP
                                   && (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER)) {
                            if (enter_down) {
                                enter_down = false;
#if !SIM_BACKEND_DEMO
                                HAL::Sim_SetPowerKey(false);
#endif
                                app->on_input(App::InputAction::Release);
                            }
                        } else if (event.type == SDL_MOUSEWHEEL && event.wheel.y != 0) {
                            app->on_input(event.wheel.y > 0 ? App::InputAction::PreviousFocus
                                                            : App::InputAction::NextFocus);
                        } else if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN
                                   || event.type == SDL_MOUSEBUTTONUP) {
                            const int x = event.type == SDL_MOUSEMOTION ? event.motion.x : event.button.x;
                            const int y = event.type == SDL_MOUSEMOTION ? event.motion.y : event.button.y;
                            pointer.point = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y)};
                            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
                                app->on_input(App::InputAction::Back);
                            } else if (event.type != SDL_MOUSEMOTION && event.button.button == SDL_BUTTON_MIDDLE) {
#if !SIM_BACKEND_DEMO
                                HAL::Sim_SetPowerKey(event.type == SDL_MOUSEBUTTONDOWN);
#endif
                                app->on_input(event.type == SDL_MOUSEBUTTONDOWN ? App::InputAction::Press
                                                                                : App::InputAction::Release);
                            } else if (event.type != SDL_MOUSEMOTION && event.button.button == SDL_BUTTON_LEFT) {
                                pointer.state =
                                    event.type == SDL_MOUSEBUTTONDOWN ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
                                lv_timer_ready(indev->driver->read_timer);
                            }
                        } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                            enter_down = false;
#if !SIM_BACKEND_DEMO
                            HAL::Sim_SetPowerKey(false);
#endif
                            app->cancel_input();
                            pointer.state = LV_INDEV_STATE_RELEASED;
                            lv_indev_reset(indev, nullptr);
                        }
                    }
#if SIM_BACKEND_DEMO
                    const auto now = SDL_GetTicks64();
                    app->update();
                    lv_timer_handler();
                    if (now - last_status >= 200) {
                        app->update_status(options.random_status ? DemoStatus::sample_random(now - started, options.seed)
                                                                 : DemoStatus::sample(now - started));
                        last_status = now;
                    }
#else
                    HAL::HAL_Update();
                    simulate_host(lv_tick_get());
                    App_Update();
                    lv_timer_handler();
                    if (cold_boot_wait && systemInfo.power.panel_power_on) {
                        // SDL 事件循环维持冷启动界面；Startup 确认后再退出固件等待并拉高主控电源。
                        HAL::Power_OnCheck();
                        cold_boot_wait = false;
                    }
#endif
                    SDL_SetWindowTitle(window, app->current_page());
                    if (frame.dirty) {
                        if (SDL_UpdateTexture(texture, nullptr, frame.pixels.data(), kWidth * 4) != 0
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
#if !SIM_BACKEND_DEMO
                App_Deinit(); // 页面与 Account 在 LVGL 对象仍有效时清理
#endif
            }
            lv_indev_delete(indev);
        }
        if (display) {
            lv_disp_remove(display);
        }
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
