export module Main;

import std;
import vulkan_hpp;
import glfwpp;
import <glm/glm.hpp>;
import <vulkan/vk_platform.h>;
import ImGui;

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

vk::raii::Context g_VkRaiiContext{};
vk::raii::Instance g_VkInstance{nullptr};
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;


struct QueueFamilyIndices {
    std::optional<uint32_t> GraphicsFamily;
    std::optional<uint32_t> PresentFamily;

    [[nodiscard]] bool IsComplete() const;
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR Capabilities;
    std::vector<vk::SurfaceFormatKHR> Formats;
    std::vector<vk::PresentModeKHR> PresentModes;
};

class HelloTriangleApplication;

class ImGuiImageRenderTarget {
    friend class HelloTriangleApplication;
public:
    ImGuiImageRenderTarget(HelloTriangleApplication* app);

public:
    void Rebuild();

    void Flush();
    void FlushAndWait();

    vk::Fence GetFence() const;

    vk::ClearValue m_ClearColor{
        vk::ClearColorValue(std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f})
    };

    void RenderImGui();
private:
    void CreateImageAndView();
    void CreateSampler();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateFramebuffer();
    void CreateSyncObjects();
    void CreateCommandBuffer();
    void CreateDescriptorSetLayout();
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void RecordCommandBuffer();

    HelloTriangleApplication* m_App{nullptr};

    vk::raii::Image m_Image{nullptr};
    vk::raii::DeviceMemory m_ImageMemory{nullptr};
    vk::raii::ImageView m_ImageView{nullptr};
    vk::raii::Pipeline m_GraphicsPipeline{nullptr};
    vk::raii::PipelineLayout m_PipelineLayout{nullptr};
    vk::raii::RenderPass m_RenderPass{nullptr};
    vk::raii::Framebuffer m_Framebuffer{nullptr};
    vk::raii::Fence m_RenderFinishedFence{nullptr};
    vk::raii::CommandBuffer m_CommandBuffer{nullptr};

    vk::raii::Sampler m_Sampler{nullptr};

    vk::DescriptorSet m_ImageDescriptorSet{nullptr};
    VkDescriptorSet m_SetHandler;

    vk::Rect2D m_RenderArea{
        .offset = vk::Offset2D{0, 0},
        .extent = vk::Extent2D{960, 640} // Set your desired width and height
    };

    bool m_NeedsRebuild = false;
};

class HelloTriangleApplication {
public:
    HelloTriangleApplication();
    void MainLoop();
    void Cleanup();

    friend class ImGuiImageRenderTarget;

private:
    void InitializeWindow();
    void InitVulkan();
    void CreateInstance();
    static bool CheckValidationLayerSupport();
    void SetupDebugMessenger();
    static std::vector<const char*> GetRequiredExtensions();
    static vk::DebugUtilsMessengerCreateInfoEXT PopulateDebugMessengerCreateInfo();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSurface();
    QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice);
    bool IsDeviceSuitable(const vk::raii::PhysicalDevice &device);
    [[nodiscard]] bool CheckDeviceExtensionSupport(vk::PhysicalDevice device) const;
    SwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice device);
    [[nodiscard]] vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
    [[nodiscard]] vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const;
    [[nodiscard]] vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
    void CreateSwapChain();
    void CreateImageViews();

    void InitImGui();
    void DrawImGui();
    void BeginImGuiFrame();
    void CreateImGuiRenderTarget();

    // Graphics
    void CreateGraphicsPipeline();
    vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& code);
    void CreateRenderPass();
    void CreateFramebuffers();

    // Commands
    void CreateCommandPool();
    void CreateCommandBuffer();
    void CreateSyncObjects();
    void RecordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

    void DrawFrame();

    // Swap chain recreation
    void RecreateSwapChain();
    void CleanupSwapChain();

    std::optional<glfw::Window> m_Window;
    vk::raii::SurfaceKHR m_Surface{nullptr};
    vk::raii::DebugUtilsMessengerEXT m_DebugMessenger{nullptr};
    vk::raii::PhysicalDevice m_PhysicalDevice{nullptr};
    vk::raii::Device m_Device{nullptr};
    vk::raii::Queue m_GraphicsQueue{nullptr};
    vk::raii::Queue m_PresentQueue{nullptr};
    vk::raii::SwapchainKHR m_SwapChain{nullptr};

    std::vector<vk::Image> m_SwapChainImages;
    vk::Format m_SwapChainImageFormat;
    vk::Extent2D m_SwapChainExtent;
    std::vector<vk::raii::ImageView> m_SwapChainImageViews;

    vk::raii::RenderPass m_RenderPass{nullptr};
    vk::raii::Pipeline m_GraphicsPipeline{nullptr};
    vk::raii::PipelineLayout m_PipelineLayout{nullptr};
    std::vector<vk::raii::Framebuffer> m_SwapChainFramebuffers;

    vk::raii::CommandPool m_CommandPool{nullptr};

    // synchronization objects
    // vk::raii::Semaphore m_ImageAvailableSemaphore{nullptr};
    // vk::raii::Semaphore m_RenderFinishedSemaphore{nullptr};
    // vk::raii::Fence m_InFlightFence{nullptr};
    std::vector<vk::raii::Semaphore> m_ImageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphores;
    std::vector<vk::raii::Fence> m_InFlightFences;
    // std::vector<vk::Fence> m_ImagesInFlight;
    size_t m_CurrentFrame = 0;
    std::vector<vk::raii::CommandBuffer> m_CommandBuffers;
    bool m_ShouldUpdate = true;

    std::unique_ptr<ImGuiImageRenderTarget> m_ImGuiImageRenderTarget;

private:
    const inline static std::vector<const char*> s_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const inline static std::vector<const char*> s_DeviceExtensions = {
        vk::KHRSwapchainExtensionName
    };

};


export int main() {
    auto glfwLib = glfw::init();

    HelloTriangleApplication app{};
    app.MainLoop();
    app.Cleanup();

    return 0;
}