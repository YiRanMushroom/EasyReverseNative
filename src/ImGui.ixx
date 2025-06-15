export module ImGui;

export import "imgui.h";
export import "backends/imgui_impl_glfw.h";
export import "backends/imgui_impl_vulkan.h";
import "vulkan/vulkan.hpp";

vk::DescriptorPool g_ImDescriptorPool{nullptr};
vk::Device g_ImguiLogicalDevice{nullptr};

void InitImGuiDescriptorPool(vk::Device device) {
    vk::DescriptorPoolSize poolSize{
        .type = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 10
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 10,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };

    g_ImDescriptorPool = device.createDescriptorPool(poolInfo).value;
}

void DestroyImGuiDescriptorPool(vk::Device device) {
    if (g_ImDescriptorPool) {
        device.destroyDescriptorPool(g_ImDescriptorPool);
        g_ImDescriptorPool = nullptr;
    }
}

export void InitImGuiForMyProgram(uint32_t apiVersion,
    vk::Instance instance, vk::PhysicalDevice physicalDevice,
    vk::Device device, uint32_t queueFamily, vk::Queue queue,
    vk::RenderPass renderPass, uint32_t minImageCount, uint32_t imageCount) {
    InitImGuiDescriptorPool(device);
    g_ImguiLogicalDevice = device;

    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = apiVersion;
    info.Instance = instance;
    info.PhysicalDevice = physicalDevice;
    info.Device = device;
    info.QueueFamily = queueFamily;
    info.Queue = queue;
    info.DescriptorPool = g_ImDescriptorPool;
    info.RenderPass = renderPass;
    info.MinImageCount = minImageCount;
    info.ImageCount = imageCount;

    ImGui_ImplVulkan_Init(&info);
}

export void ShutdownImGuiForMyProgram() {
    ImGui_ImplVulkan_Shutdown();
    DestroyImGuiDescriptorPool(g_ImguiLogicalDevice);
    g_ImguiLogicalDevice = nullptr;
}