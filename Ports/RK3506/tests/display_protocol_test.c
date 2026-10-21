#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "lv_display.h"

struct mock {
    unsigned calls, fail_at, transfers, releases, reset_count, delay_count, commands;
    unsigned delays[9];
    uint8_t init_commands[12], init_values[12];
    int dc, selected, command, x1, x2, y1, y2, aligned;
    size_t position, max_chunk, limit;
    uint8_t frame[RM_WIDTH * RM_HEIGHT * 2];
};

static int step(struct mock* m) {
    if (++m->calls == m->fail_at) {
        errno = ETIMEDOUT;
        return -1;
    }
    return 0;
}

static int dc(void* context, int value) {
    struct mock* m = context;
    if (step(m) < 0)
        return -1;
    m->dc = value;
    return 0;
}

static int reset(void* context, int value) {
    struct mock* m = context;
    if (step(m) < 0)
        return -1;
    assert(value == (m->reset_count % 3 != 1));
    ++m->reset_count;
    return 0;
}

static int delay(void* context, unsigned ms) {
    struct mock* m = context;
    if (step(m) < 0)
        return -1;
    assert(m->delay_count < 9);
    m->delays[m->delay_count++] = ms;
    return 0;
}

static int transfer(void* context, const uint8_t* data, size_t size, int keep) {
    struct mock* m = context;
    if (step(m) < 0)
        return -1;
    if (!size) {
        assert(!keep);
        m->selected = 0;
        ++m->releases;
        return 0;
    }
    assert(data && size <= m->limit);
    ++m->transfers;
    if (size > m->max_chunk)
        m->max_chunk = size;
    if (!m->dc) {
        assert(size == 1);
        m->command = data[0];
        if (m->commands < 12)
            m->init_commands[m->commands] = data[0];
        ++m->commands;
        if (data[0] == 0x2c) {
            assert(m->selected && keep);
            m->position = 0;
        }
    } else {
        assert(m->selected); /* D/C change must not break command/data CS. */
        if (m->commands <= 12) {
            assert(size == 1);
            m->init_values[m->commands - 1] = data[0];
        } else if (m->command == 0x2a || m->command == 0x2b) {
            assert(size == 4 && keep);
            int first = data[0] * 256 + data[1], last = data[2] * 256 + data[3];
            assert(first <= last);
            if (m->aligned)
                assert(first % 2 == 0 && last % 2 == 1);
            if (m->command == 0x2a) {
                assert(last < RM_WIDTH);
                m->x1 = first;
                m->x2 = last;
            } else {
                assert(last < RM_HEIGHT);
                m->y1 = first;
                m->y2 = last;
            }
        } else {
            assert(m->command == 0x2c && size % 2 == 0);
            size_t width = (size_t)(m->x2 - m->x1 + 1);
            size_t total = width * (size_t)(m->y2 - m->y1 + 1) * 2;
            assert(m->position + size <= total);
            for (size_t i = 0; i < size; ++i) {
                size_t p = m->position + i;
                size_t offset =
                    ((size_t)m->y1 + p / 2 / width) * RM_WIDTH * 2 + ((size_t)m->x1 + p / 2 % width) * 2 + p % 2;
                m->frame[offset] = data[i];
            }
            m->position += size;
            assert(keep == (m->position != total));
        }
    }
    m->selected = keep;
    return 0;
}

static struct Rm690a0 setup(struct mock* m, size_t chunk) {
    memset(m, 0, sizeof(*m));
    memset(m->frame, 0xaa, sizeof(m->frame));
    m->limit = chunk;
    return (struct Rm690a0){.io = {m, dc, reset, delay, transfer, chunk}};
}

static uint16_t pixel(struct mock* m, int x, int y) {
    size_t p = ((size_t)y * RM_WIDTH + (size_t)x) * 2;
    return (uint16_t)(m->frame[p] * 256 + m->frame[p + 1]);
}

