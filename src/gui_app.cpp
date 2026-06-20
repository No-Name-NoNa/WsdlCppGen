#include "wsdl_codegen.hpp"
#include "native_dialogs.hpp"
#include "ui_sprites.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <array>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace {
    constexpr size_t PathBufferSize = 512;

    void CopyToBuffer(std::array<char, PathBufferSize> &buffer, const std::filesystem::path &path) {
        std::string text = path.generic_string();
        std::snprintf(buffer.data(), buffer.size(), "%s", text.c_str());
    }

    std::filesystem::path FindFontPath(const char *executable_path) {
        constexpr const char *font_name = "JetBrainsMono-Regular.ttf";
        std::vector<std::filesystem::path> candidates = {
            std::filesystem::path("font") / font_name,
            std::filesystem::path("..") / "font" / font_name,
            std::filesystem::path("..") / ".." / "font" / font_name,
            std::filesystem::path("cmake-build-debug") / "font" / font_name,
            std::filesystem::path("build") / "font" / font_name,
        };

        if (executable_path && *executable_path) {
            std::filesystem::path executable_dir = std::filesystem::absolute(executable_path).parent_path();
            candidates.push_back(executable_dir / "font" / font_name);
            candidates.push_back(executable_dir.parent_path() / "font" / font_name);
        }

        for (const std::filesystem::path &candidate: candidates)
            if (std::filesystem::exists(candidate))
                return candidate;

        throw std::runtime_error("JetBrains Mono font not found: font/JetBrainsMono-Regular.ttf");
    }

    void ApplyTheme() {
        ImGuiStyle &style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(22.0f, 22.0f);
        style.FramePadding = ImVec2(14.0f, 10.0f);
        style.ItemSpacing = ImVec2(14.0f, 14.0f);
        style.ItemInnerSpacing = ImVec2(10.0f, 8.0f);
        style.ScrollbarSize = 18.0f;
        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;
        style.WindowBorderSize = 0.0f;
        style.ChildBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.InputTextCursorSize = 2.0f;

        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(0.23f, 0.19f, 0.15f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.47f, 0.37f, 1.00f);
        colors[ImGuiCol_InputTextCursor] = ImVec4(0.23f, 0.19f, 0.15f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.36f, 0.59f, 0.56f, 0.35f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.957f, 0.933f, 0.882f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(1.000f, 0.976f, 0.937f, 0.98f);
        colors[ImGuiCol_Border] = ImVec4(0.41f, 0.35f, 0.26f, 0.80f);
        colors[ImGuiCol_FrameBg] = ImVec4(1.000f, 0.980f, 0.937f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.984f, 0.957f, 0.902f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.949f, 0.918f, 0.847f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.910f, 0.875f, 0.792f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.910f, 0.875f, 0.792f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.36f, 0.59f, 0.56f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.69f, 0.66f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.24f, 0.44f, 0.44f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.89f, 0.84f, 0.74f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.84f, 0.78f, 0.67f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.68f, 0.55f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.31f, 0.56f, 0.52f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.36f, 0.59f, 0.56f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.62f, 0.53f, 0.41f, 1.00f);
    }

    void SetWindowIcon(GLFWwindow *window) {
        std::array<unsigned char, 32 * 32 * 4> pixels{};
        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 32; ++x) {
                const int offset = (y * 32 + x) * 4;
                const bool border = x < 2 || y < 2 || x >= 30 || y >= 30;
                const bool arrow = (x > 9 && x < 23 && y > 8 && y < 13) || (x > 18 && x < 24 && y > 6 && y < 16);
                const bool bracket = (x >= 7 && x <= 9 && y >= 7 && y <= 24) || (
                                         x >= 23 && x <= 25 && y >= 7 && y <= 24);
                pixels[offset + 0] = border ? 72 : (arrow ? 74 : (bracket ? 41 : 13));
                pixels[offset + 1] = border ? 210 : (arrow ? 196 : (bracket ? 119 : 24));
                pixels[offset + 2] = border ? 225 : (arrow ? 216 : (bracket ? 154 : 31));
                pixels[offset + 3] = 255;
            }
        }
        GLFWimage icon{};
        icon.width = 32;
        icon.height = 32;
        icon.pixels = pixels.data();
        glfwSetWindowIcon(window, 1, &icon);
    }

    void CheckVk(VkResult result) {
        if (result != VK_SUCCESS)
            throw std::runtime_error("Vulkan error: " + std::to_string(result));
    }

    struct VulkanApp {
        GLFWwindow *window = nullptr;
        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        uint32_t queue_family = 0;
        VkQueue queue = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        ImGui_ImplVulkanH_Window main_window;
        uint32_t min_image_count = 2;
        WsdlCppGen::UiSpriteAtlas sprites;

        void InitWindow() {
            if (!glfwInit())
                throw std::runtime_error("glfwInit failed");
            if (!glfwVulkanSupported())
                throw std::runtime_error("GLFW reports Vulkan is not supported");
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            window = glfwCreateWindow(1120, 720, "WsdlCppGen", nullptr, nullptr);
            if (!window)
                throw std::runtime_error("glfwCreateWindow failed");
            SetWindowIcon(window);
        }

        void InitVulkan() {
            VkApplicationInfo app_info{};
            app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app_info.pApplicationName = "WsdlCppGen";
            app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            app_info.pEngineName = "WsdlCppGen";
            app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            app_info.apiVersion = VK_API_VERSION_1_2;

            uint32_t extension_count = 0;
            const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);
            VkInstanceCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            create_info.pApplicationInfo = &app_info;
            create_info.enabledExtensionCount = extension_count;
            create_info.ppEnabledExtensionNames = glfw_extensions;
            CheckVk(vkCreateInstance(&create_info, nullptr, &instance));

            physical_device = ImGui_ImplVulkanH_SelectPhysicalDevice(instance);
            if (physical_device == VK_NULL_HANDLE)
                throw std::runtime_error("No Vulkan physical device found");
            queue_family = ImGui_ImplVulkanH_SelectQueueFamilyIndex(physical_device);

            constexpr float queue_priority = 1.0f;
            VkDeviceQueueCreateInfo queue_info{};
            queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info.queueFamilyIndex = queue_family;
            queue_info.queueCount = 1;
            queue_info.pQueuePriorities = &queue_priority;

            const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
            VkDeviceCreateInfo device_info{};
            device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            device_info.queueCreateInfoCount = 1;
            device_info.pQueueCreateInfos = &queue_info;
            device_info.enabledExtensionCount = 1;
            device_info.ppEnabledExtensionNames = device_extensions;
            CheckVk(vkCreateDevice(physical_device, &device_info, nullptr, &device));
            vkGetDeviceQueue(device, queue_family, 0, &queue);

            CheckVk(glfwCreateWindowSurface(instance, window, nullptr, &surface));
            main_window.Surface = surface;
            const VkFormat request_formats[] = {
                VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM
            };
            main_window.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
                physical_device,
                surface,
                request_formats,
                static_cast<int>(std::size(request_formats)),
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
            const VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
            main_window.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(physical_device, surface, present_modes, 1);
            min_image_count = static_cast<uint32_t>(ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(
                main_window.PresentMode));

            int width = 0;
            int height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physical_device, device, &main_window, queue_family,
                                                   nullptr, width, height, min_image_count, 0);
        }

        void InitImGui(const char *executable_path) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            std::filesystem::path font_path = FindFontPath(executable_path);
            if (!io.Fonts->AddFontFromFileTTF(font_path.string().c_str(), 20.0f))
                throw std::runtime_error("Failed to load font: " + font_path.string());

            ApplyTheme();
            ImGui_ImplGlfw_InitForVulkan(window, true);

            ImGui_ImplVulkan_InitInfo init_info{};
            init_info.ApiVersion = VK_API_VERSION_1_2;
            init_info.Instance = instance;
            init_info.PhysicalDevice = physical_device;
            init_info.Device = device;
            init_info.QueueFamily = queue_family;
            init_info.Queue = queue;
            init_info.DescriptorPoolSize = 64;
            init_info.MinImageCount = min_image_count;
            init_info.ImageCount = main_window.ImageCount;
            init_info.PipelineInfoMain.RenderPass = main_window.RenderPass;
            init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            init_info.CheckVkResultFn = [](VkResult result) {
                if (result != VK_SUCCESS)
                    std::cerr << "Vulkan error: " << result << "\n";
            };
            if (!ImGui_ImplVulkan_Init(&init_info))
                throw std::runtime_error("ImGui_ImplVulkan_Init failed");

            sprites.Init();
        }

        void ResizeIfNeeded() {
            int width = 0;
            int height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            if (width <= 0 || height <= 0)
                return;
            if (main_window.Width == width && main_window.Height == height)
                return;
            vkDeviceWaitIdle(device);
            ImGui_ImplVulkan_SetMinImageCount(min_image_count);
            ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physical_device, device, &main_window, queue_family,
                                                   nullptr, width, height, min_image_count, 0);
        }

        void RenderFrame() {
            ImGui_ImplVulkanH_Window *wd = &main_window;
            VkSemaphore image_acquired = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
            VkSemaphore render_complete = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
            VkResult result = vkAcquireNextImageKHR(device, wd->Swapchain, UINT64_MAX, image_acquired, VK_NULL_HANDLE,
                                                    &wd->FrameIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
                return;
            CheckVk(result);

            ImGui_ImplVulkanH_Frame *fd = &wd->Frames[wd->FrameIndex];
            CheckVk(vkWaitForFences(device, 1, &fd->Fence, VK_TRUE, UINT64_MAX));
            CheckVk(vkResetFences(device, 1, &fd->Fence));
            CheckVk(vkResetCommandPool(device, fd->CommandPool, 0));

            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            CheckVk(vkBeginCommandBuffer(fd->CommandBuffer, &begin_info));

            VkRenderPassBeginInfo render_pass_info{};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass = wd->RenderPass;
            render_pass_info.framebuffer = fd->Framebuffer;
            render_pass_info.renderArea.extent.width = static_cast<uint32_t>(wd->Width);
            render_pass_info.renderArea.extent.height = static_cast<uint32_t>(wd->Height);
            render_pass_info.clearValueCount = 1;
            render_pass_info.pClearValues = &wd->ClearValue;
            vkCmdBeginRenderPass(fd->CommandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), fd->CommandBuffer);
            vkCmdEndRenderPass(fd->CommandBuffer);
            CheckVk(vkEndCommandBuffer(fd->CommandBuffer));

            VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &image_acquired;
            submit_info.pWaitDstStageMask = &wait_stage;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &fd->CommandBuffer;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &render_complete;
            CheckVk(vkQueueSubmit(queue, 1, &submit_info, fd->Fence));
        }

        void PresentFrame() {
            ImGui_ImplVulkanH_Window *wd = &main_window;
            VkSemaphore render_complete = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
            VkPresentInfoKHR present_info{};
            present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &render_complete;
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &wd->Swapchain;
            present_info.pImageIndices = &wd->FrameIndex;
            VkResult result = vkQueuePresentKHR(queue, &present_info);
            if (result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUBOPTIMAL_KHR)
                CheckVk(result);
            wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;
        }

        void Cleanup() {
            if (device != VK_NULL_HANDLE)
                vkDeviceWaitIdle(device);
            ImGui_ImplVulkan_Shutdown();
            sprites.Shutdown();
            ImGui_ImplGlfw_Shutdown();
            if (ImGui::GetCurrentContext())
                ImGui::DestroyContext();
            if (device != VK_NULL_HANDLE)
                ImGui_ImplVulkanH_DestroyWindow(instance, device, &main_window, nullptr);
            if (surface != VK_NULL_HANDLE)
                vkDestroySurfaceKHR(instance, surface, nullptr);
            if (device != VK_NULL_HANDLE)
                vkDestroyDevice(device, nullptr);
            if (instance != VK_NULL_HANDLE)
                vkDestroyInstance(instance, nullptr);
            if (window)
                glfwDestroyWindow(window);
            glfwTerminate();
        }
    };

    struct GuiState {
        std::array<char, PathBufferSize> wsdl_path{};
        std::array<char, PathBufferSize> output_dir{};
        std::array<char, 128> service_name{};
        std::string log = "Ready.";
        bool show_close_confirm = false;
    };

    void DrawPathInput(WsdlCppGen::UiSpriteAtlas &sprites, const char *label, std::array<char, PathBufferSize> &buffer,
                       const char *button, WsdlCppGen::SpriteIcon icon, float y, const ImVec2 &content_min,
                       const ImVec2 &content_max, float scale,
                       const std::function<WsdlCppGen::DialogResult()> &select_path, std::string &log,
                       bool &has_error) {
        const float label_width = 104.0f * scale;
        const float button_width = 116.0f * scale;
        const float icon_size = 16.0f * scale;
        const float gap = 18.0f * scale;
        const float frame_height = ImGui::GetFrameHeight();
        const float input_x = content_min.x + label_width;
        const float button_x = content_max.x - button_width;
        const float input_width = button_x - gap - input_x;

        ImGui::AlignTextToFramePadding();
        ImGui::SetCursorScreenPos(ImVec2(content_min.x, y));
        ImGui::TextUnformatted(label);

        ImGui::SetCursorScreenPos(ImVec2(input_x, y));
        WsdlCppGen::SpriteInputText(sprites, (std::string("##") + label).c_str(), buffer.data(), buffer.size(),
                                    input_width);

        ImGui::SetCursorScreenPos(ImVec2(button_x, y));
        if (WsdlCppGen::SpriteButton(sprites, button, ImVec2(button_width, frame_height),
                                     WsdlCppGen::SpriteButtonKind::Secondary, icon, icon_size)) {
            WsdlCppGen::DialogResult result = select_path();
            if (result.path) {
                CopyToBuffer(buffer, *result.path);
                log = "Selected: " + *result.path;
                has_error = false;
            } else if (!result.error.empty()) {
                log = result.error;
                has_error = true;
            }
        }
    }

    void DrawCloseConfirm(WsdlCppGen::UiSpriteAtlas &sprites, GuiState &state, GLFWwindow *window, float scale) {
        if (!state.show_close_confirm)
            return;

        constexpr const char *PopupName = "CloseConfirm";
        ImGui::OpenPopup(PopupName);

        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const ImVec2 popup_size = ImVec2(380.0f * scale, 174.0f * scale);
        ImGui::SetNextWindowPos(ImVec2((display.x - popup_size.x) * 0.5f, (display.y - popup_size.y) * 0.5f),
                                ImGuiCond_Always);
        ImGui::SetNextWindowSize(popup_size, ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        if (ImGui::BeginPopupModal(PopupName, nullptr,
                                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings)) {
            const ImVec2 popup_min = ImGui::GetWindowPos();
            const ImVec2 popup_max = ImVec2(popup_min.x + popup_size.x, popup_min.y + popup_size.y);
            const float pad = 28.0f * scale;
            const float button_width = 116.0f * scale;
            const float button_height = 44.0f * scale;
            const float gap = 18.0f * scale;

            WsdlCppGen::DrawSprite9(sprites, WsdlCppGen::SpriteFrame::PanelAlt, popup_min, popup_max);

            ImGui::SetCursorScreenPos(ImVec2(popup_min.x + pad, popup_min.y + pad));
            ImGui::TextUnformatted("Close application?");

            const float button_y = popup_max.y - pad - button_height;
            const float close_x = popup_max.x - pad - button_width;
            const float cancel_x = close_x - gap - button_width;

            ImGui::SetCursorScreenPos(ImVec2(cancel_x, button_y));
            if (WsdlCppGen::SpriteButton(sprites, "Cancel", ImVec2(button_width, button_height),
                                         WsdlCppGen::SpriteButtonKind::Secondary, WsdlCppGen::SpriteIcon::None,
                                         16.0f * scale)) {
                state.show_close_confirm = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SetCursorScreenPos(ImVec2(close_x, button_y));
            if (WsdlCppGen::SpriteButton(sprites, "Close", ImVec2(button_width, button_height),
                                         WsdlCppGen::SpriteButtonKind::Primary, WsdlCppGen::SpriteIcon::None,
                                         16.0f * scale)) {
                state.show_close_confirm = false;
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    void DrawGeneratorUi(WsdlCppGen::UiSpriteAtlas &sprites, GuiState &state, GLFWwindow *window) {
        static bool has_error = false;
        constexpr float BaseWidth = 1120.0f;
        constexpr float BaseHeight = 720.0f;

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
        ImGui::Begin("WSDL C++ Proxy Generator", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        const ImVec2 window_pos = ImGui::GetWindowPos();
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const float scale_x = display.x / BaseWidth;
        const float scale_y = display.y / BaseHeight;
        const float scale = std::clamp(std::min(scale_x, scale_y), 0.75f, 2.25f);
        const float outer_margin = 12.0f * scale;
        const float content_padding = 48.0f * scale;
        const ImVec2 background_min = ImVec2(window_pos.x + outer_margin, window_pos.y + outer_margin);
        const ImVec2 background_max = ImVec2(window_pos.x + display.x - outer_margin,
                                             window_pos.y + display.y - outer_margin);
        const ImVec2 content_min = ImVec2(background_min.x + content_padding, background_min.y + content_padding);
        const ImVec2 content_max = ImVec2(background_max.x - content_padding, background_max.y - content_padding);

        WsdlCppGen::DrawSprite9(sprites, WsdlCppGen::SpriteFrame::Panel, background_min, background_max);
        ImGui::PushFont(nullptr, 20.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f * scale, 10.0f * scale));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14.0f * scale, 14.0f * scale));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(10.0f * scale, 8.0f * scale));

        const float row_gap = 18.0f * scale;
        const float label_width = 104.0f * scale;
        const float button_gap = 18.0f * scale;
        const float frame_height = ImGui::GetFrameHeight();
        float y = content_min.y;

        DrawPathInput(sprites, "WSDL", state.wsdl_path, "Browse", WsdlCppGen::SpriteIcon::File, y, content_min,
                      content_max, scale, WsdlCppGen::SelectWsdlFile, state.log, has_error);
        y += frame_height + row_gap;
        DrawPathInput(sprites, "Output", state.output_dir, "Choose", WsdlCppGen::SpriteIcon::Folder, y, content_min,
                      content_max, scale, WsdlCppGen::SelectOutputDirectory, state.log, has_error);
        y += frame_height + row_gap;

        ImGui::SetCursorScreenPos(ImVec2(content_min.x, y));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Service");
        ImGui::SetCursorScreenPos(ImVec2(content_min.x + label_width, y));
        WsdlCppGen::SpriteInputText(sprites, "##ServiceName", state.service_name.data(), state.service_name.size(),
                                    content_max.x - content_min.x - label_width);

        y += frame_height + 28.0f * scale;
        const float button_height = 44.0f * scale;
        const float icon_size = 16.0f * scale;
        ImGui::SetCursorScreenPos(ImVec2(content_min.x, y));
        const bool generate = WsdlCppGen::SpriteButton(sprites, "Generate", ImVec2(190.0f * scale, button_height),
                                                       WsdlCppGen::SpriteButtonKind::Primary,
                                                       WsdlCppGen::SpriteIcon::Generate, icon_size);
        ImGui::SetCursorScreenPos(ImVec2(content_min.x + 190.0f * scale + button_gap, y));
        if (WsdlCppGen::SpriteButton(sprites, "Open", ImVec2(132.0f * scale, button_height),
                                     WsdlCppGen::SpriteButtonKind::Secondary, WsdlCppGen::SpriteIcon::Open,
                                     icon_size)) {
            WsdlCppGen::DialogResult result = WsdlCppGen::OpenDirectoryInShell(state.output_dir.data());
            if (!result.error.empty()) {
                state.log = result.error;
                has_error = true;
            }
        }

        if (generate) {
            try {
                WsdlCppGen::GenerateOptions options;
                options.wsdl_path = state.wsdl_path.data();
                options.output_dir = state.output_dir.data();
                options.service_name_override = state.service_name.data();
                WsdlCppGen::GenerateResult result = WsdlCppGen::GenerateFromWsdlFile(options);
                state.log = "Generated:\n" + result.header_path.string() + "\n" + result.source_path.string();
                has_error = false;
            } catch (const std::exception &error) {
                state.log = std::string("Error: ") + error.what();
                has_error = true;
            }
        }

        const ImVec2 status_size = ImVec2(content_max.x - content_min.x, 104.0f * scale);
        const ImVec2 status_min = ImVec2(content_min.x, content_max.y - status_size.y);
        const ImVec2 status_max = ImVec2(status_min.x + status_size.x, status_min.y + status_size.y);
        WsdlCppGen::DrawSprite9(
            sprites, has_error ? WsdlCppGen::SpriteFrame::StatusError : WsdlCppGen::SpriteFrame::StatusOk, status_min,
            status_max);
        WsdlCppGen::DrawSpriteIcon(sprites, has_error ? WsdlCppGen::SpriteIcon::Error : WsdlCppGen::SpriteIcon::Ok,
                                   ImVec2(status_min.x + 14.0f * scale, status_min.y + 14.0f * scale), icon_size);
        ImGui::SetCursorScreenPos(ImVec2(status_min.x + 40.0f * scale, status_min.y + 13.0f * scale));
        ImGui::PushTextWrapPos(status_max.x - 14.0f * scale);
        ImGui::PushStyleColor(ImGuiCol_Text,
                              has_error ? ImVec4(0.68f, 0.25f, 0.21f, 1.0f) : ImVec4(0.30f, 0.50f, 0.30f, 1.0f));
        ImGui::TextWrapped("%s", state.log.c_str());
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
        ImGui::SetCursorScreenPos(status_min);
        ImGui::Dummy(status_size);
        DrawCloseConfirm(sprites, state, window, scale);
        ImGui::PopStyleVar(3);
        ImGui::PopFont();
        ImGui::End();
    }
}

int RunGuiApp(const char *executable_path) {
    VulkanApp app;
    try {
        app.InitWindow();
        app.InitVulkan();
        app.InitImGui(executable_path);

        GuiState state;
        glfwSetWindowUserPointer(app.window, &state);
        glfwSetWindowCloseCallback(app.window, [](GLFWwindow *window) {
            if (GuiState *gui_state = static_cast<GuiState *>(glfwGetWindowUserPointer(window)))
                gui_state->show_close_confirm = true;
            glfwSetWindowShouldClose(window, GLFW_FALSE);
        });

        while (!glfwWindowShouldClose(app.window)) {
            glfwPollEvents();
            app.ResizeIfNeeded();

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            DrawGeneratorUi(app.sprites, state, app.window);

            ImGui::Render();
            app.RenderFrame();
            app.PresentFrame();
        }

        app.Cleanup();
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "GUI error: " << error.what() << "\n";
        app.Cleanup();
        return 1;
    }
}
