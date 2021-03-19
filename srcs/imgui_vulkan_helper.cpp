#include <stdio.h>
#include <vector>
#include <set>
#include <algorithm>
#include <array>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "imgui_vulkan_helper.h"

#define HELPER_NAME         "GLFW Vulkan Helper"
#define HELPER_VERSION      VK_MAKE_VERSION(0, 1, 0)
#define BAD_MEMORY_TYPE     0xFFFFFFFF

#define CHECK_RET(exp)      if ((exp) == false) return false;

#ifdef DEBUG
const std::vector<const char*> validationLayersRequired = {
    "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_LUNARG_object_tracker",
    "VK_LAYER_LUNARG_core_validation",
    "VK_LAYER_LUNARG_standard_validation"
};
#else
const std::vector<const char*> validationLayersRequired;
#endif

const std::vector<const char*> deviceExtensionsRequired = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error 0x%x: %s\n", error, description);
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<ImguiVulkanHelper *>(glfwGetWindowUserPointer(window));
    app->setFramebufferResized();
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
    fprintf(stderr, "Validation layer: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

bool ImguiVulkanHelper::initWindow(int width, int height, const char *title)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit() != GLFW_TRUE) {
        fprintf(stderr, "glfwInit failed, aborting.\n");
        return false;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == nullptr) {
        fprintf(stderr, "glfwCreateWindow failed. Width: %d, height: %d\n", width, height);
        return false;
    }
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    return true;
}

void ImguiVulkanHelper::setFramebufferResized(void)
{
    framebufferResized = true;
}

bool ImguiVulkanHelper::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayersRequired) {
        bool layerFound = false;
        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            fprintf(stderr, "Validation layer: %s is not supported.\n", layerName);
            return false;
        }
    }
    return true;
}

SwapChainSupportDetails ImguiVulkanHelper::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

QueueFamilyIndices ImguiVulkanHelper::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;
    indices.init();

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
            indices.presentFamily = i;

        if (indices.isComplete())
            break;
        i++;
    }

    return indices;
}

bool ImguiVulkanHelper::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    for (const char *ext_need: deviceExtensionsRequired) {
        bool found = false;
        for (const auto& ext_available: availableExtensions) {
            if (!strncmp(ext_need, ext_available.extensionName, strlen(ext_need))) {
                found = true;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "Device extension: %s is not supported.\n", ext_need);
            return false;
        }
    }
    return true;
}

bool ImguiVulkanHelper::isDeviceSuitable(VkPhysicalDevice device)
{
    /* TODO: before digging queue families and other stuffs, we can also check
     * some other device properties or features, e.g: VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
     */
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    // Anisotropy is to handle the undersampling(more texels than fragments)
    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool ImguiVulkanHelper::createInstance(const char *app_name, uint32_t app_version)
{
    VkResult ret = VK_SUCCESS;
    uint32_t glfw_required_ext_count = 0;
    const char** glfw_required_exts = glfwGetRequiredInstanceExtensions(&glfw_required_ext_count);
    std::vector<const char*> extensions(glfw_required_exts, glfw_required_exts + glfw_required_ext_count);

    /* 1. glfwVulkanSupported is not needed to be called since a lot of other functions, e.g:
     *    glfwGetRequiredInstanceExtensions calls it implicitly
     * 2. VK_EXT_debug_report is deprecated, replaced by VK_EXT_debug_utils instance extension
     *    We will enable this extension here.
     */
    if (validationLayersRequired.size() > 0 && !checkValidationLayerSupport())
        return false;

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = app_name;
    appInfo.applicationVersion = app_version;
    appInfo.pEngineName = HELPER_NAME;
    appInfo.engineVersion = HELPER_VERSION;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (validationLayersRequired.size() > 0)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo1{};
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo2{};
    debugCreateInfo1.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo1.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo1.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo1.pfnUserCallback = debugCallback;
    memcpy(&debugCreateInfo2, &debugCreateInfo1, sizeof(debugCreateInfo2));

    if (validationLayersRequired.size() > 0) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersRequired.size());
        createInfo.ppEnabledLayerNames = validationLayersRequired.data();
        // This debugCreateInfo1 is for debugging vkCreateInstance and vkDestroyInstance
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo1;
        fprintf(stdout, "Create instance: validation layer enabled.\n");
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    ret = vkCreateInstance(&createInfo, nullptr, &instance);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vulkan instance: %d\n", ret);
        return false;
    }

    // This is to use our debug callback when validation layer outputs
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        ret = func(instance, &debugCreateInfo2, nullptr, &debugMessenger);
        if (ret != VK_SUCCESS) {
            fprintf(stderr, "Create debug utils messenger failed: %d\n", ret);
            return false;
        }
    } else {
        fprintf(stderr, "The function pointer: vkCreateDebugUtilsMessengerEXT is not found.\n");
        return false;
    }

    return true;
}

