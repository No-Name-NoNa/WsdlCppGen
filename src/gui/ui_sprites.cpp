#include "ui_sprites.hpp"

#include "ui_sprites.generated.hpp"

#include "imgui_internal.h"

#include <algorithm>
#include <cstring>

namespace
{
WsdlCppGen::SpriteRect ToSpriteRect(const UiSprites::Rect& rect)
{
    return {rect.x, rect.y, rect.w, rect.h, rect.border};
}

ImVec2 Uv(float x, float y)
{
    return ImVec2(x / static_cast<float>(UiSprites::AtlasWidth), y / static_cast<float>(UiSprites::AtlasHeight));
}

const UiSprites::Rect& IconRect(WsdlCppGen::SpriteIcon icon)
{
    switch (icon)
    {
    case WsdlCppGen::SpriteIcon::File:
        return UiSprites::IconFile;
    case WsdlCppGen::SpriteIcon::Folder:
        return UiSprites::IconFolder;
    case WsdlCppGen::SpriteIcon::Generate:
        return UiSprites::IconGenerate;
    case WsdlCppGen::SpriteIcon::Open:
        return UiSprites::IconOpen;
    case WsdlCppGen::SpriteIcon::Ok:
        return UiSprites::IconOk;
    case WsdlCppGen::SpriteIcon::Error:
        return UiSprites::IconError;
    case WsdlCppGen::SpriteIcon::None:
    default:
        return UiSprites::IconFile;
    }
}

const UiSprites::Rect& ButtonRect(WsdlCppGen::SpriteButtonKind kind, bool hovered, bool active)
{
    if (kind == WsdlCppGen::SpriteButtonKind::Primary)
    {
        if (active)
            return UiSprites::ButtonPrimaryActive;
        if (hovered)
            return UiSprites::ButtonPrimaryHover;
        return UiSprites::ButtonPrimary;
    }

    if (active)
        return UiSprites::ButtonSecondaryActive;
    if (hovered)
        return UiSprites::ButtonSecondaryHover;
    return UiSprites::ButtonSecondary;
}

const UiSprites::Rect& FrameRect(WsdlCppGen::SpriteFrame frame)
{
    switch (frame)
    {
    case WsdlCppGen::SpriteFrame::Panel:
        return UiSprites::Panel;
    case WsdlCppGen::SpriteFrame::PanelAlt:
        return UiSprites::PanelAlt;
    case WsdlCppGen::SpriteFrame::Banner:
        return UiSprites::Banner;
    case WsdlCppGen::SpriteFrame::StatusOk:
        return UiSprites::StatusOk;
    case WsdlCppGen::SpriteFrame::StatusError:
        return UiSprites::StatusError;
    default:
        return UiSprites::Panel;
    }
}

void AddNearestCallback(ImDrawList* draw_list)
{
    ImDrawCallback callback = ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest;
    if (callback)
        draw_list->AddCallback(callback);
}

void AddResetCallback(ImDrawList* draw_list)
{
    ImDrawCallback callback = ImGui::GetPlatformIO().DrawCallback_ResetRenderState;
    if (callback)
        draw_list->AddCallback(callback);
}
}

namespace WsdlCppGen
{
void UiSpriteAtlas::Init()
{
    if (registered)
        return;

    texture.Create(ImTextureFormat_RGBA32, UiSprites::AtlasWidth, UiSprites::AtlasHeight);
    std::memcpy(texture.GetPixels(), UiSprites::Pixels, sizeof(UiSprites::Pixels));
    ImGui::RegisterUserTexture(&texture);
    registered = true;
}

void UiSpriteAtlas::Shutdown()
{
    if (!registered || ImGui::GetCurrentContext() == nullptr)
        return;

    ImGui::UnregisterUserTexture(&texture);
    registered = false;
}

ImTextureRef UiSpriteAtlas::Ref() const
{
    return const_cast<ImTextureData&>(texture).GetTexRef();
}

void DrawSprite9(UiSpriteAtlas& atlas, const SpriteRect& rect, const ImVec2& min, const ImVec2& max, ImU32 tint)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const float width = max.x - min.x;
    const float height = max.y - min.y;
    if (width <= 0.0f || height <= 0.0f)
        return;

    const float border_x = std::min(static_cast<float>(rect.border), width * 0.5f);
    const float border_y = std::min(static_cast<float>(rect.border), height * 0.5f);

