#ifndef _GLFW_VULKAN_HELPER_H
#define _GLFW_VULKAN_HELPER_H

#include <vector>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices {
    int graphicsFamily;
    int presentFamily;

    void init() {
        graphicsFamily = -1;
        presentFamily = -1;
    }

    bool isComplete() {
        return (graphicsFamily != -1 && presentFamily != -1);
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct UserTextureImage {
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
    VkSampler sampler;
};

class ImguiVulkanHelper
{
public:
    bool initWindow(int width, int height, const char *title);
    bool initVulkan(const char *app_name, uint32_t app_version);
    void setFramebufferResized(void);
    ~ImguiVulkanHelper();

    GLFWwindow *getWindow(void);
    VkDevice getDevice(void);
    void fillImguiVulkanInitInfo(ImGui_ImplVulkan_InitInfo *info);
    VkRenderPass getRenderPass(void);
    bool initializeFontTexture(void);
    ImTextureID loadImage(const char *image, int *width, int *height);
    void drawFrame(ImDrawData *data);

private:
    bool framebufferResized = false;
    bool terminated = false;
    uint32_t currentFrame = 0;
    VkClearValue clearColor = { 0.45f, 0.55f, 0.60f, 1.00f };

    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    uint32_t imageCount;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> swapChainImageFences;
    std::vector<UserTextureImage> userTextureImages;

    bool createInstance(const char *app_name, uint32_t app_version);
    bool createDevice(void);
    bool createSwapChain(void);
    bool createImageViews(void);
    bool createRenderPass(void);
    bool createFramebuffers(void);
    bool createCommandPool(void);
    bool createDescriptorPool(void);
    bool createCommandBuffers(void);
    bool createSyncObjects(void);
    bool checkValidationLayerSupport(void);
    void cleanupSwapChain(void);
    void cleanup(void);
    void recreateSwapChain(void);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool isDeviceSuitable(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkImageView createImageView(VkImage image, VkFormat format);
    bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    bool createImage(uint32_t width, uint32_t height,
            VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkCommandBuffer beginSingleTimeCommands();
    bool endSingleTimeCommands(VkCommandBuffer commandBuffer);
    bool transitionImageLayout(VkImage image, VkFormat format,
            VkImageLayout oldLayout, VkImageLayout newLayout);
    bool copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
};

#endif