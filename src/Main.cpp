import std;
import vulkan_hpp;
import glfwpp;
import <glm/glm.hpp>;

int main() {
    auto GLFW = glfw::init();

    glfw::WindowHints{
        .contextVersionMajor = 4,
        .contextVersionMinor = 6,
        .openglProfile = glfw::OpenGlProfile::Core
    }.apply();
    glfw::Window window{640, 480, "Hello World"};

    uint32_t extensionCount = 0;
    (void) vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << "Available Vulkan extensions: " << extensionCount << std::endl;

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    glfw::makeContextCurrent(window);

    while (!window.shouldClose()) {
        window.swapBuffers();
        glfw::pollEvents();
    }

    return 0;
}
