export module Main;

import std;
import vulkan_hpp;
import glfwpp;
import <glm/glm.hpp>;
import <vulkan/vk_platform.h>;

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

vk::raii::Context g_VkRaiiContext{};

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


class HelloTriangleApplication {
public:
    HelloTriangleApplication();
    void MainLoop();
    void Cleanup();

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

    std::optional<glfw::Window> m_Window;
    vk::raii::Instance m_VkInstance{nullptr};
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
