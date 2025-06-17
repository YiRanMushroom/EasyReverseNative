module Main;

import <vulkan/vk_platform.h>;
import "vendor/glfwpp/native.h";
import Util;
import ImGui;
import <cassert>;
import BasicContext;

ImGuiImageRenderTarget::ImGuiImageRenderTarget(IBasicContext *app, uint32_t width, uint32_t height)
    : m_Ctx(app), m_RenderArea{
        .offset = vk::Offset2D{0, 0},
        .extent = vk::Extent2D{width, height}
    } {
    Rebuild();
}

void ImGuiImageRenderTarget::Rebuild() {
    assert(m_Ctx != nullptr && "HelloTriangleApplication pointer cannot be null");
    CreateImageAndView();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffer();
    CreateSyncObjects();
    CreateCommandBuffer();
    CreateDescriptorSetLayout();
}

void ImGuiImageRenderTarget::Flush() {
    if (m_NeedsRebuild) {
        m_Ctx->GetLogicalDevice().waitIdle();
        ImGui_ImplVulkan_RemoveTexture(m_SetHandler);
        Rebuild();
        m_NeedsRebuild = false;
    }
    auto waitResult = m_Ctx->GetLogicalDevice().waitForFences(*m_RenderFinishedFence, vk::True, std::numeric_limits<uint64_t>::max());
    if (waitResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence");
    }
    m_Ctx->GetLogicalDevice().resetFences(*m_RenderFinishedFence);
    m_CommandBuffer.reset();

    RecordCommandBuffer();

    vk::CommandBuffer commandBuffers[] = {*m_CommandBuffer};

    vk::SubmitInfo submitInfo{
        .sType = vk::StructureType::eSubmitInfo,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = commandBuffers,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };

    m_Ctx->GetGraphicsQueue().submit(submitInfo, *m_RenderFinishedFence);
}

void ImGuiImageRenderTarget::FlushAndWait() {
    Flush();
    auto waitResult = m_Ctx->GetLogicalDevice().waitForFences(*m_RenderFinishedFence, vk::True, std::numeric_limits<uint64_t>::max());
    if (waitResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence");
    }
}

vk::Fence ImGuiImageRenderTarget::GetFence() const {
    return *m_RenderFinishedFence;
}

ImGuiImageRenderTarget::~ImGuiImageRenderTarget()  {
    if (m_Ctx) {
        m_Ctx->GetLogicalDevice().waitIdle();
        ImGui_ImplVulkan_RemoveTexture(m_SetHandler);
    }
}

