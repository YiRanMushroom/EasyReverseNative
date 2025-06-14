module Main;

import <vulkan/vk_platform.h>;
import "vendor/glfwpp/native.h";

bool QueueFamilyIndices::IsComplete() const {
    return GraphicsFamily.has_value() && PresentFamily.has_value();
}


VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

QueueFamilyIndices HelloTriangleApplication::FindQueueFamilies(vk::PhysicalDevice physicalDevice) {
    QueueFamilyIndices indices;

    auto queueFamilies = physicalDevice.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        const auto& queueFamily = queueFamilies[i];

        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.GraphicsFamily = i;
        }

        auto presentSupport = physicalDevice.getSurfaceSupportKHR(i, *m_Surface);

        if (presentSupport) {
            indices.PresentFamily = i;
        }

        if (indices.IsComplete()) {
            break; // We found the graphics family, no need to continue
        }
    }

    return indices;
}

HelloTriangleApplication::HelloTriangleApplication() {
    InitializeWindow();
    InitVulkan();
}

void HelloTriangleApplication::MainLoop() {
    while (!m_Window->shouldClose()) {
        glfw::pollEvents();
    }
}

void HelloTriangleApplication::Cleanup() {}

void HelloTriangleApplication::InitializeWindow() {
    glfw::WindowHints{
        .resizable = false,
        .clientApi = glfw::ClientApi::None,
    }.apply();

    m_Window = glfw::Window{1920, 1080, "Vulkan", nullptr, nullptr};
}

void HelloTriangleApplication::InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
}

void HelloTriangleApplication::CreateInstance() {
    if (enableValidationLayers && !CheckValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    vk::ApplicationInfo appInfo{
        .sType = vk::StructureType::eApplicationInfo,
        .pNext = nullptr,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = vk::makeVersion(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = vk::makeVersion(1, 0, 0),
        .apiVersion = vk::ApiVersion10
    };

    auto extensions = GetRequiredExtensions();

    auto debugCreateInfo = PopulateDebugMessengerCreateInfo();

    // flags,
    vk::InstanceCreateInfo createInfo{
        .sType = vk::StructureType::eInstanceCreateInfo,
        .pNext = enableValidationLayers ? static_cast<const void *>(&debugCreateInfo) : nullptr,
        .flags = vk::InstanceCreateFlags{},
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = enableValidationLayers ? static_cast<uint32_t>(s_ValidationLayers.size()) : 0,
        .ppEnabledLayerNames = enableValidationLayers ? s_ValidationLayers.data() : nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    m_VkInstance = vk::raii::Instance(g_VkRaiiContext, createInfo);

    std::cout << "Vulkan instance created successfully." << std::endl;
}

bool HelloTriangleApplication::CheckValidationLayerSupport() {
    auto availableLayers = vk::enumerateInstanceLayerProperties();

    for (const auto& layerName : s_ValidationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (std::string_view(layerProperties.layerName) == layerName) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            std::cerr << "Validation layer " << layerName << " not found!" << std::endl;
            return false;
        }
    }

    std::cout << "Validation layers are supported." << std::endl;

    return true;
}

void HelloTriangleApplication::SetupDebugMessenger() {
    if constexpr (!enableValidationLayers) return;

    m_DebugMessenger = m_VkInstance.createDebugUtilsMessengerEXT(PopulateDebugMessengerCreateInfo());
}

std::vector<const char *> HelloTriangleApplication::GetRequiredExtensions() {
    auto glfwRequiredExtensions = glfw::getRequiredInstanceExtensions();
    if constexpr (enableValidationLayers) {
        glfwRequiredExtensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
    return glfwRequiredExtensions;
}

vk::DebugUtilsMessengerCreateInfoEXT HelloTriangleApplication::PopulateDebugMessengerCreateInfo() {
    return vk::DebugUtilsMessengerCreateInfoEXT{
        .sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT,
        .pNext = nullptr,
        .flags = vk::DebugUtilsMessengerCreateFlagsEXT{},
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = DebugCallback,
        .pUserData = nullptr
    };
}

bool HelloTriangleApplication::IsDeviceSuitable(const vk::raii::PhysicalDevice &device) {
    auto queueFamilies = FindQueueFamilies(device);

    bool extensionSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate = false;

    if (extensionSupported) {
        auto swapChainSupport = QuerySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
    }

    return queueFamilies.IsComplete() && extensionSupported && swapChainAdequate;
}

bool HelloTriangleApplication::CheckDeviceExtensionSupport(vk::PhysicalDevice device) const {
    auto availableExtensions = device.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(s_DeviceExtensions.begin(), s_DeviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    if (requiredExtensions.empty()) {
        std::cout << "All required device extensions are supported." << std::endl;
        return true;
    }
    std::cerr << "Not all required device extensions are supported!" << std::endl;
    for (const auto& ext : requiredExtensions) {
        std::cerr << "Missing extension: " << ext << std::endl;
    }
    return false;
}

SwapChainSupportDetails HelloTriangleApplication::QuerySwapChainSupport(vk::PhysicalDevice device) {
    SwapChainSupportDetails details;

    auto capabilities = device.getSurfaceCapabilitiesKHR(*m_Surface);
    auto formats = device.getSurfaceFormatsKHR(*m_Surface);
    auto presentModes = device.getSurfacePresentModesKHR(*m_Surface);

    return {
        .Capabilities = std::move(capabilities),
        .Formats = std::move(formats),
        .PresentModes = std::move(presentModes)
    };
}

vk::SurfaceFormatKHR HelloTriangleApplication::ChooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats) const {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats.front();
}

vk::PresentModeKHR HelloTriangleApplication::ChooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes) const {
    return vk::PresentModeKHR::eFifo; // FIFO is guaranteed to be supported
}

vk::Extent2D HelloTriangleApplication::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    auto [width, height] = m_Window->getFramebufferSize();

    vk::Extent2D actualExtent = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void HelloTriangleApplication::CreateSwapChain() {
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(*m_PhysicalDevice);

    auto surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
    auto presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
    auto extent = ChooseSwapExtent(swapChainSupport.Capabilities);

    uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
    if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount) {
        imageCount = swapChainSupport.Capabilities.maxImageCount;
    }

    auto queueFamilies = FindQueueFamilies(*m_PhysicalDevice);

    uint32_t queueFamilyIndices[] = {
        queueFamilies.GraphicsFamily.value(),
        queueFamilies.PresentFamily.value()
    };

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .pNext = nullptr,
        .flags = {},
        .surface = *m_Surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = queueFamilies.GraphicsFamily == queueFamilies.PresentFamily
            ? vk::SharingMode::eExclusive
            : vk::SharingMode::eConcurrent,
        .queueFamilyIndexCount = static_cast<uint32_t>((queueFamilies.GraphicsFamily == queueFamilies.PresentFamily) ? 0 : 2),
        .pQueueFamilyIndices = (queueFamilies.GraphicsFamily == queueFamilies.PresentFamily) ? nullptr : queueFamilyIndices,
        .preTransform = swapChainSupport.Capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque, // Opaque is a common choice
        .presentMode = presentMode,
        .clipped = vk::True,
        .oldSwapchain = m_SwapChain.release()
    };

    m_SwapChain = m_Device.createSwapchainKHR(swapChainCreateInfo);

    m_SwapChainImages = m_SwapChain.getImages();
    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent = extent;
}

