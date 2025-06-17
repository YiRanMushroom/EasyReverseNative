export module Application;

import std;
import vulkan_hpp;
import glfwpp;
import <glm/glm.hpp>;
import <vulkan/vk_platform.h>;
import ImGui;

import BasicContext;
import Event.AllEvents;


class ImGuiImageRenderTarget {
public:
    ImGuiImageRenderTarget(IBasicContext *context, uint32_t width = 960, uint32_t height = 640);

public:
    void Rebuild();

    void Flush();

    void FlushAndWait();

    vk::Fence GetFence() const;

    vk::ClearValue m_ClearColor{
        vk::ClearColorValue(std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f})
    };

    ~ImGuiImageRenderTarget();

private:
    void CreateImageAndView();

    void CreateRenderPass();

    void CreateGraphicsPipeline();

    void CreateFramebuffer();

    void CreateSyncObjects();

    void CreateCommandBuffer();

    void CreateDescriptorSetLayout();

    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    void RecordCommandBuffer();

    IBasicContext *m_Ctx{nullptr};

public:
    vk::raii::Image m_Image{nullptr};
    vk::raii::DeviceMemory m_ImageMemory{nullptr};
    vk::raii::ImageView m_ImageView{nullptr};
    vk::raii::Pipeline m_GraphicsPipeline{nullptr};
    vk::raii::PipelineLayout m_PipelineLayout{nullptr};
    vk::raii::RenderPass m_RenderPass{nullptr};
    vk::raii::Framebuffer m_Framebuffer{nullptr};
    vk::raii::Fence m_RenderFinishedFence{nullptr};
    vk::raii::CommandBuffer m_CommandBuffer{nullptr};

    vk::DescriptorSet m_ImageDescriptorSet{nullptr};
    VkDescriptorSet m_SetHandler;

    vk::Rect2D m_RenderArea{
        .offset = vk::Offset2D{0, 0},
        .extent = vk::Extent2D{960, 640} // Set your desired width and height
    };

    bool m_NeedsRebuild = false;
};

export IBasicContext *g_BasicContext = nullptr;

class AppUiLayer : public IUpdatableLayer {
public:
    AppUiLayer() {
        m_RenderTarget = std::make_shared<ImGuiImageRenderTarget>(g_BasicContext, 960, 640);
    }

    void OnUpdate() override {
        ImGui::Begin("Application UI");
        ImGui::Text("Hello, Triangle!");
        if (ImGui::Button("Rebuild Swap Chain")) {
            g_BasicContext->RecreateSwapChain();
        }

        ImGui::End();

        m_RenderTarget->Flush();

        ImTextureID id = reinterpret_cast<ImTextureID>(m_RenderTarget->m_SetHandler);
        auto [width, height] = m_RenderTarget->m_RenderArea.extent;


        ImGui::Begin("ImGui Frame Buffer");
        auto [currentWidth, currentHeight] = ImGui::GetContentRegionAvail();
        ImGui::Image(id, ImVec2(static_cast<float>(width), static_cast<float>(height)));
        ImGui::End();

        if ((width != currentWidth || height != currentHeight) && (currentWidth > 0 && currentHeight > 0)) {
            m_RenderTarget = std::make_shared<ImGuiImageRenderTarget>(g_BasicContext, currentWidth, currentHeight);
        }
    }

    void OnSubmitCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<std::any> &dependentContexts) override {
        dependentContexts.push_back(m_RenderTarget);
    }

    bool OnEvent(const Event *event) override {
        if (event->GetEventType() == EventType::KeyTyped) {
            std::cout << event->ToString() << std::endl;
        }
        return true;
    }

private:
    std::shared_ptr<ImGuiImageRenderTarget> m_RenderTarget;
};

export template<typename F>
void RenderBackgroundSpace(const F &renderMenuBar) {
    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;


    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    // if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    //     window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);

    ImGui::PopStyleVar();

    ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    }

    if (ImGui::BeginMenuBar()) {
        renderMenuBar();
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

export class BackGroundLayer : public IUpdatableLayer {
public:
    void OnUpdate() override {
        RenderBackgroundSpace([]{});
    }

    void OnSubmitCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<std::any> &dependentContexts) override {
        // No specific command buffer submission for this layer
    }

    bool OnEvent(const Event *event) override {
        // Handle events if necessary
        return false;
    }
};