bool ImguiVulkanHelper::createDevice(void)
{
    VkResult ret = VK_SUCCESS;

    // We need to create the surface early because we need it to choose GPU, queue families
    ret = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "glfwCreateWindowSurface failed: %d\n", ret);
        return false;
    }

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        fprintf(stderr, "Failed to find GPUs with Vulkan support.\n");
        return false;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to find a suitable GPU.\n");
        return false;
    }

    QueueFamilyIndices queue_families = findQueueFamilies(physicalDevice);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    // no duplicated queue families should be passed to vkCreateDevice, so use a set here
    std::set<int> qf_remove_duplicates = {queue_families.graphicsFamily, queue_families.presentFamily};
    for (auto queue_family: qf_remove_duplicates) {
        VkDeviceQueueCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = queue_family;
        info.queueCount = 1;
        info.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(info);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensionsRequired.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionsRequired.data();
    if (validationLayersRequired.size() > 0) {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersRequired.size());
        deviceCreateInfo.ppEnabledLayerNames = validationLayersRequired.data();
        fprintf(stdout, "Create device: validation layer enabled.\n");
    } else {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    ret = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Create logical device failed: %d\n", ret);
        return false;
    }
    vkGetDeviceQueue(device, queue_families.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queue_families.presentFamily, 0, &presentQueue);

    return true;
}

VkSurfaceFormatKHR ImguiVulkanHelper::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR ImguiVulkanHelper::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
#ifdef VSYNC
        return VK_PRESENT_MODE_FIFO_KHR;
#else
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
#endif
}

VkExtent2D ImguiVulkanHelper::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

bool ImguiVulkanHelper::createSwapChain(void)
{
    VkResult ret = VK_SUCCESS;
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {
        static_cast<uint32_t>(indices.graphicsFamily),
        static_cast<uint32_t>(indices.presentFamily)
    };
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    ret = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Creating swapchain failed: %d\n", ret);
        return false;
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
    this->imageCount = imageCount;
    fprintf(stdout, "Create swapchain, image count: %u\n", imageCount);
    return true;
}

VkImageView ImguiVulkanHelper::createImageView(VkImage image, VkFormat format)
{
    VkResult ret = VK_SUCCESS;
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    ret = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Creating image view failed: %d\n", ret);
        return VK_NULL_HANDLE;
    }
    return imageView;
}

bool ImguiVulkanHelper::createImageViews(void)
{
    swapChainImageViews.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
        if (swapChainImageViews[i] == VK_NULL_HANDLE)
            return false;
    }
    return true;
}

bool ImguiVulkanHelper::createRenderPass(void)
{
    VkResult ret = VK_SUCCESS;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    ret = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Creating RenderPass failed: %d\n", ret);
        return false;
    }
    return true;
}

bool ImguiVulkanHelper::createFramebuffers(void)
{
    VkResult ret = VK_SUCCESS;

    // Framebuffer bounds with ImageViews, but the count of the ImageViews
    // is same with the count of Images, so using `imageCount' here
    swapChainFramebuffers.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        ret = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
        if (ret != VK_SUCCESS) {
            fprintf(stderr, "Creating framebuffer failed: %d\n", ret);
            return false;
        }
    }
    return true;
}

bool ImguiVulkanHelper::createCommandPool(void)
{
    VkResult ret = VK_SUCCESS;
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = static_cast<uint32_t>(queueFamilyIndices.graphicsFamily);

    ret = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Creating command pool failed: %d\n", ret);
        return false;
    }
    return true;
}

bool ImguiVulkanHelper::createDescriptorPool(void)
{
    VkResult ret = VK_SUCCESS;

    // TODO, we need more descriptors here
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(imageCount);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(imageCount);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(imageCount);

    ret = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Creating descriptor pool failed: %d\n", ret);
        return false;
    }
    return true;
}

