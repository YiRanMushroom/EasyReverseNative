export module BasicContext;

import vulkan_hpp;
import glfwpp;

import std;
import <set>;
import ImGui;
import <memory>;

import Event;

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

export class IUpdatableLayer {
public:
    virtual ~IUpdatableLayer() = default;

    virtual void OnUpdate() = 0;

    virtual void OnSubmitCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<std::any> &dependentContexts) = 0;

    virtual bool OnEvent(const Event* event) = 0;
};

export class IBasicContext {
public:
    virtual ~IBasicContext() = default;

public:
    virtual const glfw::GlfwLibrary &GetGlfwLibrary() = 0;

    virtual glfw::Window &GetWindow() = 0;

    virtual const vk::raii::Context &GetRaiiContext() = 0;

    virtual vk::raii::Instance &GetVulkanInstance() = 0;

    virtual vk::raii::PhysicalDevice &GetPhysicalDevice() = 0;

    virtual vk::raii::Device &GetLogicalDevice() = 0;

    virtual vk::raii::SurfaceKHR &GetSurface() = 0;

    virtual vk::raii::Queue &GetGraphicsQueue() = 0;

    virtual vk::raii::Queue &GetPresentQueue() = 0;

    virtual vk::raii::RenderPass &GetRenderPass() = 0;

    virtual vk::raii::CommandPool& GetCommandPool() = 0;

    virtual vk::raii::Sampler &GetSampler() = 0;

    virtual void SetClearColor(const vk::ClearColorValue &clearColor) = 0;

    virtual void RecreateSwapChain() = 0;

    template<std::derived_from<IUpdatableLayer> T>
    std::shared_ptr<T> EmplaceLayer(auto &&... args) {
        auto layer = std::make_shared<T>(std::forward<decltype(args)>(args)...);
        PushLayer(layer);
        return layer;
    }

public:
    virtual void MainLoop() = 0;

    virtual void Cleanup() = 0;

public:
    virtual void PushLayer(std::shared_ptr<IUpdatableLayer> layer) = 0;

    virtual void PopLayer(std::shared_ptr<IUpdatableLayer> layer) = 0;
};

export struct WindowSpec {
    std::string title{};
    int width = 1920;
    int height = 1080;
};

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

class BasicContextImpl : public IBasicContext {
public:
    explicit BasicContextImpl(const WindowSpec &windowSpec);

private:
    void InitializeWindow(const WindowSpec &windowSpec);

    void InitVulkan();

    void CreateInstance();

    static bool CheckValidationLayerSupport();

    void SetupDebugMessenger();

    static std::vector<const char *> GetRequiredExtensions();

    static vk::DebugUtilsMessengerCreateInfoEXT PopulateDebugMessengerCreateInfo();

    void PickPhysicalDevice();

    void CreateLogicalDevice();

    void CreateSurface();

    QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice);

    bool IsDeviceSuitable(const vk::raii::PhysicalDevice &device);

    [[nodiscard]] bool CheckDeviceExtensionSupport(vk::PhysicalDevice device) const;

    SwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice device);

    [[nodiscard]] vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR> &availableFormats) const;

    [[nodiscard]] vk::PresentModeKHR ChooseSwapPresentMode(
        const std::vector<vk::PresentModeKHR> &availablePresentModes) const;

    [[nodiscard]] vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const;

    void CreateSwapChain();

    void CreateImageViews();

    void CreateSampler();

    void InitImGui();

    void BeginImGuiFrame();

    void CreateRenderPass();

    void CreateFramebuffers();

    void CreateCommandPool();

    void CreateCommandBuffer();

    void CreateSyncObjects();

    void DrawFrame();

    void RecreateSwapChain() override;

    void CleanupSwapChain();

    void MainLoop() override;

    void Cleanup() override;

    void OnUpdate();

    glfw::GlfwLibrary m_GlfwLibrary;
    vk::raii::Context m_Context;
    vk::raii::Instance m_Instance{nullptr};

    std::optional<glfw::Window> m_Window{};
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

    vk::raii::Sampler m_Sampler{nullptr};

    vk::raii::RenderPass m_RenderPass{nullptr};
    std::vector<vk::raii::Framebuffer> m_SwapChainFramebuffers;
    vk::raii::CommandPool m_CommandPool{nullptr};

    std::vector<vk::raii::Semaphore> m_ImageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphores;
    std::vector<vk::raii::Fence> m_InFlightFences;
    size_t m_CurrentFrame = 0;
    std::vector<vk::raii::CommandBuffer> m_CommandBuffers;
    bool m_ShouldUpdate = true;

    std::vector<std::vector<std::any>> m_CommandBufferDependentContexts;

    std::vector<std::shared_ptr<IUpdatableLayer>> m_Layers;

private:
    size_t m_MinImageCount = 0;
    size_t m_ImageCount = 0;
    vk::ClearColorValue m_ClearColor = vk::ClearColorValue(std::array<float, 4>{.8f, .2f, 0.8f, 1.0f});

private:
    const inline static std::vector<const char *> s_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const inline static std::vector<const char *> s_DeviceExtensions = {
        vk::KHRSwapchainExtensionName
    };

public:
    // override all IBasicContext methods
    const glfw::GlfwLibrary &GetGlfwLibrary() override { return m_GlfwLibrary; }
    glfw::Window &GetWindow() override { return *m_Window; }
    const vk::raii::Context &GetRaiiContext() override { return m_Context; }
    vk::raii::Instance &GetVulkanInstance() override { return m_Instance; }
    vk::raii::PhysicalDevice &GetPhysicalDevice() override { return m_PhysicalDevice; }
    vk::raii::Device &GetLogicalDevice() override { return m_Device; }
    vk::raii::SurfaceKHR &GetSurface() override { return m_Surface; }
    vk::raii::Queue &GetGraphicsQueue() override { return m_GraphicsQueue; }
    vk::raii::Queue &GetPresentQueue() override { return m_PresentQueue; }
    vk::raii::RenderPass &GetRenderPass() override { return m_RenderPass; }
    vk::raii::CommandPool &GetCommandPool() override { return m_CommandPool; }
    vk::raii::Sampler &GetSampler() override { return m_Sampler; }
    void SetClearColor(const vk::ClearColorValue &clearColor) override { m_ClearColor = clearColor; }

    void PushLayer(std::shared_ptr<IUpdatableLayer> layer) override {
        m_Layers.push_back(layer);
    }

    void PopLayer(std::shared_ptr<IUpdatableLayer> layer) override {
        auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
        if (it != m_Layers.end()) {
            m_Layers.erase(it);
        }
    }
};

export std::shared_ptr<IBasicContext> CreateBasicContext(const WindowSpec &windowSpec) {
    return std::make_shared<BasicContextImpl>(windowSpec);
}
