#include "wsdl_codegen.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <array>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace
{
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
        window = glfwCreateWindow(900, 560, "WsdlCppGen", nullptr, nullptr);
        if (!window)
            throw std::runtime_error("glfwCreateWindow failed");
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
        if (!io.Fonts->AddFontFromFileTTF(font_path.string().c_str(), 16.0f))
            throw std::runtime_error("Failed to load font: " + font_path.string());

        ImGui::StyleColorsDark();
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
    std::array<char, 512> wsdl_path{};
    std::array<char, 512> output_dir{};
    std::array<char, 128> service_name{};
    std::string log = "Ready.";
};

void DrawGeneratorUi(GuiState& state)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    ImGui::Begin("WSDL C++ Proxy Generator", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::TextUnformatted("WSDL C++ Proxy Generator");
    ImGui::Separator();
    ImGui::InputText("WSDL file", state.wsdl_path.data(), state.wsdl_path.size());
    ImGui::InputText("Output dir", state.output_dir.data(), state.output_dir.size());
    ImGui::InputText("Service name override", state.service_name.data(), state.service_name.size());

    if (ImGui::Button("Generate", ImVec2(120, 0)))
    {
        try
        {
            WsdlCppGen::GenerateOptions options;
            options.wsdl_path = state.wsdl_path.data();
            options.output_dir = state.output_dir.data();
            options.service_name_override = state.service_name.data();
            WsdlCppGen::GenerateResult result = WsdlCppGen::GenerateFromWsdlFile(options);
            state.log = "Generated:\n" + result.header_path.string() + "\n" + result.source_path.string();
        }
        catch (const std::exception& error)
        {
            state.log = std::string("Error: ") + error.what();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Use sample paths", ImVec2(150, 0)))
    {
        std::strncpy(state.wsdl_path.data(), "samples/AddService.wsdl", state.wsdl_path.size() - 1);
        std::strncpy(state.output_dir.data(), "generated", state.output_dir.size() - 1);
    }

    ImGui::Separator();
    ImGui::TextWrapped("%s", state.log.c_str());
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