bool ImguiVulkanHelper::createCommandBuffers(void)
{
    VkResult ret = VK_SUCCESS;

    commandBuffers.resize(swapChainFramebuffers.size());
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    ret = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Creating command buffers failed: %d\n", ret);
        return false;
    }
    return true;
}

bool ImguiVulkanHelper::createSyncObjects(void)
{
    VkResult ret = VK_SUCCESS;

    imageAvailableSemaphores.resize(imageCount);
    renderFinishedSemaphores.resize(imageCount);
    swapChainImageFences.resize(imageCount);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < imageCount; i++) {
        ret = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
        if (ret != VK_SUCCESS) {
            fprintf(stderr, "Creating image available semaphore failed: %d\n", ret);
            return false;
        }
        ret = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
        if (ret != VK_SUCCESS) {
            fprintf(stderr, "Creating render finished semaphore failed: %d\n", ret);
            return false;
        }
        ret = vkCreateFence(device, &fenceInfo, nullptr, &swapChainImageFences[i]);
        if (ret != VK_SUCCESS) {
            fprintf(stderr, "Creating fence failed: %d\n", ret);
            return false;
        }
    }
    return true;
}

bool ImguiVulkanHelper::initVulkan(const char *app_name, uint32_t app_version)
{
    CHECK_RET(createInstance(app_name, app_version));
    CHECK_RET(createDevice());
    CHECK_RET(createSwapChain());
    CHECK_RET(createImageViews());
    CHECK_RET(createRenderPass());
    CHECK_RET(createFramebuffers());
    CHECK_RET(createCommandPool());
    CHECK_RET(createDescriptorPool());
    CHECK_RET(createCommandBuffers());
    CHECK_RET(createSyncObjects());

    return true;
}

void ImguiVulkanHelper::cleanupSwapChain(void)
{
    for (auto fb: swapChainFramebuffers)
        vkDestroyFramebuffer(device, fb, nullptr);
    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    for (auto imageView : swapChainImageViews)
        vkDestroyImageView(device, imageView, nullptr);
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void ImguiVulkanHelper::cleanup(void)
{
    if (terminated)
        return;
    fprintf(stdout, "ImguiVulkanHelper is terminating...\n");

    cleanupSwapChain();

    for (size_t i = 0; i < userTextureImages.size(); i++) {
        vkDestroySampler(device, userTextureImages[i].sampler, nullptr);
        vkDestroyImageView(device, userTextureImages[i].imageView, nullptr);
        vkDestroyImage(device, userTextureImages[i].image, nullptr);
        vkFreeMemory(device, userTextureImages[i].imageMemory, nullptr);
    }
    userTextureImages.clear();

    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    for (size_t i = 0; i < imageCount; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, swapChainImageFences[i], nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    if (validationLayersRequired.size() > 0) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(instance, debugMessenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
    if (window != nullptr)
        glfwDestroyWindow(window);
    glfwTerminate();

    terminated = true;
}

ImguiVulkanHelper::~ImguiVulkanHelper(void)
{
    cleanup();
}

GLFWwindow* ImguiVulkanHelper::getWindow(void)
{
    return window;
}

static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[Vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void ImguiVulkanHelper::fillImguiVulkanInitInfo(ImGui_ImplVulkan_InitInfo *info)
{
    if (info == nullptr)
        return;
    info->Instance = instance;
    info->PhysicalDevice = physicalDevice;
    info->Device = device;

    // Current imgui vulkan implementation assumes graphics queue = present queue
    auto queue_families = findQueueFamilies(physicalDevice);
    info->QueueFamily = queue_families.graphicsFamily;
    info->Queue = graphicsQueue;

    info->PipelineCache = VK_NULL_HANDLE;
    info->DescriptorPool = descriptorPool;
    info->Allocator = nullptr;
    info->MinImageCount = imageCount;
    info->ImageCount = imageCount;
    info->CheckVkResultFn = check_vk_result;
}

VkRenderPass ImguiVulkanHelper::getRenderPass(void)
{
    return renderPass;
}

bool ImguiVulkanHelper::initializeFontTexture(void)
{
    VkResult ret = VK_SUCCESS;

    ret = vkResetCommandPool(device, commandPool, 0);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Reset command pool failed: %d\n", ret);
        return false;
    }

    VkCommandBuffer cmd = beginSingleTimeCommands();
    if (cmd == nullptr)
        goto fail;
    if (!ImGui_ImplVulkan_CreateFontsTexture(cmd)) {
        fprintf(stderr, "ImGui_ImplVulkan_CreateFontsTexture failed.\n");
        goto fail;
    }
    if (!endSingleTimeCommands(cmd)) {
        ImGui_ImplVulkan_DestroyFontUploadObjects();
        return false;
    }

    ImGui_ImplVulkan_DestroyFontUploadObjects();
    return true;

fail:
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    return false;
}

VkDevice ImguiVulkanHelper::getDevice(void)
{
    return device;
}

void ImguiVulkanHelper::recreateSwapChain(void)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    /* RenderPass can be reused so don't create it here
     * That's also why we don't have to call ImGui_ImplVulkan_Init again
     * Pipeline can be reused as well because nothing needs to be changed in it.
     * There is viewport setting when creating the pipeline, but imgui uses
     * set viewport command in the command buffer to fix this (so in imgui pipeline
     * creation, it doesn't set the width/height of the viewport).
     * So unless we need to change something like vertex binding/attributes, color formats,
     * we don't have to recreate the pipeline, which is also good for performance.
     */
    createSwapChain();
    createImageViews();
    createFramebuffers();
    createCommandBuffers();
}

uint32_t ImguiVulkanHelper::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;

    fprintf(stderr, "failed to find suitable memory type!\n");
    return BAD_MEMORY_TYPE;
}

bool ImguiVulkanHelper::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkResult ret = VK_SUCCESS;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ret = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "failed to create buffer: %d\n", ret);
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    if (allocInfo.memoryTypeIndex == BAD_MEMORY_TYPE)
        return false;
    ret = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
    if (ret != VK_SUCCESS) {
        vkDestroyBuffer(device, buffer, nullptr);
        fprintf(stderr, "Allocate buffer memory failed: %d\n", ret);
        return false;
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
    return true;
}

