#include "SecondarySwapchain.h"


/*
struct SwapChainBuilder
{
	std::shared_ptr<Texture> depthTarget;
	std::shared_ptr<Texture> colorTarget;
	VkFormat depthFormat;
	VkFormat imageFormat;

	VkExtent2D windowExtent;

	VkRenderPass renderPass;

	std::vector<VkImageView> additionalImageView;
};
*/

SecondarySwapchain::SecondarySwapchain(Device& device, SwapChainBuilder builder) : device {device}
{
    assert((builder.colorTarget || builder.depthTarget || !builder.additionalImageView.empty()) && "must provide at least 1 target to create swapchain");

	if (builder.depthTarget) {
		hasDepth = true;
		depthTarget = builder.depthTarget; 
        depthFormat = builder.depthFormat;
	}

	if (builder.colorTarget) {
		hasColor = true;
		colorTarget = builder.colorTarget;
	}

    additionalImageView = builder.additionalImageView;  
    // caller might pass created per-face 2D views; we won't assume ownership
    // if caller passed empty -> we won't set ownsAdditionalViews
    ownsAdditionalViews = false;

    if (builder.renderPass) {
        // Use provided render pass (caller is responsible for compatibility)
        renderPass = builder.renderPass;
        ownsRenderPass = false;
    }
    else {
        createRenderPass();
        ownsRenderPass = true;
    }

    // If additionalImageView provided (per-face color views), we need per-face framebuffers.
    if (!additionalImageView.empty()) {
        size_t faces = additionalImageView.size();
        framebuffers.resize(faces);

        // If we also have depthTarget, create a per-face depth view for each face (layer)
        if (hasDepth) {
            perFaceDepthViews.resize(faces);
            for (size_t i = 0; i < faces; ++i) {
                // create depth view for layer i
                VkImageViewCreateInfo depthViewCI{};
                depthViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                depthViewCI.image = depthTarget->getImage();
                depthViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D; // per-face 2D view
                depthViewCI.format = depthFormat;
                depthViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                depthViewCI.subresourceRange.baseMipLevel = 0;
                depthViewCI.subresourceRange.levelCount = 1;
                depthViewCI.subresourceRange.baseArrayLayer = static_cast<uint32_t>(i);
                depthViewCI.subresourceRange.layerCount = 1;

                if (vkCreateImageView(device.device(), &depthViewCI, nullptr, &perFaceDepthViews[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create per-face depth image view");
                }
            }
        }

        for (int i = 0; i < faces; i++)
            createFramebuffer(builder.windowExtent, framebuffers[i], i);
    }
    else
    {
        framebuffers.resize(1);
        createFramebuffer(builder.windowExtent, framebuffers[0]);
    }
    
    
}

SecondarySwapchain::~SecondarySwapchain()
{
    for (auto fb : framebuffers) {
        if (fb != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device.device(), fb, nullptr);
        }
    }
    framebuffers.clear();

    // Destroy per-face depth views we created (if any)
    for (auto dv : perFaceDepthViews) {
        if (dv != VK_NULL_HANDLE) {
            vkDestroyImageView(device.device(), dv, nullptr);
        }
    }
    perFaceDepthViews.clear();

    if (ownsRenderPass && renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device.device(), renderPass, nullptr);
    }
}


void SecondarySwapchain::createRenderPass() {
    VkAttachmentReference depthAttachmentRef{};
    VkAttachmentReference colorRef{};

    std::vector<VkAttachmentDescription> attachments{};

    if (hasColor || !additionalImageView.empty()) {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = VK_FORMAT_R8G8B8A8_SRGB;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // important        

        attachments.push_back(colorAttachment);
    }


    if (hasDepth) {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
        attachments.push_back(depthAttachment);
    }

    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    if (hasColor || !additionalImageView.empty()) {

        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        sub.colorAttachmentCount = 1;
        sub.pColorAttachments = &colorRef;
    }

    if (hasDepth) {
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        sub.pDepthStencilAttachment = &depthAttachmentRef;
    }

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &sub;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pre-render depth pass!");
    }
}

void SecondarySwapchain::createFramebuffer(VkExtent2D extent, VkFramebuffer& framebuffer, int indexFB) {
    std::vector<VkImageView> attachments{};

    // IMPORTANT: push attachments in the same order as the render pass expects:
    // color first (if present), then depth (if present).
    if (indexFB >= 0) {
        // per-face color view was supplied in additionalImageView
        attachments.push_back(additionalImageView[indexFB]);

        if (hasDepth) {
            // use the per-face depth view we created earlier
            attachments.push_back(perFaceDepthViews[indexFB]);
        }
    }
    else 
    {
        if (hasColor) attachments.push_back(colorTarget->getImageView());
        if (hasDepth) attachments.push_back(depthTarget->getImageView());
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer for secondary swapchain!");
    }
}


void SecondarySwapchain::submitCommandBuffer(const VkCommandBuffer* commandBuffer)
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = commandBuffer;

    device.submitToGraphicQueue(submitInfo, nullptr);
}

VkFormat SecondarySwapchain::findDepthFormat() {
    return device.findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