    const float sx[4] = {
        static_cast<float>(rect.x),
        static_cast<float>(rect.x + rect.border),
        static_cast<float>(rect.x + rect.w - rect.border),
        static_cast<float>(rect.x + rect.w)};
    const float sy[4] = {
        static_cast<float>(rect.y),
        static_cast<float>(rect.y + rect.border),
        static_cast<float>(rect.y + rect.h - rect.border),
        static_cast<float>(rect.y + rect.h)};
    const float dx[4] = {min.x, min.x + border_x, max.x - border_x, max.x};
    const float dy[4] = {min.y, min.y + border_y, max.y - border_y, max.y};

    AddNearestCallback(draw_list);
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            if (dx[x + 1] <= dx[x] || dy[y + 1] <= dy[y])
                continue;
            draw_list->AddImage(
                atlas.Ref(),
                ImVec2(dx[x], dy[y]),
                ImVec2(dx[x + 1], dy[y + 1]),
                Uv(sx[x], sy[y]),
                Uv(sx[x + 1], sy[y + 1]),
                tint);
        }
    }
    AddResetCallback(draw_list);
}

void DrawSprite9(UiSpriteAtlas& atlas, SpriteFrame frame, const ImVec2& min, const ImVec2& max, ImU32 tint)
{
    DrawSprite9(atlas, ToSpriteRect(FrameRect(frame)), min, max, tint);
}

void DrawSpriteIcon(UiSpriteAtlas& atlas, SpriteIcon icon, const ImVec2& min, float size, ImU32 tint)
{
    if (icon == SpriteIcon::None || size <= 0.0f)
        return;

    const UiSprites::Rect& rect = IconRect(icon);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    AddNearestCallback(draw_list);
    draw_list->AddImage(
        atlas.Ref(),
        min,
        ImVec2(min.x + size, min.y + size),
        Uv(static_cast<float>(rect.x), static_cast<float>(rect.y)),
        Uv(static_cast<float>(rect.x + rect.w), static_cast<float>(rect.y + rect.h)),
        tint);
    AddResetCallback(draw_list);
}

bool SpriteButton(UiSpriteAtlas& atlas, const char* label, const ImVec2& size, SpriteButtonKind kind, SpriteIcon icon, float icon_size)
{
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(label, size);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();

    DrawSprite9(atlas, ToSpriteRect(ButtonRect(kind, hovered, active)), min, max);

    const ImVec4 text_color = (kind == SpriteButtonKind::Primary)
        ? ImVec4(0.97f, 0.95f, 0.89f, 1.0f)
        : ImGui::GetStyleColorVec4(ImGuiCol_Text);
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    const ImVec2 text_size = ImGui::CalcTextSize(label);
    const float drawn_icon_size = icon == SpriteIcon::None ? 0.0f : icon_size;
    const float gap = icon == SpriteIcon::None ? 0.0f : icon_size * 0.5f;
    const float content_width = drawn_icon_size + gap + text_size.x;
    float content_x = pos.x + (size.x - content_width) * 0.5f;

    if (icon != SpriteIcon::None)
    {
        DrawSpriteIcon(atlas, icon, ImVec2(content_x, pos.y + (size.y - drawn_icon_size) * 0.5f), drawn_icon_size);
        content_x += drawn_icon_size + gap;
    }
    ImGui::GetWindowDrawList()->AddText(ImVec2(content_x, pos.y + (size.y - text_size.y) * 0.5f), ImGui::GetColorU32(ImGuiCol_Text), label);
    ImGui::PopStyleColor();

    return pressed;
}

bool SpriteInputText(UiSpriteAtlas& atlas, const char* id, char* buffer, size_t buffer_size, float width)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID input_id = window->GetID(id);
    const bool focused = ImGui::GetCurrentContext()->ActiveId == input_id;

    const ImVec2 min = ImGui::GetCursorScreenPos();
    const float height = ImGui::GetFrameHeight();
    DrawSprite9(atlas, ToSpriteRect(focused ? UiSprites::InputFocus : UiSprites::Input), min, ImVec2(min.x + width, min.y + height));

    ImGui::SetNextItemWidth(width);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    const bool changed = ImGui::InputText(id, buffer, buffer_size);
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
    return changed;
}
}