bool ImguiVulkanHelper::createImage(uint32_t width, uint32_t height,
            VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkResult ret = VK_SUCCESS;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ret = vkCreateImage(device, &imageInfo, nullptr, &image);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Failed to create image: %d\n", ret);
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    if (allocInfo.memoryTypeIndex == BAD_MEMORY_TYPE)
        return false;
    ret = vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
    if (ret != VK_SUCCESS) {
        vkDestroyImage(device, image, nullptr);
        fprintf(stderr, "Failed to allocate image memory: %d\n", ret);
        return false;
    }

    vkBindImageMemory(device, image, imageMemory, 0);
    return true;
}

VkCommandBuffer ImguiVulkanHelper::beginSingleTimeCommands()
{
    VkResult ret = VK_SUCCESS;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    ret = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Allocate command buffer failed: %d\n", ret);
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    ret = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (ret != VK_SUCCESS) {
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        fprintf(stderr, "Begin command buffer failed: %d\n", ret);
        return VK_NULL_HANDLE;
    }

    return commandBuffer;
}

bool ImguiVulkanHelper::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    VkResult ret = VK_SUCCESS;
    bool result = false;
    VkSubmitInfo submitInfo{};

    ret = vkEndCommandBuffer(commandBuffer);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "End command buffer failed: %d\n", ret);
        goto out;
    }

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    ret = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Submit command buffer to graphics queue failed: %d\n", ret);
        goto out;
    }
    ret = vkQueueWaitIdle(graphicsQueue);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Wait graphics queue idle failed: %d\n", ret);
        goto out;
    }
    result = true;

out:
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    return result;
}

bool ImguiVulkanHelper::transitionImageLayout(VkImage image, VkFormat format,
            VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    if (commandBuffer == nullptr) {
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        return false;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }else {
        fprintf(stderr, "Unsupported layout transition!\n");
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        return false;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    return endSingleTimeCommands(commandBuffer);
}

bool ImguiVulkanHelper::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    if (commandBuffer == nullptr) {
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        return false;
    }

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    return endSingleTimeCommands(commandBuffer);
}

