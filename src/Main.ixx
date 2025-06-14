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

    [[nodiscard]] bool IsComplete() const;
};

QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice);

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

    std::optional<glfw::Window> m_Window;
    vk::raii::Instance m_VkInstance{nullptr};
    vk::raii::DebugUtilsMessengerEXT m_DebugMessenger{nullptr};
    vk::raii::PhysicalDevice m_PhysicalDevice{nullptr};
    vk::raii::Device m_Device{nullptr};
    vk::raii::Queue m_GraphicsQueue{nullptr};

private:
    const inline static std::vector<const char*> s_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

};


export int main() {
    auto glfwLib = glfw::init();

    HelloTriangleApplication app{};
    app.MainLoop();
    app.Cleanup();

    return 0;
}
