/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Intel's Vulkan driver exposes an unregistered extension function,
// vkCreateDmaBufImageINTEL.  There is no extension associated with the
// function (as of 2016-09-21). The Intel developers added the function during
// Vulkan's early early days, in Mesa's first Vulkan commit on 2015-05-08, to
// provide a convenient way to test the driver. The Vulkan API had no extension
// mechanism yet in that early pre-1.0 timeframe.
//
// We use vkCreateDmaBufImageINTEL, despite its ambiguous status, because there
// does not yet exist a good alternative for importing a dma_buf as a VkImage.
//
// The Vulkan validation layer (VK_LAYER_LUNARG_standard_validation) does not
// understand vkCreateDmaBufImageINTEL. To run with the validation layer,
// #undef USE_vkCreateDmaBufImageINTEL and rebuild.
#undef USE_vkCreateDmaBufImageINTEL

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>

#ifdef USE_vkCreateDmaBufImageINTEL
#include <vulkan/vulkan_intel.h>
#endif

#include "bs_drm.h"

// Used for double-buffering.
struct frame {
	struct gbm_bo *bo;
	int bo_prime_fd;
	uint32_t drm_fb_id;
	VkDeviceMemory vk_memory;
	VkImage vk_image;
	VkImageView vk_image_view;
	VkFramebuffer vk_framebuffer;
	VkCommandBuffer vk_cmd_buf;
};

#define check_vk_success(result, vk_func) \
	__check_vk_success(__FILE__, __LINE__, __func__, (result), (vk_func))

static void __check_vk_success(const char *file, int line, const char *func, VkResult result,
			       const char *vk_func)
{
	if (result == VK_SUCCESS)
		return;

	bs_debug_print("ERROR", func, file, line, "%s failed with VkResult(%d)", vk_func, result);
	exit(EXIT_FAILURE);
}

static void page_flip_handler(int fd, unsigned int frame, unsigned int sec, unsigned int usec,
			      void *data)
{
	bool *waiting_for_flip = data;
	*waiting_for_flip = false;
}

// Choose the first physical device. Exit on failure.
VkPhysicalDevice choose_physical_device(VkInstance inst)
{
	uint32_t n_phys_devs;
	VkResult res;

	res = vkEnumeratePhysicalDevices(inst, &n_phys_devs, NULL);
	check_vk_success(res, "vkEnumeratePhysicalDevices");

	if (n_phys_devs == 0) {
		fprintf(stderr, "No available VkPhysicalDevices\n");
		exit(EXIT_FAILURE);
	}

	VkPhysicalDevice phys_devs[n_phys_devs];
	res = vkEnumeratePhysicalDevices(inst, &n_phys_devs, phys_devs);
	check_vk_success(res, "vkEnumeratePhysicalDevices");

	// Print information about all available devices. This helps debugging
	// when bringing up Vulkan on a new system.
	printf("Available VkPhysicalDevices:\n");

	for (uint32_t i = 0; i < n_phys_devs; ++i) {
		VkPhysicalDeviceProperties props;

		vkGetPhysicalDeviceProperties(phys_devs[i], &props);

		printf("    VkPhysicalDevice %u:\n", i);
		printf("	apiVersion: %u.%u.%u\n", VK_VERSION_MAJOR(props.apiVersion),
		       VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
		printf("	driverVersion: %u\n", props.driverVersion);
		printf("	vendorID: 0x%x\n", props.vendorID);
		printf("	deviceID: 0x%x\n", props.deviceID);
		printf("	deviceName: %s\n", props.deviceName);
		printf("	pipelineCacheUUID: %x%x%x%x-%x%x-%x%x-%x%x-%x%x%x%x%x%x\n",
		       props.pipelineCacheUUID[0], props.pipelineCacheUUID[1],
		       props.pipelineCacheUUID[2], props.pipelineCacheUUID[3],
		       props.pipelineCacheUUID[4], props.pipelineCacheUUID[5],
		       props.pipelineCacheUUID[6], props.pipelineCacheUUID[7],
		       props.pipelineCacheUUID[8], props.pipelineCacheUUID[9],
		       props.pipelineCacheUUID[10], props.pipelineCacheUUID[11],
		       props.pipelineCacheUUID[12], props.pipelineCacheUUID[13],
		       props.pipelineCacheUUID[14], props.pipelineCacheUUID[15]);
	}

	printf("Chose VkPhysicalDevice 0\n");
	fflush(stdout);

	return phys_devs[0];
}

// Return the index of a graphics-enabled queue family. Return UINT32_MAX on
// failure.
uint32_t choose_gfx_queue_family(VkPhysicalDevice phys_dev)
{
	uint32_t family_idx = UINT32_MAX;
	VkQueueFamilyProperties *props = NULL;
	uint32_t n_props = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &n_props, NULL);

	props = calloc(sizeof(props[0]), n_props);
	if (!props) {
		bs_debug_error("out of memory");
		exit(EXIT_FAILURE);
	}

	vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &n_props, props);

	// Choose the first graphics queue.
	for (uint32_t i = 0; i < n_props; ++i) {
		if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && props[i].queueCount > 0) {
			family_idx = i;
			break;
		}
	}

	free(props);
	return family_idx;
}

