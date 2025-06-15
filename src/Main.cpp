module Main;

import <vulkan/vk_platform.h>;
import "vendor/glfwpp/native.h";
import Util;
import ImGui;
import <cassert>;

bool QueueFamilyIndices::IsComplete() const {
    return GraphicsFamily.has_value() && PresentFamily.has_value();
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData);

QueueFamilyIndices HelloTriangleApplication::FindQueueFamilies(vk::PhysicalDevice physicalDevice) {
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

HelloTriangleApplication::HelloTriangleApplication() {
    InitializeWindow();
    InitVulkan();
}

void HelloTriangleApplication::MainLoop() {
    while (!m_Window->shouldClose()) {
        glfw::pollEvents();
        if (!m_ShouldUpdate) {
            continue;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        DrawFrame();
        // m_Device.waitIdle();
    }

    m_Device.waitIdle();
}

void HelloTriangleApplication::Cleanup() {
    CleanupSwapChain();

    ShutdownImGuiForMyProgram();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void HelloTriangleApplication::InitializeWindow() {
    glfw::WindowHints{
        .resizable = true,
        .clientApi = glfw::ClientApi::None,
    }.apply();

    m_Window = glfw::Window{1920, 1080, "Vulkan", nullptr, nullptr};

    m_Window->framebufferSizeEvent.setCallback([this](glfw::Window &window, int width, int height) {
        if (width > 0 && height > 0) {
            m_ShouldUpdate = true;
            m_Window->setShouldClose(false);
            RecreateSwapChain();
        } else {
            m_ShouldUpdate = false;
        }
    });
}

void HelloTriangleApplication::InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();

    CreateSwapChain();
    CreateImageViews();

    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();

    CreateCommandPool();
    CreateSyncObjects();
    CreateCommandBuffer();

    InitImGui();
}

void HelloTriangleApplication::CreateInstance() {
    if (*g_VkInstance)
        return;

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

    // m_VkInstance = vk::raii::Instance(g_VkRaiiContext, createInfo);
    g_VkInstance = g_VkRaiiContext.createInstance(createInfo).value();

    std::cout << "Vulkan instance created successfully." << std::endl;
}

bool HelloTriangleApplication::CheckValidationLayerSupport() {
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

    std::cout << "Validation layers are supported." << std::endl;

    return true;
}

void HelloTriangleApplication::SetupDebugMessenger() {
    if constexpr (!enableValidationLayers) return;

    m_DebugMessenger = g_VkInstance.createDebugUtilsMessengerEXT(PopulateDebugMessengerCreateInfo()).value();
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
    auto availableExtensions = device.enumerateDeviceExtensionProperties().value;
    std::set<std::string> requiredExtensions(s_DeviceExtensions.begin(), s_DeviceExtensions.end());
    for (const auto &extension: availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    if (requiredExtensions.empty()) {
        std::cout << "All required device extensions are supported." << std::endl;
        return true;
    }
    std::cerr << "Not all required device extensions are supported!" << std::endl;
    for (const auto &ext: requiredExtensions) {
        std::cerr << "Missing extension: " << ext << std::endl;
    }
    return false;
}

SwapChainSupportDetails HelloTriangleApplication::QuerySwapChainSupport(vk::PhysicalDevice device) {
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

vk::SurfaceFormatKHR HelloTriangleApplication::ChooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats) const {
    for (const auto &availableFormat: availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats.front();
}

vk::PresentModeKHR HelloTriangleApplication::ChooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes) const {
    return vk::PresentModeKHR::eFifo;
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

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);

    return actualExtent;
}

uint32_t g_MinImageCount, g_ImageCount;

void HelloTriangleApplication::CreateSwapChain() {
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(*m_PhysicalDevice);

    auto surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
    auto presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
    auto extent = ChooseSwapExtent(swapChainSupport.Capabilities);

    uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
    g_MinImageCount = swapChainSupport.Capabilities.minImageCount;
    g_ImageCount = imageCount;
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

void HelloTriangleApplication::CreateImageViews() {
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

void HelloTriangleApplication::InitImGui() {
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForVulkan(m_Window.value(), true);

    InitImGuiForMyProgram(
        vk::ApiVersion10,
        *g_VkInstance,
        *m_PhysicalDevice,
        *m_Device,
        FindQueueFamilies(*m_PhysicalDevice).GraphicsFamily.value(),
        *m_GraphicsQueue,
        *m_RenderPass,
        g_MinImageCount, g_ImageCount
    );
}

void HelloTriangleApplication::DrawImGui() {
    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Begin("Debug Window");
    ImGui::Text("Frame Rate: %.1f FPS", ImGui::GetIO().Framerate);
    ImGui::Text("Swap Chain Image Count: %zu", m_SwapChainImages.size());
    ImGui::Text("Current Frame: %zu", m_CurrentFrame);
    if (ImGui::Button("Recreate Swap Chain")) {
        RecreateSwapChain();
    }
    ImGui::End();
}

void HelloTriangleApplication::CreateGraphicsPipeline() {
    vk::raii::ShaderModule vertShaderModule = CreateShaderModule(ReadFileBin("shaders/simple_triangle.vert.spv"));
    vk::raii::ShaderModule fragShaderModule = CreateShaderModule(ReadFileBin("shaders/simple_triangle.frag.spv"));

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .pNext = nullptr,
        .flags = {},
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *vertShaderModule,
        .pName = "main",
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .pNext = nullptr,
        .flags = {},
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = *fragShaderModule,
        .pName = "main",
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    std::array dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{
        .pNext = nullptr,
        .flags = {},
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .pNext = nullptr,
        .flags = {},
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .pNext = nullptr,
        .flags = {},
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False
    };

    vk::Viewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_SwapChainExtent.width),
        .height = static_cast<float>(m_SwapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vk::Rect2D scissor{
        .offset = {0, 0},
        .extent = m_SwapChainExtent
    };

    vk::PipelineViewportStateCreateInfo viewportState{
        .pNext = nullptr,
        .flags = {},
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    vk::PipelineRasterizationStateCreateInfo rasterizationCreateInfo{
        .pNext = nullptr,
        .flags = {},
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .pNext = nullptr,
        .flags = {},
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = vk::False,
        .alphaToOneEnable = vk::False
    };

    // enable alpha-blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .pNext = nullptr,
        .flags = {},
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {{0.0f, 0.0f, 0.0f, 0.0f}}
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .pNext = nullptr,
        .flags = {},
        .setLayoutCount = 0, // No descriptor sets for now
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0, // No push constants for now
        .pPushConstantRanges = nullptr
    };

    m_PipelineLayout = m_Device.createPipelineLayout(pipelineLayoutInfo).value();

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext = nullptr,
        .flags = {},
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = nullptr, // No tessellation for now
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationCreateInfo,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr, // No depth/stencil for now
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = *m_PipelineLayout,
        .renderPass = *m_RenderPass,
        .subpass = 0, // Index of the subpass in the render pass
        .basePipelineHandle = nullptr, // No base pipeline for now
        .basePipelineIndex = -1 // No base pipeline index for now
    };

    m_GraphicsPipeline = m_Device.createGraphicsPipeline(nullptr, pipelineInfo).value();
}

vk::raii::ShaderModule HelloTriangleApplication::CreateShaderModule(const std::vector<char> &code) {
    vk::ShaderModuleCreateInfo createInfo{
        .pNext = nullptr,
        .flags = {},
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t *>(code.data())
    };

    auto result = m_Device.createShaderModule(createInfo).value();

    return result;
}

void HelloTriangleApplication::CreateRenderPass() {
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

void HelloTriangleApplication::CreateFramebuffers() {
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

void HelloTriangleApplication::CreateCommandPool() {
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

void HelloTriangleApplication::CreateCommandBuffer() {
    vk::CommandBufferAllocateInfo allocInfo{
        .pNext = nullptr,
        .commandPool = *m_CommandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_CommandBuffers.push_back(std::move(m_Device.allocateCommandBuffers(allocInfo).value().front()));
    }
}

void HelloTriangleApplication::CreateSyncObjects() {
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
    // m_ImageAvailableSemaphore = m_Device.createSemaphore(semaphoreInfo).value();
    // m_RenderFinishedSemaphore = m_Device.createSemaphore(semaphoreInfo).value();



    // m_InFlightFence = m_Device.createFence(fenceInfo).value();
    for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
        m_RenderFinishedSemaphores.push_back(m_Device.createSemaphore(semaphoreInfo).value());
    }

    // m_ImagesInFlight.resize(m_SwapChainImages.size(), nullptr);
}

void HelloTriangleApplication::RecordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
    vk::CommandBufferBeginInfo beginInfo{
        .pNext = nullptr,
        .flags = {},
        .pInheritanceInfo = nullptr // No inheritance for primary command buffers
    };

    vk::Result beginResult = commandBuffer.begin(beginInfo);
    if (beginResult != vk::Result::eSuccess) {
        std::cerr << "Failed to begin command buffer: " << vk::to_string(beginResult) << std::endl;
        return;
    }

    vk::ClearValue clearColor{
        vk::ClearColorValue(std::array<float, 4>{.5f, .0f, .5f, 1.0f})
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

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_GraphicsPipeline);

    vk::Viewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_SwapChainExtent.width),
        .height = static_cast<float>(m_SwapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vk::Rect2D scissor = {
        .offset = {0, 0},
        .extent = m_SwapChainExtent
    };

    commandBuffer.setViewport(0, viewport);
    commandBuffer.setScissor(0, scissor);
    commandBuffer.draw(3, 1, 0, 0); // Draw a triangle (3 vertices)

    ImDrawData *draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);

    commandBuffer.endRenderPass();

    vk::Result endResult = commandBuffer.end();
    if (endResult != vk::Result::eSuccess) {
        std::cerr << "Failed to record command buffer: " << vk::to_string(endResult) << std::endl;
    }
}

void HelloTriangleApplication::BeginImGuiFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawImGui();
}

void HelloTriangleApplication::DrawFrame() {
    BeginImGuiFrame();
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

    m_CommandBuffers[m_CurrentFrame].reset();
    RecordCommandBuffer(m_CommandBuffers[m_CurrentFrame], imageIndex);

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

void HelloTriangleApplication::RecreateSwapChain() {
    m_Device.waitIdle();

    CleanupSwapChain();

    CreateSwapChain();
    CreateImageViews();
    CreateFramebuffers();

    std::cout << "Swap chain recreated successfully." << std::endl;
}
void HelloTriangleApplication::CleanupSwapChain() {
    m_SwapChainFramebuffers.clear();
    m_SwapChainImageViews.clear();
    m_SwapChainImages.clear();

    m_SwapChain.clear();
}

void HelloTriangleApplication::PickPhysicalDevice() {
    auto physicalDevices = g_VkInstance.enumeratePhysicalDevices().value();
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

void HelloTriangleApplication::CreateSurface() {
    vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo{
        .pNext = nullptr,
        .flags = {},
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = glfw::native::getWin32Window(*m_Window)
    };

    m_Surface = g_VkInstance.createWin32SurfaceKHR(surfaceCreateInfo).value();
}


vk::Bool32 DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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
