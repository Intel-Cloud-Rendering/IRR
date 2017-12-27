#pragma once

#include <memory>

// remote renderer header files
#include "Common.h"

extern "C" {


namespace irr {

/*
 * define variables by needs,
 * hard code their initial value for now
 */
typedef struct IrrConfig{
    IRRCFG_INT(hw_lcd_width,
            "lcd.width",
            720,
            "LCD pixel width",
            "")

    IRRCFG_INT(hw_lcd_height,
            "lcd.height",
            1280,
            "LCD pixel height",
            "")

    IRRCFG_INT(hw_lcd_depth,
            "lcd.depth",
            16,
            "LCD color depth",
            "Color bit depth of emulated framebuffer.")

    IRRCFG_BOOL(is_phone_api,
            "api.type",
            true,
            "",
            "")

    IRRCFG_INT(api_level,
            "api.level",
            1,
            "",
            "")
} IrrConfig;

using IrrConfigPtr = std::unique_ptr<irr::IrrConfig>;

typedef struct IrrOnPostContext{
    int reserved;
    int size;
} IrrOnPostContext;


} // end_of_irr(intel_remote_renderer)

}
