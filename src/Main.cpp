import ImGui;
import BasicContext;
import Application;
import vulkan_hpp;
import ApplicationLayers;

int main() {
    auto basicContext = CreateBasicContext(
        WindowSpec{
            .title = "Easy Reverse Native",
            .width = 1920,
            .height = 1080
        }
    );

    g_BasicContext = basicContext.get();

    // Set ImGui font size to 32
    auto& io = ImGui::GetIO();
    io.FontGlobalScale = 2.0f; // Scale the font size to 32px

    basicContext->EmplaceLayer<BackGroundLayer>();
    basicContext->EmplaceLayer<AppUiLayer>();

    basicContext->SetClearColor(vk::ClearColorValue(std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.0f}));

    basicContext->MainLoop();

    basicContext->Cleanup();

    return 0;
}