void ImGuiImageRenderTarget::CreateImageAndView() {
    vk::ImageCreateInfo imageInfo{
        .pNext = nullptr,
        .flags = vk::ImageCreateFlags{},
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR8G8B8A8Unorm, // or any other format you need
        .extent = {
            .width = m_RenderArea.extent.width,
            .height = m_RenderArea.extent.height,
            .depth = 1 // For 2D images, depth is always 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1, // or any other sample count
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        .sharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = vk::ImageLayout::eUndefined // or any other initial layout you need
    };

    m_Image = m_Ctx->GetLogicalDevice().createImage(imageInfo).value();
    vk::DeviceImageMemoryRequirements deviceImageMemoryRequirements{
        .sType = vk::StructureType::eDeviceImageMemoryRequirementsKHR,
        .pNext = nullptr,
        .pCreateInfo = &imageInfo,
        .planeAspect = vk::ImageAspectFlagBits::eColor,
    };

    vk::MemoryRequirements memRequirements =
        m_Ctx->GetLogicalDevice().getImageMemoryRequirements(deviceImageMemoryRequirements).memoryRequirements;

    vk::MemoryAllocateInfo allocInfo{
        .sType = vk::StructureType::eMemoryAllocateInfo,
        .pNext = nullptr,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    };

    m_ImageMemory = m_Ctx->GetLogicalDevice().allocateMemory(allocInfo).value();

    vk::BindImageMemoryInfo bindInfo{
        .sType = vk::StructureType::eBindImageMemoryInfo,
        .pNext = nullptr,
        .image = *m_Image,
        .memory = *m_ImageMemory,
        .memoryOffset = 0
    };

    m_Ctx->GetLogicalDevice().bindImageMemory2(bindInfo);

    vk::ImageViewCreateInfo viewInfo{
        .sType = vk::StructureType::eImageViewCreateInfo,
        .pNext = nullptr,
        .image = *m_Image,
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eR8G8B8A8Unorm, // Match the image format
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

    m_ImageView = m_Ctx->GetLogicalDevice().createImageView(viewInfo).value();
}

void ImGuiImageRenderTarget::CreateRenderPass() {
    vk::AttachmentDescription colorAttachment{
        .flags = {},
        .format = vk::Format::eR8G8B8A8Unorm, // Match the image format
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal
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

    m_RenderPass = m_Ctx->GetLogicalDevice().createRenderPass(renderPassInfo).value();
}

vk::raii::ShaderModule CreateShaderModule(const vk::raii::Device& device, const std::vector<char> &code) {
    vk::ShaderModuleCreateInfo createInfo{
        .pNext = nullptr,
        .flags = {},
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t *>(code.data())
    };

    auto result = device.createShaderModule(createInfo).value();

    return result;
}

void ImGuiImageRenderTarget::CreateGraphicsPipeline() {
    vk::raii::ShaderModule vertShaderModule = CreateShaderModule(m_Ctx->GetLogicalDevice(), ReadFileBin("shaders/simple_triangle.vert.spv"));
    vk::raii::ShaderModule fragShaderModule = CreateShaderModule(m_Ctx->GetLogicalDevice(), ReadFileBin("shaders/simple_triangle.frag.spv"));

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
        .width = static_cast<float>(m_RenderArea.extent.width), // Set your desired width
        .height = static_cast<float>(m_RenderArea.extent.height), // Set your desired height
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vk::Rect2D scissor{
        .offset = {0, 0},
        .extent = {
            .width = static_cast<uint32_t>(m_RenderArea.extent.width), // Set your desired width
            .height = static_cast<uint32_t>(m_RenderArea.extent.height) // Set your desired height
        }
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

    m_PipelineLayout = m_Ctx->GetLogicalDevice().createPipelineLayout(pipelineLayoutInfo).value();

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

    m_GraphicsPipeline = m_Ctx->GetLogicalDevice().createGraphicsPipeline(nullptr, pipelineInfo).value();
}

void ImGuiImageRenderTarget::CreateFramebuffer() {
    vk::ImageView attachments[] = {*m_ImageView};

    vk::FramebufferCreateInfo framebufferInfo{
        .pNext = nullptr,
        .flags = {},
        .renderPass = *m_RenderPass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = static_cast<uint32_t>(m_RenderArea.extent.width), // Set your desired width
        .height = static_cast<uint32_t>(m_RenderArea.extent.height), // Set your desired height
        .layers = 1
    };

    m_Framebuffer = m_Ctx->GetLogicalDevice().createFramebuffer(framebufferInfo).value();
}

void ImGuiImageRenderTarget::CreateSyncObjects() {
    vk::FenceCreateInfo fenceInfo{
        .sType = vk::StructureType::eFenceCreateInfo,
        .pNext = nullptr,
        .flags = vk::FenceCreateFlagBits::eSignaled // Start in signaled state
    };

    m_RenderFinishedFence = m_Ctx->GetLogicalDevice().createFence(fenceInfo).value();
}

void ImGuiImageRenderTarget::CreateCommandBuffer() {
    vk::CommandBufferAllocateInfo allocInfo{
        .sType = vk::StructureType::eCommandBufferAllocateInfo,
        .pNext = nullptr,
        .commandPool = *m_Ctx->GetCommandPool(),
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    m_CommandBuffer = std::move(m_Ctx->GetLogicalDevice().allocateCommandBuffers(allocInfo).value().front());
}

void ImGuiImageRenderTarget::CreateDescriptorSetLayout() {
    m_ImageDescriptorSet = ImGui_ImplVulkan_AddTexture(
        *m_Ctx->GetSampler(),
        *m_ImageView,
        static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal)
    );

    m_SetHandler = m_ImageDescriptorSet;
}

uint32_t ImGuiImageRenderTarget::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = m_Ctx->GetPhysicalDevice().getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void ImGuiImageRenderTarget::RecordCommandBuffer() {
    // start recording commands
    vk::CommandBufferBeginInfo beginInfo{
        .pNext = nullptr,
        .flags = {},
        .pInheritanceInfo = nullptr // No inheritance for primary command buffers
    };

    m_CommandBuffer.begin(beginInfo);

    vk::RenderPassBeginInfo renderPassInfo{
        .pNext = nullptr,
        .renderPass = m_RenderPass,
        .framebuffer = m_Framebuffer,
        .renderArea = vk::Rect2D{
            .offset = {0, 0},
            .extent = {
                .width = m_RenderArea.extent.width,
                .height = m_RenderArea.extent.height
            }
        },
        .clearValueCount = 1,
        .pClearValues = &m_ClearColor
    };

    m_CommandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    m_CommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_GraphicsPipeline);

    vk::Viewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_RenderArea.extent.width), // Set your desired width
        .height = static_cast<float>(m_RenderArea.extent.height), // Set your desired height
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vk::Rect2D scissor = {
        .offset = {0, 0},
        .extent = {
            .width = (m_RenderArea.extent.width), // Set your desired width
            .height = (m_RenderArea.extent.height) // Set your desired height
        }
    };

    m_CommandBuffer.setViewport(0, viewport);
    m_CommandBuffer.setScissor(0, scissor);
    m_CommandBuffer.draw(3, 1, 0, 0); // Draw a triangle (3 vertices)

    m_CommandBuffer.endRenderPass();

    m_CommandBuffer.end();
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData);
