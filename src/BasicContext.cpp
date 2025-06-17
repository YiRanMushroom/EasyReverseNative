module BasicContext;

import "vendor/glfwpp/native.h";
import Event.AllEvents;

BasicContextImpl::BasicContextImpl(const WindowSpec &windowSpec) : m_GlfwLibrary(glfw::init()),
                                                                   m_Context(vk::raii::Context{}) {
    InitializeWindow(windowSpec);
    InitVulkan();
}

void BasicContextImpl::InitializeWindow(const WindowSpec &windowSpec) {
    glfw::WindowHints{
        .resizable = true,
        .clientApi = glfw::ClientApi::None,
    }.apply();

    m_Window = glfw::Window{windowSpec.width, windowSpec.height, windowSpec.title.c_str()};

    m_Window->framebufferSizeEvent.setCallback([this](glfw::Window &window, int width, int height) {
        if (width > 0 && height > 0) {
            m_ShouldUpdate = true;
            RecreateSwapChain();
        } else {
            m_ShouldUpdate = false;
        }

        WindowResizeEvent event{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        for (auto reverseIt = m_Layers.rbegin(); reverseIt != m_Layers.rend(); ++reverseIt) {
            if ((*reverseIt)->OnEvent(&event)) {
                break;
            }
        }
    });


    m_Window->closeEvent.setCallback([this](glfw::Window &window) {
        WindowCloseEvent event{};
        for (auto reverseIt = m_Layers.rbegin(); reverseIt != m_Layers.rend(); ++reverseIt) {
            if ((*reverseIt)->OnEvent(&event)) {
                break;
            }
        }
    });

    m_Window->keyEvent.setCallback([this](glfw::Window &window, glfw::KeyCode key, int scanCode,
                                          glfw::KeyState action, glfw::ModifierKeyBit mods) {
        switch (action) {
            case glfw::KeyState::Press: {
                KeyPressedEvent event{Key::KeyCode(key), false};
                for (auto reverseIt = m_Layers.rbegin(); reverseIt != m_Layers.rend(); ++reverseIt) {
                    if ((*reverseIt)->OnEvent(&event)) {
                        break;
                    }
                }
                break;
            }
            case glfw::KeyState::Release: {
                KeyReleasedEvent event{Key::KeyCode(key)};
                for (auto reverseIt = m_Layers.rbegin(); reverseIt != m_Layers.rend(); ++reverseIt) {
                    if ((*reverseIt)->OnEvent(&event)) {
                        break;
                    }
                }
                break;
            }
            case glfw::KeyState::Repeat: {
                KeyPressedEvent event{Key::KeyCode(key), true};
                for (auto reverseIt = m_Layers.rbegin(); reverseIt != m_Layers.rend(); ++reverseIt) {
                    if ((*reverseIt)->OnEvent(&event)) {
                        break;
                    }
                }
                break;
            }
            default:
                std::cerr << "Unknown key action: " << static_cast<int>(action) << std::endl;
        }
    });

    m_Window->charEvent.setCallback([this](glfw::Window &window, unsigned int keyCode) {
        KeyTypedEvent event{keyCode};
        for (auto reverseIt = m_Layers.rbegin(); reverseIt != m_Layers.rend(); ++reverseIt) {
            if ((*reverseIt)->OnEvent(&event)) {
                break;
            }
        }
    });

    m_Window->mouseButtonEvent.setCallback([this](glfw::Window &window, glfw::MouseButton button,
                                                    glfw::MouseButtonState action, glfw::ModifierKeyBit mods) {
        switch (action) {
            case glfw::MouseButtonState::Press: {
                MouseButtonPressedEvent event{static_cast<uint16_t>(button)};
                for (auto reverseIt = m_Layers.rbegin(); reverseIt != m_Layers.rend(); ++reverseIt) {
                    if ((*reverseIt)->OnEvent(&event)) {
                        break;
                    }
                }
                break;
            }
            case glfw::MouseButtonState::Release: {
                MouseButtonReleasedEvent event{static_cast<uint16_t>(button)};
                for (auto reverseIt = m_Layers.rbegin(); reverseIt != m_Layers.rend(); ++reverseIt) {
                    if ((*reverseIt)->OnEvent(&event)) {
                        break;
                    }
                }
                break;
            }
            default:
                std::cerr << "Unknown mouse button action: " << static_cast<int>(action) << std::endl;
        }
    });

    m_Window->cursorPosEvent.setCallback([this](glfw::Window &window, double xPos, double yPos) {
        MouseMovedEvent event{static_cast<float>(xPos), static_cast<float>(yPos)};
        for (auto reverseIt = m_Layers.rbegin(); reverseIt != m_Layers.rend(); ++reverseIt) {
            if ((*reverseIt)->OnEvent(&event)) {
                break;
            }
        }
    });
}

void BasicContextImpl::InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();

    CreateSwapChain();
    CreateImageViews();

    CreateSampler();

    CreateRenderPass();
    CreateFramebuffers();

    CreateCommandPool();
    CreateSyncObjects();
    CreateCommandBuffer();

    InitImGui();
}

