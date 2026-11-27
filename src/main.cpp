// My Sun Simulation — Vulkan fullscreen procedural Sun
// Renders a realistic Sun with sunspots, flares, granulation, and solar storms.
// Uses a fullscreen triangle — no vertex buffers needed.

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
#define VK_CHECK(call, msg)                                                    \
    do {                                                                       \
        VkResult _r = (call);                                                  \
        if (_r != VK_SUCCESS) {                                                \
            fprintf(stderr, "%s failed: VkResult = %d\n", msg, (int)_r);       \
            std::abort();                                                      \
        }                                                                      \
    } while (0)

static std::vector<char> readFile(const std::string& path) {
    std::vector<std::string> candidates = {
        path,
        "../" + path,
        "/home/aditya/My-Sun-Simulation/" + path,
        "/home/aditya/My-Sun-Simulation/build/" + path
    };
    std::ifstream f;
    for (const auto& c : candidates) {
        f.open(c, std::ios::ate | std::ios::binary);
        if (f.is_open()) break;
    }
    if (!f.is_open())
        throw std::runtime_error("Cannot open file: " + path);
    size_t sz = (size_t)f.tellg();
    std::vector<char> buf(sz);
    f.seekg(0);
    f.read(buf.data(), sz);
    return buf;
}

// Push‑constant block passed to the fragment shader every frame.
struct PushConstants {
    float time;
    float resX;
    float resY;
    float camX;
    float camY;
    float camZ;
    float yaw;
    float pitch;
};

