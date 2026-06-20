#pragma once

#include "imgui.h"

namespace WsdlCppGen
{
struct UiSpriteAtlas
{
    ImTextureData texture;
    bool registered = false;

    void Init();
    void Shutdown();
    ImTextureRef Ref() const;
};

struct SpriteRect
{
    int x;
    int y;
    int w;
    int h;
    int border;
};

enum class SpriteButtonKind
{
    Primary,
    Secondary,
};

enum class SpriteIcon
{
    None,
    File,
    Folder,
    Generate,
    Open,
    Ok,
    Error,
};

enum class SpriteFrame
{
    Panel,
    PanelAlt,
    Banner,
    StatusOk,
    StatusError,
};

void DrawSprite9(UiSpriteAtlas& atlas, const SpriteRect& rect, const ImVec2& min, const ImVec2& max, ImU32 tint = IM_COL32_WHITE);
void DrawSprite9(UiSpriteAtlas& atlas, SpriteFrame frame, const ImVec2& min, const ImVec2& max, ImU32 tint = IM_COL32_WHITE);
void DrawSpriteIcon(UiSpriteAtlas& atlas, SpriteIcon icon, const ImVec2& min, float size, ImU32 tint = IM_COL32_WHITE);
bool SpriteButton(UiSpriteAtlas& atlas, const char* label, const ImVec2& size, SpriteButtonKind kind, SpriteIcon icon = SpriteIcon::None, float icon_size = 16.0f);
bool SpriteInputText(UiSpriteAtlas& atlas, const char* id, char* buffer, size_t buffer_size, float width);
}