void BasicContextImpl::CreateInstance() {
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
        .apiVersion = vk::ApiVersion13
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

    // m_VkInstance = vk::raii::Instance(g_VkRaiiContext, createInfo);
    m_Instance = m_Context.createInstance(createInfo).value();
}

bool BasicContextImpl::CheckValidationLayerSupport() {
    auto availableLayers = vk::enumerateInstanceLayerProperties().value;

    for (const auto &layerName: s_ValidationLayers) {
        bool layerFound = false;
        for (const auto &layerProperties: availableLayers) {
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

    // std::cout << "Validation layers are supported." << std::endl;

    return true;
}

void BasicContextImpl::SetupDebugMessenger() {
    if constexpr (!enableValidationLayers) return;

    m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(PopulateDebugMessengerCreateInfo()).value();
}

std::vector<const char *> BasicContextImpl::GetRequiredExtensions() {
    auto glfwRequiredExtensions = glfw::getRequiredInstanceExtensions();
    if constexpr (enableValidationLayers) {
        glfwRequiredExtensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
    return glfwRequiredExtensions;
}

inline vk::Bool32 DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                vk::DebugUtilsMessageTypeFlagsEXT messageType,
                                const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                void *pUserData) {
    if (messageSeverity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    } else {
        std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return vk::False;
}

vk::DebugUtilsMessengerCreateInfoEXT BasicContextImpl::PopulateDebugMessengerCreateInfo() {
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

void BasicContextImpl::PickPhysicalDevice() {
    auto physicalDevices = m_Instance.enumeratePhysicalDevices().value();
    if (physicalDevices.empty()) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    for (const auto &device: physicalDevices) {
        if (IsDeviceSuitable(device)) {
            m_PhysicalDevice = device;
            std::cout << "Physical device selected: " << device.getProperties().deviceName << std::endl;
            return;
        }
    }
}

void BasicContextImpl::CreateLogicalDevice() {
    if (!*m_PhysicalDevice) {
        throw std::runtime_error("Physical device not selected.");
    }

    auto queueFamilies = FindQueueFamilies(m_PhysicalDevice);

    float queuePriorities[1] = {1.0f};

    std::unordered_set<uint32_t> uniqueQueueFamilies = {
        queueFamilies.GraphicsFamily.value(),
        queueFamilies.PresentFamily.value()
    };

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueFamilies.size());
    for (const auto &queueFamily: uniqueQueueFamilies) {
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

    m_Device = m_PhysicalDevice.createDevice(deviceCreateInfo).value();
    m_GraphicsQueue = m_Device.getQueue(queueFamilies.GraphicsFamily.value(), 0).value();
    m_PresentQueue = m_Device.getQueue(queueFamilies.PresentFamily.value(), 0).value();
}

void BasicContextImpl::CreateSurface() {
    vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo{
        .pNext = nullptr,
        .flags = {},
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = glfw::native::getWin32Window(*m_Window)
    };

    m_Surface = m_Instance.createWin32SurfaceKHR(surfaceCreateInfo).value();
}

QueueFamilyIndices BasicContextImpl::FindQueueFamilies(vk::PhysicalDevice physicalDevice) {
    QueueFamilyIndices indices;

    auto queueFamilies = physicalDevice.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        const auto &queueFamily = queueFamilies[i];

        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.GraphicsFamily = i;
        }

        if (physicalDevice.getSurfaceSupportKHR(i, *m_Surface).value) {
            indices.PresentFamily = i;
        }

        if (indices.IsComplete()) {
            break;
        }
    }

    return indices;
}

bool BasicContextImpl::IsDeviceSuitable(const vk::raii::PhysicalDevice &device) {
    auto queueFamilies = FindQueueFamilies(device);

    bool extensionSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate = false;

    if (extensionSupported) {
        auto swapChainSupport = QuerySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
    }

    return queueFamilies.IsComplete() && extensionSupported && swapChainAdequate;
}

bool BasicContextImpl::CheckDeviceExtensionSupport(vk::PhysicalDevice device) const {
    auto availableExtensions = device.enumerateDeviceExtensionProperties().value;
    std::set<std::string> requiredExtensions(s_DeviceExtensions.begin(), s_DeviceExtensions.end());
    for (const auto &extension: availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    if (requiredExtensions.empty()) {
        // std::cout << "All required device extensions are supported." << std::endl;
        return true;
    }
    std::cerr << "Not all required device extensions are supported!" << std::endl;
    for (const auto &ext: requiredExtensions) {
        std::cerr << "Missing extension: " << ext << std::endl;
    }
    return false;
}

SwapChainSupportDetails BasicContextImpl::QuerySwapChainSupport(vk::PhysicalDevice device) {
    SwapChainSupportDetails details;

    auto capabilities = device.getSurfaceCapabilitiesKHR(*m_Surface).value;
    auto formats = device.getSurfaceFormatsKHR(*m_Surface).value;
    auto presentModes = device.getSurfacePresentModesKHR(*m_Surface).value;

    return {
        .Capabilities = std::move(capabilities),
        .Formats = std::move(formats),
        .PresentModes = std::move(presentModes)
    };
}

vk::SurfaceFormatKHR BasicContextImpl::ChooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats) const {
    for (const auto &availableFormat: availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats.front();
}

vk::PresentModeKHR BasicContextImpl::ChooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes) const {
    return vk::PresentModeKHR::eFifo; // FIFO is guaranteed to be supported
}

vk::Extent2D BasicContextImpl::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    auto [width, height] = m_Window->getFramebufferSize();

    vk::Extent2D actualExtent = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);

    return actualExtent;
}