static float gScrollDelta = 0.0f;
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    gScrollDelta += (float)yoffset;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    // ---- GLFW ----
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return 1; }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "My Sun Simulation", nullptr, nullptr);
    if (!window) { std::cerr << "Window creation failed\n"; glfwTerminate(); return 1; }

    // ---- Instance ----
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName   = "My Sun Simulation";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName        = "None";
    appInfo.apiVersion         = VK_API_VERSION_1_2;

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);

    VkInstanceCreateInfo instCI{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instCI.pApplicationInfo        = &appInfo;
    instCI.enabledExtensionCount   = glfwExtCount;
    instCI.ppEnabledExtensionNames = glfwExts;

    VkInstance instance;
    VK_CHECK(vkCreateInstance(&instCI, nullptr, &instance), "vkCreateInstance");

    // ---- Surface ----
    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface),
             "glfwCreateWindowSurface");

    // ---- Physical device ----
    uint32_t devCount = 0;
    vkEnumeratePhysicalDevices(instance, &devCount, nullptr);
    if (devCount == 0) { std::cerr << "No Vulkan GPUs\n"; return 1; }
    std::vector<VkPhysicalDevice> physDevs(devCount);
    vkEnumeratePhysicalDevices(instance, &devCount, physDevs.data());
    VkPhysicalDevice physDev = physDevs[0];

    // ---- Queue family ----
    uint32_t qfCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, nullptr);
    std::vector<VkQueueFamilyProperties> qfProps(qfCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, qfProps.data());

    uint32_t gfxFamily = UINT32_MAX, presFamily = UINT32_MAX;
    for (uint32_t i = 0; i < qfCount; ++i) {
        if (qfProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) gfxFamily = i;
        VkBool32 ps = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physDev, i, surface, &ps);
        if (ps) presFamily = i;
        if (gfxFamily != UINT32_MAX && presFamily != UINT32_MAX) break;
    }
    if (gfxFamily == UINT32_MAX || presFamily == UINT32_MAX) {
        std::cerr << "No suitable queue families\n"; return 1;
    }

    // ---- Logical device ----
    float prio = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> qCIs;
    {
        VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        q.queueFamilyIndex = gfxFamily; q.queueCount = 1; q.pQueuePriorities = &prio;
        qCIs.push_back(q);
    }
    if (presFamily != gfxFamily) {
        VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        q.queueFamilyIndex = presFamily; q.queueCount = 1; q.pQueuePriorities = &prio;
        qCIs.push_back(q);
    }

    const char* devExts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceFeatures feat{};
    VkDeviceCreateInfo devCI{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    devCI.queueCreateInfoCount    = (uint32_t)qCIs.size();
    devCI.pQueueCreateInfos       = qCIs.data();
    devCI.enabledExtensionCount   = 1;
    devCI.ppEnabledExtensionNames = devExts;
    devCI.pEnabledFeatures        = &feat;

    VkDevice device;
    VK_CHECK(vkCreateDevice(physDev, &devCI, nullptr, &device), "vkCreateDevice");

    VkQueue gfxQueue, presQueue;
    vkGetDeviceQueue(device, gfxFamily,  0, &gfxQueue);
    vkGetDeviceQueue(device, presFamily, 0, &presQueue);

    // ---- Query surface capabilities ----
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, surface, &caps);

    // ---- Pick surface format (prefer B8G8R8A8_UNORM, fall back to first) ----
    uint32_t fmtCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &fmtCount, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fmtCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &fmtCount, fmts.data());
    VkSurfaceFormatKHR chosenFmt = fmts[0]; // safe default
    for (auto& f : fmts) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFmt = f; break;
        }
    }

    // ---- Pick present mode (prefer FIFO — always available) ----
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    // ---- Pick composite alpha (first supported flag) ----
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    for (auto ca : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                    VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                    VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR}) {
        if (caps.supportedCompositeAlpha & ca) { compositeAlpha = ca; break; }
    }

    // ---- Determine extent ----
    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        extent.width  = std::clamp((uint32_t)w, caps.minImageExtent.width,  caps.maxImageExtent.width);
        extent.height = std::clamp((uint32_t)h, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    // ---- Image count ----
    uint32_t imgCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imgCount > caps.maxImageCount)
        imgCount = caps.maxImageCount;

    // ---- Create swapchain ----
    VkSwapchainCreateInfoKHR scCI{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    scCI.surface          = surface;
    scCI.minImageCount    = imgCount;
    scCI.imageFormat      = chosenFmt.format;
    scCI.imageColorSpace  = chosenFmt.colorSpace;
    scCI.imageExtent      = extent;
    scCI.imageArrayLayers = 1;
    scCI.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t families[] = {gfxFamily, presFamily};
    if (gfxFamily != presFamily) {
        scCI.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        scCI.queueFamilyIndexCount = 2;
        scCI.pQueueFamilyIndices   = families;
    } else {
        scCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    scCI.preTransform   = caps.currentTransform;
    scCI.compositeAlpha = compositeAlpha;
    scCI.presentMode    = presentMode;
    scCI.clipped        = VK_TRUE;

    VkSwapchainKHR swapchain;
    VK_CHECK(vkCreateSwapchainKHR(device, &scCI, nullptr, &swapchain),
             "vkCreateSwapchainKHR");

    // ---- Swapchain images ----
    vkGetSwapchainImagesKHR(device, swapchain, &imgCount, nullptr);
    std::vector<VkImage> scImages(imgCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imgCount, scImages.data());

    // ---- Image views ----
    std::vector<VkImageView> scViews(imgCount);
    for (uint32_t i = 0; i < imgCount; ++i) {
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vi.image    = scImages[i];
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format   = chosenFmt.format;
        vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VK_CHECK(vkCreateImageView(device, &vi, nullptr, &scViews[i]), "ImageView");
    }

    // ---- Render pass ----
    VkAttachmentDescription att{};
    att.format         = chosenFmt.format;
    att.samples        = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    att.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    att.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};
    sub.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments    = &ref;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpCI{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpCI.attachmentCount = 1;  rpCI.pAttachments  = &att;
    rpCI.subpassCount    = 1;  rpCI.pSubpasses    = &sub;
    rpCI.dependencyCount = 1;  rpCI.pDependencies = &dep;
    VkRenderPass renderPass;
    VK_CHECK(vkCreateRenderPass(device, &rpCI, nullptr, &renderPass), "RenderPass");

    // ---- Framebuffers ----
    std::vector<VkFramebuffer> fbs(imgCount);
    for (uint32_t i = 0; i < imgCount; ++i) {
        VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fi.renderPass      = renderPass;
        fi.attachmentCount = 1;
        fi.pAttachments    = &scViews[i];
        fi.width           = extent.width;
        fi.height          = extent.height;
        fi.layers          = 1;
        VK_CHECK(vkCreateFramebuffer(device, &fi, nullptr, &fbs[i]), "Framebuffer");
    }

    // ---- Shader modules ----
    auto vertCode = readFile("shaders/vert.spv");
    auto fragCode = readFile("shaders/frag.spv");
    auto createModule = [&](const std::vector<char>& code) {
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = code.size();
        ci.pCode    = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule m;
        VK_CHECK(vkCreateShaderModule(device, &ci, nullptr, &m), "ShaderModule");
        return m;
    };
    VkShaderModule vertMod = createModule(vertCode);
    VkShaderModule fragMod = createModule(fragCode);

    // ---- Pipeline layout (push constants for time + resolution) ----
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo plCI{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plCI.pushConstantRangeCount = 1;
    plCI.pPushConstantRanges    = &pcRange;
    VkPipelineLayout pipeLayout;
    VK_CHECK(vkCreatePipelineLayout(device, &plCI, nullptr, &pipeLayout), "PipelineLayout");

    // ---- Graphics pipeline (fullscreen triangle, no vertex input) ----
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertMod;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragMod;
    stages[1].pName  = "main";

    VkPipelineVertexInputStateCreateInfo viState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    // No vertex input — fullscreen triangle generated in vertex shader.

    VkPipelineInputAssemblyStateCreateInfo iaState{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    iaState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport vp{0,0,(float)extent.width,(float)extent.height,0,1};
    VkRect2D sc{{0,0},extent};
    VkPipelineViewportStateCreateInfo vpState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vpState.viewportCount = 1; vpState.pViewports = &vp;
    vpState.scissorCount  = 1; vpState.pScissors  = &sc;

    VkPipelineRasterizationStateCreateInfo rsState{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rsState.polygonMode = VK_POLYGON_MODE_FILL;
    rsState.cullMode    = VK_CULL_MODE_NONE;         // important for fullscreen tri
    rsState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rsState.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo msState{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    msState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cbAtt{};
    cbAtt.colorWriteMask = 0xF;
    VkPipelineColorBlendStateCreateInfo cbState{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cbState.attachmentCount = 1;
    cbState.pAttachments    = &cbAtt;

    VkGraphicsPipelineCreateInfo gpCI{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gpCI.stageCount          = 2;
    gpCI.pStages             = stages;
    gpCI.pVertexInputState   = &viState;
    gpCI.pInputAssemblyState = &iaState;
    gpCI.pViewportState      = &vpState;
    gpCI.pRasterizationState = &rsState;
    gpCI.pMultisampleState   = &msState;
    gpCI.pColorBlendState    = &cbState;
    gpCI.layout              = pipeLayout;
    gpCI.renderPass          = renderPass;

    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpCI, nullptr, &pipeline),
             "GraphicsPipeline");

    vkDestroyShaderModule(device, vertMod, nullptr);
    vkDestroyShaderModule(device, fragMod, nullptr);

    // ---- Command pool ----
    VkCommandPoolCreateInfo cpCI{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    cpCI.queueFamilyIndex = gfxFamily;
    cpCI.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool cmdPool;
    VK_CHECK(vkCreateCommandPool(device, &cpCI, nullptr, &cmdPool), "CmdPool");

    // ---- Command buffers (one per swapchain image) ----
    std::vector<VkCommandBuffer> cmdBufs(imgCount);
    VkCommandBufferAllocateInfo cbAI{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbAI.commandPool        = cmdPool;
    cbAI.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAI.commandBufferCount = imgCount;
    VK_CHECK(vkAllocateCommandBuffers(device, &cbAI, cmdBufs.data()), "CmdBufs");

    // ---- Sync objects ----
    VkSemaphore imageAvail, renderDone;
    VkFence     inFlight;
    {
        VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(device, &si, nullptr, &imageAvail), "Sem");
        VK_CHECK(vkCreateSemaphore(device, &si, nullptr, &renderDone), "Sem");
        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(device, &fi, nullptr, &inFlight), "Fence");
    }

    // ---- Setup Controls & Callbacks ----
    glfwSetScrollCallback(window, scroll_callback);

    float camX = 0.0f;
    float camY = 0.0f;
    float camZ = 2.5f;
    float yaw = 0.0f;
    float pitch = 0.0f;

    bool mouseDragging = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;

    double lastTime = glfwGetTime();
    double startTime = lastTime;

    std::cout << "\n============================================\n"
              << "       My Sun Simulation Controls\n"
              << "============================================\n"
              << "  [W] / [S]        : Move forward / backward\n"
              << "  [A] / [D]        : Strafe left / right\n"
              << "  [Q] / [E]        : Move down / up\n"
              << "  [Shift]          : Sprint / Move faster\n"
              << "  [Mouse Drag]     : Orbit & look around\n"
              << "  [Arrow Keys]     : Turn camera\n"
              << "  [Scroll Wheel]   : Zoom in / out\n"
              << "  [R]              : Reset camera view\n"
              << "  [Esc]            : Exit simulation\n"
              << "============================================\n\n";

    // ---- Main loop ----
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f;

        // Camera direction vectors from yaw and pitch
        float cy = std::cos(yaw), sy = std::sin(yaw);
        float cp = std::cos(pitch), sp = std::sin(pitch);
        float fwdX = sy * cp, fwdY = sp, fwdZ = -cy * cp;
        float rgtX = cy, rgtY = 0.0f, rgtZ = sy;
        float upX = rgtY * fwdZ - rgtZ * fwdY;
        float upY = rgtZ * fwdX - rgtX * fwdZ;
        float upZ = rgtX * fwdY - rgtY * fwdX;

        float speed = 2.4f * dt;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 2.5f;

        // WASD Movement
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camX += fwdX * speed; camY += fwdY * speed; camZ += fwdZ * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camX -= fwdX * speed; camY -= fwdY * speed; camZ -= fwdZ * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camX -= rgtX * speed; camY -= rgtY * speed; camZ -= rgtZ * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camX += rgtX * speed; camY += rgtY * speed; camZ += rgtZ * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            camX += upX * speed; camY += upY * speed; camZ += upZ * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            camX -= upX * speed; camY -= upY * speed; camZ -= upZ * speed;
        }

        // Arrow keys for turning
        float rotSpeed = 1.6f * dt;
        if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) yaw -= rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) yaw += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) pitch += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) pitch -= rotSpeed;

        // Mouse Drag Orbit
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            if (!mouseDragging) {
                mouseDragging = true;
                lastMouseX = mx;
                lastMouseY = my;
            } else {
                float dx = (float)(mx - lastMouseX);
                float dy = (float)(my - lastMouseY);
                lastMouseX = mx;
                lastMouseY = my;
                yaw += dx * 0.0035f;
                pitch -= dy * 0.0035f;
            }
        } else {
            mouseDragging = false;
        }

        pitch = std::clamp(pitch, -1.52f, 1.52f);

        // Scroll zoom
        if (gScrollDelta != 0.0f) {
            camX += fwdX * gScrollDelta * 0.35f;
            camY += fwdY * gScrollDelta * 0.35f;
            camZ += fwdZ * gScrollDelta * 0.35f;
            gScrollDelta = 0.0f;
        }

        // Reset view with 'R'
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            camX = 0.0f; camY = 0.0f; camZ = 2.5f;
            yaw = 0.0f; pitch = 0.0f;
        }

        vkWaitForFences(device, 1, &inFlight, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlight);

        uint32_t idx;
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvail,
                              VK_NULL_HANDLE, &idx);

        // Record command buffer
        VkCommandBuffer cb = cmdBufs[idx];
        vkResetCommandBuffer(cb, 0);
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(cb, &bi);

        VkClearValue cv{};
        cv.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        VkRenderPassBeginInfo rpBI{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpBI.renderPass        = renderPass;
        rpBI.framebuffer       = fbs[idx];
        rpBI.renderArea.extent = extent;
        rpBI.clearValueCount   = 1;
        rpBI.pClearValues      = &cv;

        vkCmdBeginRenderPass(cb, &rpBI, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        PushConstants pc{};
        pc.time = (float)(now - startTime);
        pc.resX = (float)extent.width;
        pc.resY = (float)extent.height;
        pc.camX = camX;
        pc.camY = camY;
        pc.camZ = camZ;
        pc.yaw = yaw;
        pc.pitch = pitch;
        vkCmdPushConstants(cb, pipeLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pc), &pc);

        vkCmdDraw(cb, 3, 1, 0, 0);   // fullscreen triangle — 3 verts
        vkCmdEndRenderPass(cb);
        vkEndCommandBuffer(cb);

        // Submit
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.waitSemaphoreCount   = 1; si.pWaitSemaphores   = &imageAvail;
        si.pWaitDstStageMask    = &waitStage;
        si.commandBufferCount   = 1; si.pCommandBuffers   = &cb;
        si.signalSemaphoreCount = 1; si.pSignalSemaphores = &renderDone;
        VK_CHECK(vkQueueSubmit(gfxQueue, 1, &si, inFlight), "QueueSubmit");

        // Present
        VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = &renderDone;
        pi.swapchainCount     = 1; pi.pSwapchains     = &swapchain;
        pi.pImageIndices      = &idx;
        vkQueuePresentKHR(presQueue, &pi);
    }

    // ---- Cleanup ----
    vkDeviceWaitIdle(device);
    vkDestroySemaphore(device, imageAvail, nullptr);
    vkDestroySemaphore(device, renderDone, nullptr);
    vkDestroyFence(device, inFlight, nullptr);
    vkDestroyCommandPool(device, cmdPool, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeLayout, nullptr);
    for (auto fb : fbs)      vkDestroyFramebuffer(device, fb, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (auto iv : scViews)  vkDestroyImageView(device, iv, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
