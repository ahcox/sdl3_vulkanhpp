#define VK_USE_PLATFORM_XCB_KHR
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>
// #include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <iostream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace {

    const char validation_layer_name[] {"VK_LAYER_KHRONOS_validation"};

    /// The window we'll open to show our rendering inside.
    SDL_Window* window {nullptr};
    const vk::AllocationCallbacks* g_allocators {nullptr};
    const VkAllocationCallbacks*   g_allocators_raw {nullptr};
    VkSurfaceKHR surface {VK_NULL_HANDLE};

    // RAII Vulkan objects
    std::unique_ptr<vk::raii::Context> g_context;
    std::unique_ptr<vk::raii::Instance> g_instance;
    std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> g_debug_messenger;
    std::unique_ptr<vk::raii::PhysicalDevice> g_physical_device;
    std::unique_ptr<vk::raii::SurfaceKHR> g_surface;

    /// @brief When you have a vector of strings and you need a vector of const char*,
    /// call me baby. 
    /// @return A vector of const char* that are the c_str() of the input strings,
    /// so pointing into the memory owned by the input.
    /// @note The returned vector is only valid as long as the input vector is valid.  
    std::vector<const char*> convert_to_c_ctrs(const std::vector<std::string> &v)
    {
        std::vector<const char*> result;
        for(const auto &s : v)
        {
            result.push_back(s.c_str());
        }
        return result;
    }

    // Deduplicate strings, reordering the input vector.
    template <typename T>
    void deduplicate(std::vector<T> &v)
    {
        std::sort(v.begin(), v.end());
        auto new_end = std::unique(v.begin(), v.end());
        v.resize(new_end - v.begin());
    }

    /// Look for a layer name in a set of layer properties:
    bool find_layer_name(const char * const layer_name, const std::vector<vk::LayerProperties> &layer_properties)
    {
        for(const auto &layer_property : layer_properties)
        {
            if(strcmp(layer_name, layer_property.layerName) == 0)
            {
                return true;
            }
        }
        return false;
    }

    std::vector<uint32_t> get_presentable_graphics_queue_families(const std::vector<vk::QueueFamilyProperties> &queue_family_properties, const vk::PhysicalDevice &physical_device, const vk::SurfaceKHR& surface)
    {
        std::vector<uint32_t> result;
        for(uint32_t i = 0; i < queue_family_properties.size(); ++i)
        {
            const vk::QueueFlags flags = queue_family_properties[i].queueFlags;
            if(flags & vk::QueueFlagBits::eGraphics)
            {
                if(physical_device.getSurfaceSupportKHR(i, surface))
                {
                    result.push_back(i);
                }
            }
        }
        return result;
    }
}

extern "C" {

/// @brief Callback function for Vulkan debug messages.
/// @see VkDebugUtilsMessengerCallbackEXT, PFN_vkDebugUtilsMessengerCallbackEXT
VkBool32 myVkDebugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
    void*                                            pUserData)
{
    std::cerr << "Vulkan Debug Utils Message: severity = " << messageSeverity << ", types =  " << messageTypes;
    if(pCallbackData)
    {
        std::cerr << ", message = \"" << pCallbackData->pMessage << "\"";
    }
    std::cerr << std::endl;
    return VK_FALSE;
}