void BasicContextImpl::CreateSwapChain() {
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(*m_PhysicalDevice);

    auto surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
    auto presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
    auto extent = ChooseSwapExtent(swapChainSupport.Capabilities);

    uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
    m_MinImageCount = swapChainSupport.Capabilities.minImageCount;
    m_ImageCount = imageCount;
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
        .queueFamilyIndexCount = static_cast<uint32_t>((queueFamilies.GraphicsFamily == queueFamilies.PresentFamily)
                                                           ? 0
                                                           : 2),
        .pQueueFamilyIndices = (queueFamilies.GraphicsFamily == queueFamilies.PresentFamily)
                                   ? nullptr
                                   : queueFamilyIndices,
        .preTransform = swapChainSupport.Capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque, // Opaque is a common choice
        .presentMode = presentMode,
        .clipped = vk::True,
        .oldSwapchain = m_SwapChain.release()
    };

    m_SwapChain = m_Device.createSwapchainKHR(swapChainCreateInfo).value();

    m_SwapChainImages = m_SwapChain.getImages();
    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent = extent;
}

void BasicContextImpl::CreateImageViews() {
    m_SwapChainImageViews.reserve(m_SwapChainImages.size());

    for (const auto &image: m_SwapChainImages) {
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

        m_SwapChainImageViews.push_back(m_Device.createImageView(viewInfo).value());
    }
}

void BasicContextImpl::CreateSampler() {
    vk::SamplerCreateInfo samplerInfo{
        .sType = vk::StructureType::eSamplerCreateInfo,
        .pNext = nullptr,
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0f,
        .compareEnable = VK_FALSE,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
    };

    m_Sampler = m_Device.createSampler(samplerInfo).value();
}

inline void ImGuiUseStyleColorHazel() {
    auto &colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

    // Headers
    colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Buttons
    colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Tabs
    // colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    // colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.185f, 0.19f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.285f, 0.29f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.22f, 0.225f, 0.23f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.35f, 0.40f, 0.60f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.13f, 0.135f, 0.14f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.18f, 0.185f, 0.19f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.25f, 0.30f, 0.45f, 1.00f);

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Resize Grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

    // Check Mark
    colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);

    // Slider
    colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);

    // Docking
    // colors[ImGuiCol_DockingPreview] = ImVec4(0.31f, 0.31f, 0.31f, 0.7f);
    // colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.0f);
}

