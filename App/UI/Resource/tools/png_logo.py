"""标准库 PNG 读取：非隔行 Gray/GrayAlpha/RGB/RGBA（8/16 位）及索引色（1/2/4/8 位）。"""
from __future__ import annotations

import struct
import zlib
from collections import Counter
from pathlib import Path

import convert_images_v9 as codec


def decode_rows(decoded: bytes, w: int, h: int, depth: int, color: int,
                palette: bytes | None, transparency: bytes | None) -> bytearray:
    """还原五种字节滤波，解包索引/采样值，16 位取高字节降为 8 位 RGBA。"""
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[color]
    stride = (w * channels * depth + 7) // 8
    bpp = max(1, (channels * depth + 7) // 8)
    previous, rgba = bytearray(stride), bytearray()
    key = struct.unpack('>' + 'H' * (len(transparency) // 2), transparency) if transparency and color != 3 else None
    for y in range(h):
        mode = decoded[y * (stride + 1)]
        row = bytearray(decoded[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        if mode > 4:
            raise ValueError('PNG 行滤波类型无效')
        for i in range(stride):
            left = row[i - bpp] if i >= bpp else 0
            up, corner = previous[i], previous[i - bpp] if i >= bpp else 0
            prediction = left + up - corner
            distances = abs(prediction - left), abs(prediction - up), abs(prediction - corner)
            paeth = (left, up, corner)[distances.index(min(distances))]
            row[i] = (row[i] + (0, left, up, (left + up) // 2, paeth)[mode]) & 255
        previous = row
        if depth == 16:
            samples = struct.unpack('>' + 'H' * (w * channels), row)
        elif depth == 8:
            samples = row
        else:
            samples = [(row[x * depth // 8] >> (8 - depth - x * depth % 8)) & ((1 << depth) - 1)
                       for x in range(w)]
        for x in range(w):
            pixel = samples[x * channels:(x + 1) * channels]
            if color == 3:
                index = pixel[0]
                if palette is None or index >= len(palette) // 3:
                    raise ValueError('PNG 像素索引超出 PLTE 调色板')
                rgb = palette[index * 3:index * 3 + 3]
                alpha = transparency[index] if transparency and index < len(transparency) else 255
            else:
                values = [sample >> 8 if depth == 16 else sample * 255 // ((1 << depth) - 1) for sample in pixel]
                rgb = values[:3] if color in (2, 6) else values[:1] * 3
                alpha = values[-1] if color in (4, 6) else 255
                if key and tuple(pixel) == key:
                    alpha = 0
            rgba.extend((*rgb, alpha))
    return rgba


def read(path: Path, background: tuple | None = None, fit: bool = False) -> codec.Image8:
    """读取 PNG，透明像素合成到底色；超出屏幕须显式允许等比缩小。"""
    raw = path.read_bytes()
    if raw[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('无效的 PNG 签名')
    offset, payload, header, ended = 8, bytearray(), None, False
    palette, transparency = None, None
    while offset < len(raw):
        if offset + 12 > len(raw):
            raise ValueError('PNG 数据块被截断')
        length, kind = struct.unpack_from('>I4s', raw, offset)
        offset += 8
        body = raw[offset:offset + length]
        if offset + length + 4 > len(raw):
            raise ValueError('PNG 数据块长度无效')
        crc, = struct.unpack_from('>I', raw, offset + length)
        if zlib.crc32(kind + body) != crc:
            raise ValueError('PNG CRC 校验失败')
        offset += length + 4
        if header is None and kind != b'IHDR':
            raise ValueError('PNG 首块必须是 IHDR')
        if kind == b'IHDR':
            if header is not None or length != 13:
                raise ValueError('PNG IHDR 无效')
            header = struct.unpack('>IIBBBBB', body)
        elif kind == b'PLTE':
            if palette is not None or not length or length % 3 or length > 768:
                raise ValueError('PNG PLTE 长度无效或重复')
            palette = body
        elif kind == b'tRNS':
            if transparency is not None or not length:
                raise ValueError('PNG tRNS 为空或重复')
            transparency = body
        elif kind == b'IDAT':
            payload.extend(body)
        elif kind == b'IEND':
            if length:
                raise ValueError('PNG IEND 长度必须为零')
            ended = True
            break
        elif not kind[0] & 32:
            raise ValueError(f'不支持的 PNG 关键块 {kind!r}')
    if header is None or not ended:
        raise ValueError('PNG 缺少 IHDR/IEND')
    w, h, depth, color, compression, filtering, interlace = header
    if interlace:
        raise ValueError('暂不支持隔行 PNG；请导出非隔行图片')
    depths = {0: (1, 2, 4, 8, 16), 2: (8, 16), 3: (1, 2, 4, 8), 4: (8, 16), 6: (8, 16)}
    if depth not in depths.get(color, ()) or compression or filtering:
        raise ValueError('PNG 颜色类型、位深或压缩/滤波方法不支持')
    if color == 3 and (palette is None or len(palette) // 3 > 1 << depth):
        raise ValueError('索引色 PNG 缺少 PLTE 或调色板超出位深范围')
    if transparency is not None:
        expected = {0: 2, 2: 6}.get(color)
        if ((color == 3 and (palette is None or len(transparency) > len(palette) // 3))
                or (color != 3 and (expected is None or len(transparency) != expected))):
            raise ValueError('PNG tRNS 长度与颜色类型不匹配')
    if not 0 < w <= 4096 or not 0 < h <= 4096:
        raise ValueError('PNG 尺寸必须在 1..4096 像素内')
    if (w > 294 or h > 126) and not fit:
        raise ValueError('PNG 超出 294x126；传 --fit 等比缩小，或重新导出')
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[color]
    stride = (w * channels * depth + 7) // 8
    size = h * (stride + 1)
    decoder = zlib.decompressobj()
    try:
        decoded = decoder.decompress(payload, size + 1)
    except zlib.error as exc:
        raise ValueError(f'PNG 压缩数据无效：{exc}') from exc
    if len(decoded) != size or not decoder.eof or decoder.unused_data:
        raise ValueError('PNG 解压后长度无效')
    rgba = decode_rows(decoded, w, h, depth, color, palette, transparency)
    if background is None:
        transparent = [tuple(rgba[i:i + 3]) for i in range(0, len(rgba), 4) if rgba[i + 3] == 0]
        visible = [tuple(rgba[i:i + 3]) for i in range(0, len(rgba), 4) if rgba[i + 3] > 0]
        colors = transparent or [tuple(rgba[i:i + 3]) for i in range(0, len(rgba), 4)]
        background = Counter(colors).most_common(1)[0][0]
        if transparent and visible and set(visible) == {background}:
            background = (0, 0, 0) if sum(background) >= 384 else (255, 255, 255)
    for i in range(0, len(rgba), 4):
        a = rgba[i + 3]
        rgba[i:i + 4] = bytes((*((rgba[i + k] * a + background[k] * (255 - a) + 127) // 255
                                 for k in range(3)), 255))
    if fit and (w > 294 or h > 126):
        scale = min(294 / w, 126 / h)
        ww, hh = max(1, int(w * scale)), max(1, int(h * scale))
        rgba = bytearray(v for y in range(hh) for x in range(ww)
                         for v in rgba[((y * h // hh) * w + x * w // ww) * 4:((y * h // hh) * w + x * w // ww) * 4 + 4])
        w, h = ww, hh
    return codec.Image8(path, 'img_src_logo', 'LV_IMG_CF_TRUE_COLOR_ALPHA', w, h, len(rgba), bytes(rgba))
