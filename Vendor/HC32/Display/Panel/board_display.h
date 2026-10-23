#pragma once

#include "PanelRm690a0.h"

// 仅带屏产品编译的板级组合；Platform/Config 不得反向依赖 Display。
extern SpiMaster board_display_spi;
extern PanelRm690a0 board_display_panel;
