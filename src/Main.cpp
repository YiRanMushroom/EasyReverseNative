import ImGui;
import BasicContext;
import Application;
import vulkan_hpp;
import ApplicationLayers;
import Platform.WindowsUtils;
import std.compat;

void AssertHasFile(const std::filesystem::path &resourcePath) {
    if (!std::filesystem::exists(resourcePath)) {
        Windows::ShowErrorMessage(
            ("File not found: " + resourcePath.string() +
             "\nThe program may not be able to run expectantly.\n")
            .c_str(),
            "File Not Found"
        );
    }
}

int main() {
    AssertHasFile("assets/fonts/OpenSans.ttf");
    AssertHasFile("assets/fonts/NotoSansSC.ttf");
    auto basicContext = CreateBasicContext(
        WindowSpec{
            .title = "Easy Reverse Native",
            .width = 1280,
            .height = 480
        }
    );

    g_BasicContext = basicContext.get();

    auto& io = ImGui::GetIO();
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 32.0f; // Set your desired font size, all languages
    io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans.ttf", font_cfg.SizePixels, &font_cfg, io.Fonts->GetGlyphRangesDefault());
    font_cfg.MergeMode = true;
    io.Fonts->AddFontFromFileTTF("assets/fonts/NotoSansSC.ttf", font_cfg.SizePixels, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    io.Fonts->Build();

    basicContext->EmplaceLayer<BackGroundLayer>();
    basicContext->EmplaceLayer<AppUiLayer>();

    basicContext->SetClearColor(vk::ClearColorValue(std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.0f}));

    basicContext->MainLoop();

    basicContext->Cleanup();

    return 0;
}
