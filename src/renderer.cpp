#include "renderer.h"

std::vector<const char*> extensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME
};

std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

bool vk_init(VkContext* vkcontext, void *window){
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Snake";
    appInfo.pEngineName = "No Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
    
    VkResult resultInstance = vkCreateInstance(&createInfo, nullptr, &vkcontext->instance);
    if(resultInstance != VK_SUCCESS){
        return false;
    }

    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkcontext->instance, "vkCreateDebugUtilsMessengerEXT");

    if(vkCreateDebugUtilsMessengerEXT){
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debugInfo.pfnUserCallback = debugCallback;

        vkCreateDebugUtilsMessengerEXT(vkcontext->instance, &debugInfo, nullptr, &vkcontext->debugMessenger);
    }
    else{

    }
    // Create Surface
    {
        VkWin32SurfaceCreateInfoKHR surfaceInfo{};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.hwnd = (HWND)window;
        surfaceInfo.hinstance = GetModuleHandleA(0);

        VkResult resultSurface = vkCreateWin32SurfaceKHR(vkcontext->instance, &surfaceInfo, nullptr, &vkcontext->surface);
        if(resultSurface != VK_SUCCESS){
            return false;
        }
    }

    // Choose GPU
    {
        vkcontext->graphicsIdx = -1;
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(vkcontext->instance, &count, nullptr);
        std::vector<VkPhysicalDevice> gpus(count);
        vkEnumeratePhysicalDevices(vkcontext->instance, &count, gpus.data());

        for(VkPhysicalDevice gpu : gpus){
            uint32_t queueFamilyCount = 0;

            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueProps(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueProps.data());

            for(uint32_t i = 0; i < queueProps.size(); i++){
                if(queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
                    VkBool32 surfaceSupport = VK_FALSE;
                    vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, vkcontext->surface, &surfaceSupport);
                    
                    if(surfaceSupport)
                    {
                        vkcontext->graphicsIdx = i;
                        vkcontext->gpu = gpu;
                    }
                    break;
                }
            }
        }

        if(vkcontext->graphicsIdx < 0){
            return false;
        }
    }


    // Logical Device
    {
        float queuePriority = 1.0f;

        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = vkcontext->graphicsIdx;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        std::vector<const char*> e = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

		VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.enabledExtensionCount = (uint32_t)e.size();
        deviceInfo.ppEnabledExtensionNames = e.data();
        
        
        VkResult result = vkCreateDevice(vkcontext->gpu, &deviceInfo, nullptr, &vkcontext->device);
        if(result != VK_SUCCESS){
            return false;
        }

        vkGetDeviceQueue(vkcontext->device, vkcontext->graphicsIdx, 0, &vkcontext->graphicsQueue);
    }

    // Swapchain
    {
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(vkcontext->gpu, vkcontext->surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(vkcontext->gpu, vkcontext->surface, &formatCount,surfaceFormats.data());

        for(auto surfaceFormat : surfaceFormats){
            if(surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB){
                vkcontext->surfaceFormat = surfaceFormat;
            }
        }


        VkSurfaceCapabilitiesKHR surfaceCapabilities{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkcontext->gpu, vkcontext->surface, &surfaceCapabilities);

        uint32_t imgCount = 0;
        imgCount = surfaceCapabilities.minImageCount + 1;
        imgCount > surfaceCapabilities.maxImageCount ? imgCount-1:imgCount;

        VkSwapchainCreateInfoKHR scInfo{};
        scInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        scInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        scInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        scInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        scInfo.surface = vkcontext->surface;
        scInfo.preTransform = surfaceCapabilities.currentTransform;
        scInfo.imageExtent = surfaceCapabilities.currentExtent;
        scInfo.minImageCount = imgCount;
        scInfo.imageArrayLayers = 1;
        scInfo.imageFormat = vkcontext->surfaceFormat.format;
        vkCreateSwapchainKHR(vkcontext->device, &scInfo, nullptr, &vkcontext->swapchain);


        vkGetSwapchainImagesKHR(vkcontext->device, vkcontext->swapchain, &vkcontext->scImgCount, nullptr);
        vkcontext->scImages.resize(vkcontext->scImgCount);
        vkGetSwapchainImagesKHR(vkcontext->device, vkcontext->swapchain, &vkcontext->scImgCount, vkcontext->scImages.data());
    }

    // Command Pool
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = vkcontext->graphicsIdx;
        vkCreateCommandPool(vkcontext->device, &poolInfo, nullptr, &vkcontext->commandPool);
    }

    // Sync Objects
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(vkcontext->device, &semaphoreInfo, nullptr, &vkcontext->submitSemaphore);
        vkCreateSemaphore(vkcontext->device, &semaphoreInfo, nullptr, &vkcontext->aquireSemaphore);
    }

    return true;
}

bool render(VkContext* vkcontext){
    uint32_t imgIdx = 0;

    vkAcquireNextImageKHR(vkcontext->device, vkcontext->swapchain, 0, vkcontext->aquireSemaphore, 0, &imgIdx);

    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.commandPool = vkcontext->commandPool;

    vkAllocateCommandBuffers(vkcontext->device, &cmdAllocInfo, &cmd);

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cmdBeginInfo);
    // Rendering Commands
    {
        VkClearColorValue clearColor{};
        clearColor.float32[0] = 0.0f;
        clearColor.float32[1] = 0.5f;
        clearColor.float32[2] = 0.0f;
        clearColor.float32[3] = 1.0f;

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;
        vkCmdClearColorImage(cmd, vkcontext->scImages[imgIdx], VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresourceRange);
    }

    vkEndCommandBuffer(cmd);


    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = new VkPipelineStageFlags(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.pSignalSemaphores = &vkcontext->submitSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vkcontext->aquireSemaphore;
    submitInfo.waitSemaphoreCount = 1;
    vkQueueSubmit(vkcontext->graphicsQueue, 1, &submitInfo, 0);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vkcontext->swapchain;
    presentInfo.pImageIndices = &imgIdx;
    presentInfo.pWaitSemaphores = &vkcontext->submitSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    vkQueuePresentKHR(vkcontext->graphicsQueue, &presentInfo);
    
    vkDeviceWaitIdle(vkcontext->device);

    vkFreeCommandBuffers(vkcontext->device, vkcontext->commandPool, 1, &cmd);
    return true;
}