void HelloTriangleApplication::CreateImageViews() {
    m_SwapChainImageViews.reserve(m_SwapChainImages.size());

    for (const auto& image : m_SwapChainImages) {
        vk::ImageViewCreateInfo viewInfo{
            .pNext = nullptr,
            .flags = {},
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = m_SwapChainImageFormat,
            .components = {
                .r = vk::ComponentSwizzle::eIdentity,
                .g = vk::ComponentSwizzle::eIdentity,
                .b = vk::ComponentSwizzle::eIdentity,
                .a = vk::ComponentSwizzle::eIdentity
            },
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        m_SwapChainImageViews.push_back(m_Device.createImageView(viewInfo));
    }
}

void HelloTriangleApplication::PickPhysicalDevice() {
    auto physicalDevices = m_VkInstance.enumeratePhysicalDevices();
    if (physicalDevices.empty()) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    for (const auto& device : physicalDevices) {
        if (IsDeviceSuitable(device)) {
            m_PhysicalDevice = device;
            std::cout << "Physical device selected: " << device.getProperties().deviceName << std::endl;
            return;
        }
    }
}

void HelloTriangleApplication::CreateLogicalDevice() {
    if (!*m_PhysicalDevice) {
        throw std::runtime_error("Physical device not selected.");
    }

    auto queueFamilies = FindQueueFamilies(m_PhysicalDevice);

    float queuePriorities[1] = {1.0f};

    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilies.GraphicsFamily.value(),
        queueFamilies.PresentFamily.value()
    };

    std::vector <vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueFamilies.size());
    for (const auto& queueFamily : uniqueQueueFamilies) {
        queueCreateInfos.push_back({
            .pNext = nullptr,
            .flags = {},
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = queuePriorities
        });
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = nullptr,
        .flags = {},
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = enableValidationLayers ? static_cast<uint32_t>(s_ValidationLayers.size()) : 0,
        .ppEnabledLayerNames = enableValidationLayers ? s_ValidationLayers.data() : nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(s_DeviceExtensions.size()),
        .ppEnabledExtensionNames = s_DeviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures
    };

    m_Device = m_PhysicalDevice.createDevice(deviceCreateInfo);
    m_GraphicsQueue = m_Device.getQueue(queueFamilies.GraphicsFamily.value(), 0);
    m_PresentQueue = m_Device.getQueue(queueFamilies.PresentFamily.value(), 0);
}

void HelloTriangleApplication::CreateSurface() {
    vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo {
        .pNext = nullptr,
        .flags = {},
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = glfw::native::getWin32Window(*m_Window)
    };

    m_Surface = m_VkInstance.createWin32SurfaceKHR(surfaceCreateInfo);
}


vk::Bool32 DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                         vk::DebugUtilsMessageTypeFlagsEXT messageType, const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                         void *pUserData) {

    if (messageSeverity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    } else {
        std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return vk::False;
}
