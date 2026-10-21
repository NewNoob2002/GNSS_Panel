/*
 * 命令、时序与编码迁自本仓库的 Adafruit_ST7789.cpp、Adafruit_ST77xx.cpp、
 * Adafruit_SPITFT.cpp / Adafruit_GFX.cpp（2.3 迁移时已逐项核对总线字节序列与时序）。
 *
 * Software License Agreement (BSD License)
 * Copyright (c) 2012 Adafruit Industries. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "PanelRm690a0.h"
#include "delay.h"

bool
PanelRm690a0::init() {
    // Adafruit_SPITFT::initSPI：保留输出默认 LOW 后单独写 HIGH 的 GPIO 顺序。
    cs.setMode(GPIO_OUTPUT);
    cs.write(true);
    dc.setMode(GPIO_OUTPUT);
    dc.write(true);
    if (!spi.begin()) {
        return false;
    }
    rst.setMode(GPIO_OUTPUT);
    rst.write(true);
    delay_ms(100);
    rst.write(false);
    delay_ms(100);
    rst.write(true);
    delay_ms(200);

    // Adafruit_ST77xx::displayInit 的第二轮硬件复位，不得与上一轮合并。
    rst.write(true);
    delay_ms(50);
    rst.write(false);
    delay_ms(50);
    rst.write(true);
    delay_ms(150);

    // Adafruit_ST7789.cpp generic_RM690A0 原表；255 的延时按 displayInit 解释为 500 ms。
    static constexpr uint8_t commands[] = {
        11, 0x01, 0x80, 150, 0xFE, 1,    0x01, 0x6A, 1,    0x21, 0xFE, 1,    0x00, 0xC4, 1,    0x80, 0x35,
        1,  0x00, 0x51, 1,   0xFF, 0x3A, 1,    0x05, 0x20, 0,    0x11, 0x80, 255,  0x29, 0x80, 255,
    };
    const uint8_t* cursor = commands;
    uint8_t remaining = *cursor++;
    while (remaining--) {
        const uint8_t command = *cursor++;
        const uint8_t flags = *cursor++;
        const uint8_t count = flags & 0x7FU;
        sendCommand(command, cursor, count);
        cursor += count;
        if (flags & 0x80U) {
            const uint16_t ms = *cursor++;
            delay_ms(ms == 255 ? 500 : ms);
        }
    }
    // Adafruit_ST7789::init(126,294) → setRotation(2)：MADCTL_RGB=0，colstart2/rowstart2=0。
    const uint8_t madctl = 0x00;
    sendCommand(0x36, &madctl, 1);
    return true;
}

void
PanelRm690a0::select() {
    cs.write(false);
}

void
PanelRm690a0::deselect() {
    cs.write(true);
}

void
PanelRm690a0::dataMode() {
    dc.write(true);
}

void
PanelRm690a0::writeCommand(uint8_t command) {
    // Adafruit_SPITFT::writeCommand：仅切换 DC，不操作 CS。
    dc.write(false);
    spi.writeByte(command);
    dc.write(true);
}

void
PanelRm690a0::sendCommand(uint8_t command, const uint8_t* data, size_t count) {
    // Adafruit_SPITFT::sendCommand：本平台无 SPI_HAS_TRANSACTION，事务边界只操作 CS。
    select();
    writeCommand(command);
    spi.write(data, count);
    deselect();
}

void
PanelRm690a0::writeAddress(uint16_t first, uint16_t last) {
    const uint8_t bytes[] = {static_cast<uint8_t>(first >> 8), static_cast<uint8_t>(first),
                             static_cast<uint8_t>(last >> 8), static_cast<uint8_t>(last)};
    spi.write(bytes, sizeof(bytes));
}

void
PanelRm690a0::setWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    // Adafruit_ST77xx::setAddrWindow + Adafruit_SPITFT::SPI_WRITE32；物理偏移为零。
    writeCommand(0x2A);
    writeAddress(x1, x2);
    writeCommand(0x2B);
    writeAddress(y1, y2);
    writeCommand(0x2C);
}

void
PanelRm690a0::fill(uint16_t color) {
    // Adafruit_GFX::fillScreen → SPITFT::fillRect / writeFillRectPreclipped / writeColor。
    select();
    setWindow(0, 0, width() - 1, height() - 1);
    for (uint32_t remaining = static_cast<uint32_t>(width()) * height(); remaining != 0; --remaining) {
        spi.writeByte(static_cast<uint8_t>(color >> 8));
        spi.writeByte(static_cast<uint8_t>(color));
    }
    deselect();
}

void
PanelRm690a0::enableBacklight() {
    if (hasBacklight) {
        blk.setMode(GPIO_OUTPUT);
        blk.write(false);
    }
}