ImTextureID ImguiVulkanHelper::loadImage(const char *image, int *width, int *height)
{
    ImTextureID result = NULL;
    VkResult ret = VK_SUCCESS;
    VkSamplerCreateInfo samplerInfo{};

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(image, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        fprintf(stderr, "Failed to load image: %s\n", image);
        return NULL;
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    if (!createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      stagingBuffer, stagingBufferMemory))
        return NULL;

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    stbi_image_free(pixels);

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    if (!createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory))
        goto out;

    if (!transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
        goto out;
    if (!copyBufferToImage(stagingBuffer, textureImage,
            static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)))
        goto out;
    if (!transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
        goto out;

    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
    if (textureImageView == VK_NULL_HANDLE)
        goto out;

    VkSampler textureSampler;
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    ret = vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Failed to create texture sampler: %d\n", ret);
        goto out;
    }

    result = ImGui_ImplVulkan_AddTexture(textureSampler, textureImageView,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (result == NULL) {
        fprintf(stderr, "Failed to add user texture to ImplVulkan.\n");
        goto out;
    }

    UserTextureImage uti;
    uti.image = textureImage;
    uti.imageMemory = textureImageMemory;
    uti.imageView = textureImageView;
    uti.sampler = textureSampler;
    userTextureImages.push_back(uti);

    if (width)
        *width = texWidth;
    if (height)
        *height = texHeight;

out:
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    if (textureImageView != VK_NULL_HANDLE && !result)
        vkDestroyImageView(device, textureImageView, nullptr);
    if (textureImage != VK_NULL_HANDLE && !result)
        vkDestroyImage(device, textureImage, nullptr);
    if (textureImageMemory != VK_NULL_HANDLE && !result)
        vkFreeMemory(device, textureImageMemory, nullptr);
    if (textureSampler != VK_NULL_HANDLE && !result)
        vkDestroySampler(device, textureSampler, nullptr);
    return result;
}

void ImguiVulkanHelper::drawFrame(ImDrawData *data)
{
    VkResult ret = VK_SUCCESS;
    uint32_t imageIndex;

    ret = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (ret == VK_ERROR_OUT_OF_DATE_KHR) {
        fprintf(stdout, "Acquire image out of date, recreate the swapchain.\n");
        recreateSwapChain();
        // Once swapchain is recreated, the imageIndex becomes 0 always, so currentFrame must match it.
        // See wiznote for the detailed explanation.
        currentFrame = 0;
        return;
    } else if (ret != VK_SUCCESS && ret != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "Acquire next image from swapchain failed: %d\n", ret);
        return;
    }

    vkWaitForFences(device, 1, &swapChainImageFences[currentFrame], VK_TRUE, UINT64_MAX);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    ret = vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo);
    if (ret != VK_SUCCESS) {
        fprintf(stderr, "Begin command buffer failed: %d\n", ret);
        goto out;
    }

    {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[currentFrame];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        ImGui_ImplVulkan_RenderDrawData(data, commandBuffers[currentFrame]);

        vkCmdEndRenderPass(commandBuffers[currentFrame]);
        ret = vkEndCommandBuffer(commandBuffers[currentFrame]);
        if (ret != VK_SUCCESS) {
            fprintf(stderr, "End command buffer failed: %d\n", ret);
            goto out;
        }
    }

    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(device, 1, &swapChainImageFences[currentFrame]);
        ret = vkQueueSubmit(graphicsQueue, 1, &submitInfo, swapChainImageFences[currentFrame]);
        if (ret != VK_SUCCESS) {
            fprintf(stderr, "Submitting render command to queue failed: %d\n", ret);
            goto out;
        }
    }

    // Create a new scope to avoid the build warning C4533 (goto skips the initialization of presentInfo)
    {
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        ret = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_SUBOPTIMAL_KHR || framebufferResized) {
            fprintf(stdout, "Queue present image out of date or framebuffer resized, recreate the swapchain.\n");
            framebufferResized = false;
            recreateSwapChain();
            // Once swapchain is recreated, the imageIndex becomes 0 always, so currentFrame must match it.
            // See wiznote for the detailed explanation.
            currentFrame = 0;
            return;
        } else if (ret != VK_SUCCESS) {
            fprintf(stderr, "Present the image failed: %d\n", ret);
            goto out;
        }
    }

out:
    currentFrame = (currentFrame + 1) % imageCount;
}