void BasicContextImpl::InitImGui() {
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // ImGui::StyleColorsDark();
    ImGuiUseStyleColorHazel();
    // ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForVulkan(m_Window.value(), true);

    InitImGuiForMyProgram(
        vk::ApiVersion13,
        *m_Instance,
        *m_PhysicalDevice,
        *m_Device,
        FindQueueFamilies(*m_PhysicalDevice).GraphicsFamily.value(),
        *m_GraphicsQueue,
        *m_RenderPass,
        m_MinImageCount, m_ImageCount
    );
}

void BasicContextImpl::BeginImGuiFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void BasicContextImpl::CreateRenderPass() {
    // disable v-sync
    vk::AttachmentDescription colorAttachment{
        .flags = {},
        .format = m_SwapChainImageFormat,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR
    };

    vk::AttachmentReference colorAttachmentRef{
        .attachment = 0, // Index of the attachment in the render pass
        .layout = vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::SubpassDescription subpass{
        .flags = {},
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };

    vk::SubpassDependency dependency{
        .srcSubpass = vk::SubpassExternal, // No previous subpass
        .dstSubpass = 0, // Our subpass
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .srcAccessMask = {},
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        .dependencyFlags = {}
    };

    vk::RenderPassCreateInfo renderPassInfo{
        .pNext = nullptr,
        .flags = {},
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    m_RenderPass = m_Device.createRenderPass(renderPassInfo).value();
}

void BasicContextImpl::CreateFramebuffers() {
    m_SwapChainFramebuffers.reserve(m_SwapChainImageViews.size());

    for (const auto &imageView: m_SwapChainImageViews) {
        vk::ImageView attachments[] = {
            *imageView // Single attachment for color
        };

        vk::FramebufferCreateInfo framebufferInfo{
            .pNext = nullptr,
            .flags = {},
            .renderPass = *m_RenderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = m_SwapChainExtent.width,
            .height = m_SwapChainExtent.height,
            .layers = 1 // Single layer for 2D images
        };

        m_SwapChainFramebuffers.push_back(
            m_Device.createFramebuffer(framebufferInfo).value()
        );
    }
}

void BasicContextImpl::CreateCommandPool() {
    auto queueFamilies = FindQueueFamilies(*m_PhysicalDevice);
    if (!queueFamilies.GraphicsFamily.has_value()) {
        throw std::runtime_error("Graphics queue family not found.");
    }
    vk::CommandPoolCreateInfo poolInfo{
        .pNext = nullptr,
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilies.GraphicsFamily.value()
    };

    m_CommandPool = m_Device.createCommandPool(poolInfo).value();
}

void BasicContextImpl::CreateCommandBuffer() {
    vk::CommandBufferAllocateInfo allocInfo{
        .pNext = nullptr,
        .commandPool = *m_CommandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_CommandBuffers.push_back(std::move(m_Device.allocateCommandBuffers(allocInfo).value().front()));
    }

    m_CommandBufferDependentContexts.resize(MAX_FRAMES_IN_FLIGHT);
}

void BasicContextImpl::CreateSyncObjects() {
    vk::SemaphoreCreateInfo semaphoreInfo{
        .pNext = nullptr,
        .flags = {}
    };
    vk::FenceCreateInfo fenceInfo{
        .pNext = nullptr,
        .flags = vk::FenceCreateFlagBits::eSignaled // Start with the fence signaled
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_ImageAvailableSemaphores.push_back(m_Device.createSemaphore(semaphoreInfo).value());
        m_InFlightFences.push_back(m_Device.createFence(fenceInfo).value());
    }

    // m_InFlightFence = m_Device.createFence(fenceInfo).value();
    for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
        m_RenderFinishedSemaphores.push_back(m_Device.createSemaphore(semaphoreInfo).value());
    }
}

void BasicContextImpl::DrawFrame() {
    BeginImGuiFrame();
    OnUpdate();
    ImGui::Render();

    auto waitForFenceResult = m_Device.waitForFences(*m_InFlightFences[m_CurrentFrame], vk::True,
                                                     std::numeric_limits<uint64_t>::max());

    if (waitForFenceResult != vk::Result::eSuccess) {
        std::cerr << "Failed to wait for fence: " << vk::to_string(waitForFenceResult) << std::endl;
        return;
    }

    auto [resultAcquireImage, imageIndex] = m_SwapChain.acquireNextImage(
        std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrame], nullptr
    );

    if (resultAcquireImage != vk::Result::eSuccess &&
        resultAcquireImage != vk::Result::eSuboptimalKHR) {
        if (resultAcquireImage == vk::Result::eErrorOutOfDateKHR) {
            RecreateSwapChain();
            return;
        } else {
            std::cerr << "Failed to acquire swap chain image: " << vk::to_string(resultAcquireImage) << std::endl;
            return;
        }
    }

    m_Device.resetFences(*m_InFlightFences[m_CurrentFrame]);

    // m_ImageViewDependentRenderTargetsPerFrameBuffer[imageIndex] = std::move(m_DependentRenderTargets);

    m_CommandBufferDependentContexts[m_CurrentFrame].clear();
    m_CommandBuffers[m_CurrentFrame].reset();

    vk::CommandBufferBeginInfo beginInfo{
        .pNext = nullptr,
        .flags = {},
        .pInheritanceInfo = nullptr // No inheritance for primary command buffers
    };

    m_CommandBuffers[m_CurrentFrame].begin(beginInfo);

    vk::ClearValue clearColor{
        m_ClearColor
    };

    vk::RenderPassBeginInfo renderPassInfo{
        .pNext = nullptr,
        .renderPass = m_RenderPass,
        .framebuffer = m_SwapChainFramebuffers[imageIndex],
        .renderArea = vk::Rect2D{
            .offset = {0, 0},
            .extent = m_SwapChainExtent
        },
        .clearValueCount = 1,
        .pClearValues = &clearColor
    };

    m_CommandBuffers[m_CurrentFrame].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    for (auto reverseIt = m_Layers.rbegin(); reverseIt != m_Layers.rend(); ++reverseIt) {
        auto &layer = *reverseIt;
        layer->OnSubmitCommandBuffer(m_CommandBuffers[m_CurrentFrame],
                                     m_CommandBufferDependentContexts[m_CurrentFrame]);
    }

    ImDrawData *draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, *m_CommandBuffers[m_CurrentFrame]);

    m_CommandBuffers[m_CurrentFrame].endRenderPass();

    m_CommandBuffers[m_CurrentFrame].end();

    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::Semaphore waitSemaphores[] = {*m_ImageAvailableSemaphores[m_CurrentFrame]};
    vk::Semaphore signalSemaphores[] = {*m_RenderFinishedSemaphores[imageIndex]};
    vk::CommandBuffer commandBuffers[] = {*m_CommandBuffers[m_CurrentFrame]};

    vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = commandBuffers,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };

    m_GraphicsQueue.submit(submitInfo, m_InFlightFences[m_CurrentFrame]);

    vk::SwapchainKHR swapChains[] = {*m_SwapChain};
    vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex
    };

    vk::Result presentResult = m_PresentQueue.presentKHR(presentInfo);

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    auto &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void BasicContextImpl::RecreateSwapChain() {
    m_Device.waitIdle();

    CleanupSwapChain();

    CreateSwapChain();
    CreateImageViews();
    CreateFramebuffers();

    std::cout << "Swap chain recreated successfully." << std::endl;
}

void BasicContextImpl::CleanupSwapChain() {
    m_SwapChainFramebuffers.clear();
    m_SwapChainImageViews.clear();
    m_SwapChainImages.clear();

    m_SwapChain.clear();
}

void BasicContextImpl::MainLoop() {
    while (!m_Window->shouldClose()) {
        glfw::pollEvents();
        if (!m_ShouldUpdate) {
            continue;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        DrawFrame();
    }

    m_Device.waitIdle();
}

void BasicContextImpl::Cleanup() {
    m_CommandBufferDependentContexts.clear();
    CleanupSwapChain();

    ShutdownImGuiForMyProgram();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void BasicContextImpl::OnUpdate() {
    for (auto &layer: m_Layers) {
        layer->OnUpdate();
    }
}

bool QueueFamilyIndices::IsComplete() const {
    return GraphicsFamily.has_value() && PresentFamily.has_value();
}
