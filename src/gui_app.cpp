#include "wsdl_codegen.hpp"
#include "native_dialogs.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <array>
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

namespace
{
constexpr size_t PathBufferSize = 512;

void CopyToBuffer(std::array<char, PathBufferSize>& buffer, const std::filesystem::path& path)
{
    std::string text = path.generic_string();
    std::snprintf(buffer.data(), buffer.size(), "%s", text.c_str());
}

std::filesystem::path FindFontPath(const char* executable_path)
{
    constexpr const char* font_name = "JetBrainsMono-Regular.ttf";
    std::vector<std::filesystem::path> candidates = {
        std::filesystem::path("font") / font_name,
        std::filesystem::path("..") / "font" / font_name,
        std::filesystem::path("..") / ".." / "font" / font_name,
        std::filesystem::path("cmake-build-debug") / "font" / font_name,
        std::filesystem::path("build") / "font" / font_name,
    };

    if (executable_path && *executable_path)
    {
        std::filesystem::path executable_dir = std::filesystem::absolute(executable_path).parent_path();
        candidates.push_back(executable_dir / "font" / font_name);
        candidates.push_back(executable_dir.parent_path() / "font" / font_name);
    }

    for (const std::filesystem::path& candidate : candidates)
        if (std::filesystem::exists(candidate))
            return candidate;

    throw std::runtime_error("JetBrains Mono font not found: font/JetBrainsMono-Regular.ttf");
}

void ApplyTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(22.0f, 22.0f);
    style.FramePadding = ImVec2(14.0f, 10.0f);
    style.ItemSpacing = ImVec2(14.0f, 14.0f);
    style.ItemInnerSpacing = ImVec2(10.0f, 8.0f);
    style.ScrollbarSize = 18.0f;
    style.WindowRounding = 0.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 7.0f;
    style.PopupRounding = 8.0f;
    style.GrabRounding = 7.0f;
    style.TabRounding = 7.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.94f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.42f, 0.48f, 0.56f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.070f, 0.090f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.075f, 0.095f, 0.120f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.070f, 0.090f, 0.115f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.28f, 0.34f, 0.65f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.105f, 0.135f, 0.170f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.145f, 0.205f, 0.260f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.180f, 0.270f, 0.340f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.060f, 0.075f, 0.095f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.070f, 0.095f, 0.125f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.095f, 0.300f, 0.400f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.115f, 0.390f, 0.510f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.070f, 0.470f, 0.620f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.115f, 0.230f, 0.300f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.140f, 0.320f, 0.410f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.130f, 0.420f, 0.540f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.22f, 0.78f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.18f, 0.62f, 0.72f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.25f, 0.31f, 1.00f);
}