int SDL_AppInit(void **appstate, int argc, char **argv)
{
    std::cerr << "SDL_AppInit" << std::endl;
    try {
        int result {0};
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr {nullptr};
        Uint32 extension_count {0};
        char const* const* sdl_req_instance_extensions {nullptr};

        if(result = SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
        {
            std::cerr << "SDL_InitSubSystem failed with code " << result << std::endl;
            goto error_exit;
        }
        SDL_SetEventEnabled(SDL_EVENT_TERMINATING, SDL_TRUE);
        SDL_SetEventEnabled(SDL_EVENT_DID_ENTER_BACKGROUND, SDL_TRUE);
        SDL_SetEventEnabled(SDL_EVENT_DID_ENTER_FOREGROUND, SDL_TRUE);
        SDL_SetEventEnabled(SDL_EVENT_KEY_DOWN, SDL_TRUE);
        SDL_SetEventEnabled(SDL_EVENT_KEY_UP, SDL_TRUE);
        SDL_SetEventEnabled(SDL_EVENT_MOUSE_MOTION, SDL_TRUE);

        /// @todo Review the docs for this function and work out how in the name of heck I want to be getting my vulkan functions so that the vulkan.hpp wrappers work and SDL3 works.
        /// @todo This may not be needed since we pass null. Won't SDL_CreateWindow do this for us?
        if(result = SDL_Vulkan_LoadLibrary(nullptr) < 0)
        {
            std::cerr << "SDL_Vulkan_LoadLibrary failed with code " << result << std::endl;
            goto error_exit;
        }

        window = SDL_CreateWindow("SDL3 Vulkan.hpp Clear", 960, 540, SDL_WINDOW_VULKAN);
        if( window == NULL )
        {
            std::cerr << "SDL_CreateWindow failed" << std::endl;
            goto error_exit;
        }

        vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) SDL_Vulkan_GetVkGetInstanceProcAddr();
        if(vkGetInstanceProcAddr == nullptr)
        {
            std::cerr << "SDL_Vulkan_GetVkGetInstanceProcAddr failed" << std::endl;
            goto error_exit;
        }

        // SDL might require certain instance extensions to integrate with particular platforms, so we need to query those.
        sdl_req_instance_extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);
        // Let's bung the debug utilities extension in there too as this is not a high performance app:
        std::vector<const char *> instance_extensions(sdl_req_instance_extensions, sdl_req_instance_extensions + extension_count);
        instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        // Create Vulkan objects:

        g_context.reset(new vk::raii::Context);

        // Create an Instance:
        vk::ApplicationInfo app_info("SDL3 Vulkan.hpp Clear", 1, "Raw Vulkan", 1, VK_API_VERSION_1_0);
        std::vector<const char*> layers;
        auto layer_properties { g_context->enumerateInstanceLayerProperties() };
        if(!find_layer_name(validation_layer_name, layer_properties))
        {
            layers.emplace_back(validation_layer_name);
        }
        vk::InstanceCreateInfo instance_create({}, &app_info, layers.size(), layers.data(), instance_extensions.size(), instance_extensions.data());
        /// @todo Extend the InstanceCreateInfo chain with debug message callback creation.
        g_instance.reset(new vk::raii::Instance(*g_context, instance_create, g_allocators));

        vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info(
            {}, // Reserved for future use: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerCreateInfoEXT.html
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
            myVkDebugUtilsMessengerCallback
        );
        g_debug_messenger.reset(new vk::raii::DebugUtilsMessengerEXT(*g_instance, debug_messenger_create_info));

        if(SDL_TRUE != SDL_Vulkan_CreateSurface(window, **g_instance, g_allocators_raw, &surface))
        {
            std::cerr << "SDL_Vulkan_CreateSurface failed" << std::endl;
            goto error_exit;
        }
        g_surface.reset(new vk::raii::SurfaceKHR( *g_instance, surface ));

        auto physical_devices { g_instance->enumeratePhysicalDevices() };
        if(physical_devices.empty())
        {
            std::cerr << "No physical devices found" << std::endl;
            goto error_exit;
        }
        vk::raii::PhysicalDevice physical_device = physical_devices.front();
        /// @todo Choose the most appropriate available PhysicalDevice for this clear example (integrated > discrete > virtual > cpu) 
        g_physical_device.reset(new vk::raii::PhysicalDevice(physical_device));
        auto physical_properties = g_physical_device->getProperties();
        std::cerr << "Running on GPU: " << physical_properties.deviceName << std::endl;

        // Find a queue family that supports graphics and present:
        std::vector<vk::QueueFamilyProperties> queue_family_properties = g_physical_device->getQueueFamilyProperties();
        /// @todo The next line SEGVs on Ubuntu 23.10. There was probably some init to set the vkGetPhysicalDeviceSurfaceSupportKHR function pointer of the dispatcher in use that was missed. Check the docs.
        /// @todo Uncomment and fix: std::vector<uint32_t> queue_families { get_presentable_graphics_queue_families(queue_family_properties, **g_physical_device, **g_surface) };
    }
    catch (const vk::SystemError &e)
    {
        std::cerr << "vk::SystemError: " << e.what() << std::endl;
        goto error_exit;
    }
    catch (const vk::LogicError &e)
    {
        std::cerr << "vk::LogicError: " << e.what() << std::endl;
        goto error_exit;
    }
    catch(const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        goto error_exit;
    }
    catch(...)
    {
        std::cerr << "Unknown exception" << std::endl;
        goto error_exit;
    }

    return 0;

error_exit:
    std::cerr << "Last SDL error: " << SDL_GetError() << std::endl;
    return -1;
}

int SDL_AppIterate(void *appstate)
{
    static thread_local int i = 0;
    ++i;
    return 0;
}

int SDL_AppEvent(void *appstate, const SDL_Event *event)
{
    std::cerr << "SDL_AppEvent";
    if(event)
    {
        std::cerr << ": type = " << event->type << ", timestamp = " << event->common.timestamp << std::endl;
    }
    std::cerr << std::endl;
    if(event->type == SDL_EVENT_QUIT)
    {
        std::cerr << "SDL_EVENT_QUIT" << std::endl;
        return 1;
    }
    return 0;
}

void SDL_AppQuit(void *appstate)
{
    std::cerr << "SDL_AppQuit" << std::endl;
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
}

}