int main(void) {
    static struct mock m;
    struct Rm690a0 s = setup(&m, 4096);
    assert(rm_init(&s) == 0 && s.ready && !m.selected);
    unsigned init_calls = m.calls;
    const uint8_t commands[] = {1, 0xfe, 0x6a, 0xfe, 0xc4, 0x35, 0x51, 0x3a, 0x20, 0x11, 0x29, 0x36};
    const uint8_t values[] = {0, 1, 0x21, 0, 0x80, 0, 0xff, 5, 0, 0, 0, 0};
    const unsigned delays[] = {100, 100, 200, 50, 50, 150, 150, 500, 500};
    assert(!memcmp(commands, m.init_commands, sizeof(commands)));
    assert(!memcmp(values, m.init_values, sizeof(values)));
    assert(m.reset_count == 6 && m.delay_count == 9 && !memcmp(delays, m.delays, sizeof(delays)));
    for (size_t i = 0; i < sizeof(m.frame); ++i)
        assert(m.frame[i] == 0);

    static uint8_t frame[RM_WIDTH * RM_HEIGHT * 2];
    for (size_t i = 0; i < sizeof(frame); ++i)
        frame[i] = (uint8_t)(i * 17 + i / 256);
    const size_t chunks[] = {4, 4096, 32768};
    for (unsigned i = 0; i < 3; ++i) {
        s.io.chunk_size = m.limit = chunks[i];
        assert(rm_write(&s, 0, 0, RM_WIDTH, RM_HEIGHT, frame, sizeof(frame)) == 0);
        assert(!m.selected && !memcmp(frame, m.frame, sizeof(frame)));
    }
    s.io.chunk_size = m.limit = 4096;
    const uint8_t patch[] = {0xf8, 0x00, 0x07, 0xe0, 0x00, 0x1f, 0xff, 0xff};
    assert(rm_write(&s, 124, 292, 2, 2, patch, sizeof(patch)) == 0);
    assert(pixel(&m, 124, 292) == 0xf800 && pixel(&m, 125, 292) == 0x07e0);
    assert(pixel(&m, 124, 293) == 0x001f && pixel(&m, 125, 293) == 0xffff);
    unsigned calls = m.calls;
    assert(rm_write(&s, -1, 0, 2, 2, patch, 8) < 0 && errno == EINVAL);
    assert(rm_write(&s, 125, 293, 2, 2, patch, 8) < 0);
    assert(rm_write(&s, 0, 0, 2, 2, patch, 7) < 0);
    assert(rm_write(&s, 0, 0, 0, 2, patch, 0) < 0);
    assert(rm_write(&s, 0, 0, 2, 2, NULL, 8) < 0 && m.calls == calls);

    /* Every init/reset/GPIO/SPI/delay failure invalidates the panel and releases CS. */
    for (unsigned i = 1; i <= init_calls; ++i) {
        s = setup(&m, 4096);
        m.fail_at = i;
        assert(rm_init(&s) < 0 && errno == ETIMEDOUT && !s.ready && !m.selected);
    }
    s = setup(&m, 4096);
    assert(rm_init(&s) == 0);
    calls = m.calls;
    assert(rm_write(&s, 0, 0, RM_WIDTH, RM_HEIGHT, frame, sizeof(frame)) == 0);
    unsigned write_calls = m.calls - calls;
    for (unsigned i = 1; i <= write_calls; ++i) {
        s = setup(&m, 4096);
        assert(rm_init(&s) == 0);
        m.fail_at = m.calls + i;
        assert(rm_write(&s, 0, 0, RM_WIDTH, RM_HEIGHT, frame, sizeof(frame)) < 0);
        assert(errno == ETIMEDOUT && !s.ready && !m.selected);
        calls = m.calls;
        assert(rm_write(&s, 0, 0, 2, 2, patch, 8) < 0 && m.calls == calls);
    }

    s = setup(&m, 4096);
    assert(rm_init(&s) == 0);
    lv_init();
    m.aligned = 1;
    static struct lv_display port;
    assert(lv_display_register(&port, &s) == 0);
    assert(lv_disp_get_hor_res(port.display) == 294 && lv_disp_get_ver_res(port.display) == 126);
    lv_area_t area = {1, 3, 292, 124};
    port.driver.rounder_cb(&port.driver, &area);
    assert(area.x1 == 0 && area.y1 == 2 && area.x2 == 293 && area.y2 == 125);
    lv_display_pattern();
    lv_refr_now(port.display);
    assert(!port.error && !m.selected);
    for (int y = 0; y < RM_HEIGHT; ++y)
        assert(pixel(&m, 0, y) == 0xffe0 && pixel(&m, 125, y) == 0xffe0);
    for (int x = 0; x < RM_WIDTH; ++x)
        assert(pixel(&m, x, 0) == 0xffe0 && pixel(&m, x, 293) == 0xffe0);
    assert(pixel(&m, 60, 263) == 0xf800);
    assert(pixel(&m, 60, 203) == 0x07e0);
    assert(pixel(&m, 60, 143) == 0x001f);
    assert(pixel(&m, 60, 83) == 0xffff);
    assert(pixel(&m, 60, 23) == 0x0000);
    memcpy(frame, m.frame, sizeof(frame));
    lv_obj_t* partial = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(partial);
    lv_obj_set_pos(partial, 101, 51);
    lv_obj_set_size(partial, 6, 4);
    lv_obj_set_style_bg_color(partial, lv_color_hex(0xff00ff), 0);
    lv_obj_set_style_bg_opa(partial, LV_OPA_COVER, 0);
    lv_refr_now(port.display);
    for (int y = 0; y < RM_HEIGHT; ++y) {
        for (int x = 0; x < RM_WIDTH; ++x) {
            size_t p = ((size_t)y * RM_WIDTH + (size_t)x) * 2;
            uint16_t expected =
                x >= 51 && x <= 54 && y >= 187 && y <= 192 ? 0xf81f : (uint16_t)(frame[p] * 256 + frame[p + 1]);
            assert(pixel(&m, x, y) == expected);
        }
    }
    /* Flush error must release LVGL ownership instead of deadlocking the renderer. */
    m.fail_at = m.calls + 1;
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(port.display);
    assert(port.error == ETIMEDOUT && !port.draw.flushing && !m.selected);
    lv_disp_remove(port.display);
    assert(lv_mem_test() == LV_RES_OK);
    puts("P3 protocol, segmentation, fault injection and LVGL rotation checks passed");
    return 0;
}