void SetWindowIcon(GLFWwindow* window)
{
    std::array<unsigned char, 32 * 32 * 4> pixels{};
    for (int y = 0; y < 32; ++y)
    {
        for (int x = 0; x < 32; ++x)
        {
            const int offset = (y * 32 + x) * 4;
            const bool border = x < 2 || y < 2 || x >= 30 || y >= 30;
            const bool arrow = (x > 9 && x < 23 && y > 8 && y < 13) || (x > 18 && x < 24 && y > 6 && y < 16);
            const bool bracket = (x >= 7 && x <= 9 && y >= 7 && y <= 24) || (x >= 23 && x <= 25 && y >= 7 && y <= 24);
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

void CheckVk(VkResult result)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error("Vulkan error: " + std::to_string(result));
}

struct VulkanApp
{
    GLFWwindow* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t queue_family = 0;
    VkQueue queue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    ImGui_ImplVulkanH_Window main_window;
    uint32_t min_image_count = 2;

    void InitWindow()
    {
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

    void InitVulkan()
    {
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "WsdlCppGen";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "WsdlCppGen";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_2;

        uint32_t extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);
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

        const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
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
        const VkFormat request_formats[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
        main_window.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
            physical_device,
            surface,
            request_formats,
            static_cast<int>(std::size(request_formats)),
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
        const VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
        main_window.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(physical_device, surface, present_modes, 1);
        min_image_count = static_cast<uint32_t>(ImGui_ImplVulkanH_GetMinImageCountFromPresentMode(main_window.PresentMode));

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physical_device, device, &main_window, queue_family, nullptr, width, height, min_image_count, 0);
    }

    void InitImGui(const char* executable_path)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
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
    }

    void ResizeIfNeeded()
    {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width <= 0 || height <= 0)
            return;
        if (main_window.Width == width && main_window.Height == height)
            return;
        vkDeviceWaitIdle(device);
        ImGui_ImplVulkan_SetMinImageCount(min_image_count);
        ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physical_device, device, &main_window, queue_family, nullptr, width, height, min_image_count, 0);
    }

    void RenderFrame()
    {
        ImGui_ImplVulkanH_Window* wd = &main_window;
        VkSemaphore image_acquired = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
        VkSemaphore render_complete = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        VkResult result = vkAcquireNextImageKHR(device, wd->Swapchain, UINT64_MAX, image_acquired, VK_NULL_HANDLE, &wd->FrameIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
            return;
        CheckVk(result);

        ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
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

    void PresentFrame()
    {
        ImGui_ImplVulkanH_Window* wd = &main_window;
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

    void Cleanup()
    {
        if (device != VK_NULL_HANDLE)
            vkDeviceWaitIdle(device);
        ImGui_ImplVulkan_Shutdown();
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

struct GuiState
{
    std::array<char, PathBufferSize> wsdl_path{};
    std::array<char, PathBufferSize> output_dir{};
    std::array<char, 128> service_name{};
    std::string log = "Ready.";
};

void DrawPathInput(const char* label, std::array<char, PathBufferSize>& buffer, const char* button, const std::function<WsdlCppGen::DialogResult()>& select_path, std::string& log, bool& has_error)
{
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-140.0f);
    ImGui::InputText((std::string("##") + label).c_str(), buffer.data(), buffer.size());
    ImGui::SameLine();
    if (ImGui::Button(button, ImVec2(132.0f, 0.0f)))
    {
        WsdlCppGen::DialogResult result = select_path();
        if (result.path)
        {
            CopyToBuffer(buffer, *result.path);
            log = "Selected: " + *result.path;
            has_error = false;
        }
        else if (!result.error.empty())
        {
            log = result.error;
            has_error = true;
        }
    }
}

void DrawGeneratorUi(GuiState& state)
{
    static bool has_error = false;
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    ImGui::Begin("WSDL C++ Proxy Generator", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 panel_min = ImGui::GetCursorScreenPos();
    const ImVec2 panel_max = ImVec2(panel_min.x + ImGui::GetContentRegionAvail().x, panel_min.y + 112.0f);
    draw_list->AddRectFilled(panel_min, panel_max, IM_COL32(18, 38, 48, 255), 10.0f);
    draw_list->AddRectFilled(ImVec2(panel_min.x, panel_min.y), ImVec2(panel_min.x + 8.0f, panel_max.y), IM_COL32(54, 204, 220, 255), 10.0f);
    ImGui::SetCursorScreenPos(ImVec2(panel_min.x + 26.0f, panel_min.y + 20.0f));
    ImGui::TextUnformatted("WsdlCppGen");
    ImGui::SetCursorScreenPos(ImVec2(panel_min.x + 26.0f, panel_min.y + 56.0f));
    ImGui::TextDisabled("WSDL to Visual Studio C++ SOAP proxy generator");
    ImGui::SetCursorScreenPos(ImVec2(panel_min.x, panel_max.y + 24.0f));

    const float left_width = 650.0f;
    ImGui::BeginChild("ConfigPanel", ImVec2(left_width, 0.0f), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::TextUnformatted("Generation inputs");
    ImGui::Separator();
    DrawPathInput("WSDL file", state.wsdl_path, "Browse", WsdlCppGen::SelectWsdlFile, state.log, has_error);
    DrawPathInput("Output directory", state.output_dir, "Choose", WsdlCppGen::SelectOutputDirectory, state.log, has_error);
    ImGui::TextUnformatted("Service name override");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##ServiceName", state.service_name.data(), state.service_name.size());

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.05f, 0.48f, 0.62f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.08f, 0.60f, 0.76f, 1.0f));
    const bool generate = ImGui::Button("Generate proxy", ImVec2(220.0f, 52.0f));
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
    if (ImGui::Button("Use sample", ImVec2(150.0f, 52.0f)))
    {
        CopyToBuffer(state.wsdl_path, "samples/AddService.wsdl");
        CopyToBuffer(state.output_dir, "generated");
        state.log = "Sample paths loaded.";
        has_error = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Open output", ImVec2(160.0f, 52.0f)))
    {
        WsdlCppGen::DialogResult result = WsdlCppGen::OpenDirectoryInShell(state.output_dir.data());
        if (!result.error.empty())
        {
            state.log = result.error;
            has_error = true;
        }
    }

    ImGui::Spacing();
    if (generate)
    {
        try
        {
            WsdlCppGen::GenerateOptions options;
            options.wsdl_path = state.wsdl_path.data();
            options.output_dir = state.output_dir.data();
            options.service_name_override = state.service_name.data();
            WsdlCppGen::GenerateResult result = WsdlCppGen::GenerateFromWsdlFile(options);
            state.log = "Generated:\n" + result.header_path.string() + "\n" + result.source_path.string();
            has_error = false;
        }
        catch (const std::exception& error)
        {
            state.log = std::string("Error: ") + error.what();
            has_error = true;
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("StatusPanel", ImVec2(0.0f, 0.0f), true);
    ImGui::TextUnformatted("Runbook");
    ImGui::Separator();
    ImGui::BulletText("Select the assignment WSDL file.");
    ImGui::BulletText("Choose the output directory.");
    ImGui::BulletText("Generate Visual Studio-friendly .h/.cpp proxy files.");
    ImGui::BulletText("Run the Go AddService and call it from the generated client.");
    ImGui::Spacing();

    ImGui::TextDisabled("Status");
    ImGui::PushStyleColor(ImGuiCol_Text, has_error ? ImVec4(1.0f, 0.42f, 0.35f, 1.0f) : ImVec4(0.50f, 0.90f, 0.72f, 1.0f));
    ImGui::TextWrapped("%s", state.log.c_str());
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::End();
}
}

int RunGuiApp(const char* executable_path)
{
    VulkanApp app;
    try
    {
        app.InitWindow();
        app.InitVulkan();
        app.InitImGui(executable_path);

        GuiState state;
        std::strncpy(state.wsdl_path.data(), "samples/AddService.wsdl", state.wsdl_path.size() - 1);
        std::strncpy(state.output_dir.data(), "generated", state.output_dir.size() - 1);

        while (!glfwWindowShouldClose(app.window))
        {
            glfwPollEvents();
            app.ResizeIfNeeded();

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            DrawGeneratorUi(state);

            ImGui::Render();
            app.RenderFrame();
            app.PresentFrame();
        }

        app.Cleanup();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "GUI error: " << error.what() << "\n";
        app.Cleanup();
        return 1;
    }
}