int main(int argc, char **argv)
{
	VkInstance inst;
	VkPhysicalDevice phys_dev;
	uint32_t gfx_queue_family_idx;
	VkDevice dev;
	VkQueue gfx_queue;
	VkRenderPass pass;
	VkCommandPool cmd_pool;
	VkResult res;
	struct frame frames[2];
	int err;

	int dev_fd = bs_drm_open_main_display();
	if (dev_fd < 0) {
		bs_debug_error("failed to open display device");
		exit(EXIT_FAILURE);
	}

	struct gbm_device *gbm = gbm_create_device(dev_fd);
	if (!gbm) {
		bs_debug_error("failed to create gbm_device");
		exit(EXIT_FAILURE);
	}

	struct bs_drm_pipe pipe = { 0 };
	if (!bs_drm_pipe_make(dev_fd, &pipe)) {
		bs_debug_error("failed to make drm pipe");
		exit(EXIT_FAILURE);
	}

	drmModeConnector *connector = drmModeGetConnector(dev_fd, pipe.connector_id);
	if (!connector) {
		bs_debug_error("drmModeGetConnector failed");
		exit(EXIT_FAILURE);
	}

	drmModeModeInfo *mode = &connector->modes[0];

	res = vkCreateInstance(
	    &(VkInstanceCreateInfo){
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo =
		    &(VkApplicationInfo){
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.apiVersion = VK_MAKE_VERSION(1, 0, 0),
		    },
	    },
	    /*pAllocator*/ NULL, &inst);
	check_vk_success(res, "vkCreateInstance");

	phys_dev = choose_physical_device(inst);

	gfx_queue_family_idx = choose_gfx_queue_family(phys_dev);
	if (gfx_queue_family_idx == UINT32_MAX) {
		bs_debug_error(
		    "VkPhysicalDevice exposes no VkQueueFamilyProperties "
		    "with graphics");
		exit(EXIT_FAILURE);
	}

	res = vkCreateDevice(phys_dev,
			     &(VkDeviceCreateInfo){
				 .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				 .queueCreateInfoCount = 1,
				 .pQueueCreateInfos =
				     (VkDeviceQueueCreateInfo[]){
					 {
					     .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					     .queueFamilyIndex = gfx_queue_family_idx,
					     .queueCount = 1,
					     .pQueuePriorities = (float[]){ 1.0f },

					 },
				     },
			     },
			     /*pAllocator*/ NULL, &dev);
	check_vk_success(res, "vkCreateDevice");

#if USE_vkCreateDmaBufImageINTEL
	PFN_vkCreateDmaBufImageINTEL bs_vkCreateDmaBufImageINTEL =
	    (void *)vkGetDeviceProcAddr(dev, "vkCreateDmaBufImageINTEL");
	if (bs_vkCreateDmaBufImageINTEL == NULL) {
		bs_debug_error("vkGetDeviceProcAddr(\"vkCreateDmaBufImageINTEL\') failed");
		exit(EXIT_FAILURE);
	}
#endif

	vkGetDeviceQueue(dev, gfx_queue_family_idx, /*queueIndex*/ 0, &gfx_queue);

	res = vkCreateCommandPool(dev,
				  &(VkCommandPoolCreateInfo){
				      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
					       VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				      .queueFamilyIndex = gfx_queue_family_idx,
				  },
				  /*pAllocator*/ NULL, &cmd_pool);
	check_vk_success(res, "vkCreateCommandPool");

	res = vkCreateRenderPass(
	    dev,
	    &(VkRenderPassCreateInfo){
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments =
		    (VkAttachmentDescription[]){
			{
			    .format = VK_FORMAT_A8B8G8R8_UNORM_PACK32,
			    .samples = 1,
			    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			    .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
			},
		    },
		.subpassCount = 1,
		.pSubpasses =
		    (VkSubpassDescription[]){
			{
			    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			    .colorAttachmentCount = 1,
			    .pColorAttachments =
				(VkAttachmentReference[]){
				    {
					.attachment = 0,
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				    },
				},
			},
		    },
	    },
	    /*pAllocator*/ NULL, &pass);
	check_vk_success(res, "vkCreateRenderPass");

	for (int i = 0; i < BS_ARRAY_LEN(frames); ++i) {
		struct frame *fr = &frames[i];

		fr->bo = gbm_bo_create(gbm, mode->hdisplay, mode->vdisplay, GBM_FORMAT_XBGR8888,
				       GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
		if (fr->bo == NULL) {
			bs_debug_error("failed to create framebuffer's gbm_bo");
			return 1;
		}

		fr->drm_fb_id = bs_drm_fb_create_gbm(fr->bo);
		if (fr->drm_fb_id == 0) {
			bs_debug_error("failed to create drm framebuffer id");
			return 1;
		}

		fr->bo_prime_fd = gbm_bo_get_fd(fr->bo);
		if (fr->bo_prime_fd < 0) {
			bs_debug_error("failed to get prime fd for gbm_bo");
			return 1;
		}

#if USE_vkCreateDmaBufImageINTEL
		res = bs_vkCreateDmaBufImageINTEL(
		    dev,
		    &(VkDmaBufImageCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DMA_BUF_IMAGE_CREATE_INFO_INTEL,
			.fd = fr->bo_prime_fd,
			.format = VK_FORMAT_A8B8G8R8_UNORM_PACK32,
			.extent = (VkExtent3D){ mode->hdisplay, mode->hdisplay, 1 },
			.strideInBytes = gbm_bo_get_stride(fr->bo),
		    },
		    /*pAllocator*/ NULL, &fr->vk_memory, &fr->vk_image);
		check_vk_success(res, "vkCreateDmaBufImageINTEL");
#else
		res = vkCreateImage(dev,
				    &(VkImageCreateInfo){
					.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					.imageType = VK_IMAGE_TYPE_2D,
					.format = VK_FORMAT_A8B8G8R8_UNORM_PACK32,
					.extent = (VkExtent3D){ mode->hdisplay, mode->hdisplay, 1 },
					.mipLevels = 1,
					.arrayLayers = 1,
					.samples = 1,
					.tiling = VK_IMAGE_TILING_LINEAR,
					.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
					.queueFamilyIndexCount = 1,
					.pQueueFamilyIndices = (uint32_t[]){ gfx_queue_family_idx },
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				    },
				    /*pAllocator*/ NULL, &fr->vk_image);
		check_vk_success(res, "vkCreateImage");

		VkMemoryRequirements mem_reqs;
		vkGetImageMemoryRequirements(dev, fr->vk_image, &mem_reqs);

		res = vkAllocateMemory(dev,
				       &(VkMemoryAllocateInfo){
					   .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					   .allocationSize = mem_reqs.size,

					   // Simply choose the first available memory type.
					   // We need neither performance nor mmap, so all
					   // memory types are equally good.
					   .memoryTypeIndex = ffs(mem_reqs.memoryTypeBits) - 1,
				       },
				       /*pAllocator*/ NULL, &fr->vk_memory);
		check_vk_success(res, "vkAllocateMemory");

		res = vkBindImageMemory(dev, fr->vk_image, fr->vk_memory,
					/*memoryOffset*/ 0);
		check_vk_success(res, "vkBindImageMemory");
#endif

		res = vkCreateImageView(dev,
					&(VkImageViewCreateInfo){
					    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					    .image = fr->vk_image,
					    .viewType = VK_IMAGE_VIEW_TYPE_2D,
					    .format = VK_FORMAT_A8B8G8R8_UNORM_PACK32,
					    .components =
						(VkComponentMapping){
						    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
						    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
						    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
						    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
						},
					    .subresourceRange =
						(VkImageSubresourceRange){
						    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						    .baseMipLevel = 0,
						    .levelCount = 1,
						    .baseArrayLayer = 0,
						    .layerCount = 1,
						},
					},
					/*pAllocator*/ NULL, &fr->vk_image_view);
		check_vk_success(res, "vkCreateImageView");

		res = vkCreateFramebuffer(dev,
					  &(VkFramebufferCreateInfo){
					      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
					      .renderPass = pass,
					      .attachmentCount = 1,
					      .pAttachments = (VkImageView[]){ fr->vk_image_view },
					      .width = mode->hdisplay,
					      .height = mode->vdisplay,
					      .layers = 1,
					  },
					  /*pAllocator*/ NULL, &fr->vk_framebuffer);
		check_vk_success(res, "vkCreateFramebuffer");

		res = vkAllocateCommandBuffers(
		    dev,
		    &(VkCommandBufferAllocateInfo){
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		    },
		    &fr->vk_cmd_buf);
		check_vk_success(res, "vkAllocateCommandBuffers");
	}

	// We set the screen mode using framebuffer 0. Then the first page flip
	// waits on framebuffer 1.
	err = drmModeSetCrtc(dev_fd, pipe.crtc_id, frames[0].drm_fb_id,
			     /*x*/ 0, /*y*/ 0, &pipe.connector_id, /*connector_count*/ 1, mode);
	if (err) {
		bs_debug_error("drmModeSetCrtc failed: %d", err);
		exit(EXIT_FAILURE);
	}

	// We set an upper bound on the render loop so we can run this in
	// from a testsuite.
	for (int i = 1; i < 500; ++i) {
		struct frame *fr = &frames[i % BS_ARRAY_LEN(frames)];

		// vkBeginCommandBuffer implicity resets the command buffer due
		// to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT.
		res = vkBeginCommandBuffer(fr->vk_cmd_buf,
					   &(VkCommandBufferBeginInfo){
					       .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					       .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
					   });
		check_vk_success(res, "vkBeginCommandBuffer");

		// Cycle along the circumference of the RGB color wheel.
		VkClearValue clear_color = {
			.color =
			    {
				.float32 =
				    {
					0.5f + 0.5f * sinf(2 * M_PI * i / 240.0f),
					0.5f + 0.5f * sinf(2 * M_PI * i / 240.0f +
							   (2.0f / 3.0f * M_PI)),
					0.5f + 0.5f * sinf(2 * M_PI * i / 240.0f +
							   (4.0f / 3.0f * M_PI)),
					1.0f,
				    },
			    },
		};

		vkCmdBeginRenderPass(fr->vk_cmd_buf,
				     &(VkRenderPassBeginInfo){
					 .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					 .renderPass = pass,
					 .framebuffer = fr->vk_framebuffer,
					 .renderArea =
					     (VkRect2D){
						 .offset = { 0, 0 },
						 .extent = { mode->hdisplay, mode->vdisplay },
					     },
					 .clearValueCount = 1,
					 .pClearValues = (VkClearValue[]){ clear_color },
				     },
				     VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(fr->vk_cmd_buf);

		res = vkEndCommandBuffer(fr->vk_cmd_buf);
		check_vk_success(res, "vkEndCommandBuffer");

		res =
		    vkQueueSubmit(gfx_queue,
				  /*submitCount*/ 1,
				  (VkSubmitInfo[]){
				      {
					  .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
					  .commandBufferCount = 1,
					  .pCommandBuffers = (VkCommandBuffer[]){ fr->vk_cmd_buf },
				      },
				  },
				  VK_NULL_HANDLE);
		check_vk_success(res, "vkQueueSubmit");

		res = vkQueueWaitIdle(gfx_queue);
		check_vk_success(res, "vkQueueWaitIdle");

		bool waiting_for_flip = true;
		err = drmModePageFlip(dev_fd, pipe.crtc_id, fr->drm_fb_id, DRM_MODE_PAGE_FLIP_EVENT,
				      &waiting_for_flip);
		if (err) {
			bs_debug_error("failed page flip: error=%d", err);
			exit(EXIT_FAILURE);
		}

		while (waiting_for_flip) {
			drmEventContext ev_ctx = {
				.version = DRM_EVENT_CONTEXT_VERSION,
				.page_flip_handler = page_flip_handler,
			};

			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(dev_fd, &fds);

			int n_fds = select(dev_fd + 1, &fds, NULL, NULL, NULL);
			if (n_fds < 0) {
				bs_debug_error("select() failed on page flip: %s", strerror(errno));
				exit(EXIT_FAILURE);
			} else if (n_fds == 0) {
				bs_debug_error("select() timeout on page flip");
				exit(EXIT_FAILURE);
			}

			err = drmHandleEvent(dev_fd, &ev_ctx);
			if (err) {
				bs_debug_error(
				    "drmHandleEvent failed while "
				    "waiting for page flip: error=%d",
				    err);
				exit(EXIT_FAILURE);
			}
		}
	}

	return 0;
}
