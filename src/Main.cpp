module Main;

import <vulkan/vk_platform.h>;

bool QueueFamilyIndices::IsComplete() const {
    return GraphicsFamily.has_value();
}


VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice) {
    QueueFamilyIndices indices;

    auto queueFamilies = physicalDevice.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        const auto& queueFamily = queueFamilies[i];

        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.GraphicsFamily = i;
        }

        if (indices.GraphicsFamily.has_value()) {
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
    PickPhysicalDevice();
    CreateLogicalDevice();
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


bool IsDeviceSuitable(const vk::raii::PhysicalDevice &device) {
    auto queueFamilies = FindQueueFamilies(device);
    if (!queueFamilies.IsComplete()) {
        return false; // No graphics queue family found
    }
    return true;
}

void HelloTriangleApplication::PickPhysicalDevice() {
    auto physicalDevices = m_VkInstance.enumeratePhysicalDevices();
    if (physicalDevices.empty()) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    for (const auto& device : physicalDevices) {
        if (device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            m_PhysicalDevice = device;
            std::cout << "Physical device selected: " << device.getProperties().deviceName << std::endl;
            return;
        }
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

    vk::DeviceQueueCreateInfo queueCreateInfo{
        .pNext = nullptr,
        .flags = {},
        .queueFamilyIndex = queueFamilies.GraphicsFamily.value(),
        .queueCount = 1,
        .pQueuePriorities = queuePriorities
    };

    vk::PhysicalDeviceFeatures deviceFeatures{};

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = nullptr,
        .flags = {},
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledLayerCount = enableValidationLayers ? static_cast<uint32_t>(s_ValidationLayers.size()) : 0,
        .ppEnabledLayerNames = enableValidationLayers ? s_ValidationLayers.data() : nullptr,
        .enabledExtensionCount = 0, // No extensions for now
        .ppEnabledExtensionNames = nullptr,
        .pEnabledFeatures = &deviceFeatures
    };

    m_Device = m_PhysicalDevice.createDevice(deviceCreateInfo);
    m_GraphicsQueue = m_Device.getQueue(queueFamilies.GraphicsFamily.value(), 0);
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
