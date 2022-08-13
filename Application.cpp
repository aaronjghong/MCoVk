#include "Application.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/* LOCAL VARS */
static Camera player( glm::vec3( 2.0f, 2.0f, 2.0f ) );
static float deltaTime;
static float lastX = 400;
static float lastY = 400;
static bool mouseDisabled = false;
static bool firstMouse = false;
static float fps = 0.0f;
static VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
static Application* currApp;

/* LOCAL FUNCTIONS */
static void frameBufferResizeCallback( GLFWwindow* window, int width, int height ) {
	auto app = ( Application* )glfwGetWindowUserPointer( window );
	app->frameBufferResized = true;
}

void printCoordinates() {
	std::cout << "PLAYER POSITION: " << std::endl;
	std::cout << "X: " << player.Position.x * 16 << std::endl;
	std::cout << "Y: " << player.Position.y * 16 << std::endl;
	std::cout << "Z: " << player.Position.z * 16 << std::endl;
}

void processInputs( GLFWwindow* window ) {
	if( glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ) {
		player.processKeyboard(C_FORWARD, deltaTime);
		printCoordinates();
	}
	if( glfwGetKey( window, GLFW_KEY_S ) == GLFW_PRESS ) {
		player.processKeyboard( C_BACKWARD, deltaTime );
		printCoordinates();
	}
	if( glfwGetKey( window, GLFW_KEY_A ) == GLFW_PRESS ) {
		player.processKeyboard( C_LEFT, deltaTime );
		printCoordinates();
	}
	if( glfwGetKey( window, GLFW_KEY_D ) == GLFW_PRESS ) {
		player.processKeyboard( C_RIGHT, deltaTime );
		printCoordinates();
	}

	if( glfwGetKey( window, GLFW_KEY_LEFT_ALT ) == GLFW_PRESS ) {
		glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
		mouseDisabled = true;
		firstMouse = true;
	}

	if( glfwGetKey( window, GLFW_KEY_LEFT_ALT ) == GLFW_RELEASE ) {
		glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
		mouseDisabled = false;
	}

	if( glfwGetKey( window, GLFW_KEY_V ) == GLFW_PRESS ) {
		if( polygonMode != VK_POLYGON_MODE_LINE ) {
			polygonMode = VK_POLYGON_MODE_LINE;
			currApp->refreshGraphics();
		}
	}

	if( glfwGetKey( window, GLFW_KEY_V ) == GLFW_RELEASE ) {
		if( polygonMode != VK_POLYGON_MODE_FILL ) {
			polygonMode = VK_POLYGON_MODE_FILL;
			currApp->refreshGraphics();
		}
	}

	if( glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS ) {
		glfwSetWindowShouldClose( window, GLFW_TRUE );
	}
}

static void mouseMovementCallback( GLFWwindow* window, double xPos, double yPos ) {
	if( mouseDisabled ) {
		return;
	}

	if( firstMouse ) {
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}
	
	float xOffset = xPos - lastX;
	float yOffset = lastY - yPos;
	lastX = xPos;
	lastY = yPos;

	player.processMouseMovement( xOffset, yOffset );
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if ( action != GLFW_PRESS ) {
		return;
	}
	
	glm::vec3 lookRay = player.Front;
	lookRay /= 16; // divide by 16(chunk/block space), mul by 8(block reach) // Handled by world?

	glm::vec3 pos = player.Position;

	int i = 0;
	bool collided = false;
	while ( i != 8 && !collided ) {
		pos += lookRay;
		
		if ( button == GLFW_MOUSE_BUTTON_1 ) {
			if ( currApp->world->processBlockBreak(pos.x, pos.y, pos.z) ) {
				collided = true;
			}
		}
		else if ( button == GLFW_MOUSE_BUTTON_2 ) {
			/* Change to place block */
			if ( currApp->world->processBlockPlace(pos.x, pos.y, pos.z, lookRay.x, lookRay.y, lookRay.z, 1) ) {
				collided = true;
			}
		}

		i++;
	}
}

/* APPLICATION FUNCTION DEFINITIONS */
void Application::refreshGraphics() {
	recreateSwapChain();
}

void Application::run() {
	currApp = this;
	initWindow();
	initVulkan();
	//initGui();
	mainLoop();
	cleanup();
}

void Application::initWindow() {
	if( !glfwInit() ) {
		throw std::runtime_error( "Failed to Initialize GLFW" );
	}
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );

	window = glfwCreateWindow( WIDTH, HEIGHT, "VK", nullptr, nullptr );
	glfwSetWindowUserPointer( window, this );
	glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );

	/* Set-up GLFW callbacks */
	glfwSetFramebufferSizeCallback( window, frameBufferResizeCallback );
	glfwSetCursorPosCallback( window, mouseMovementCallback );
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	//glfwSetKeyCallback( window, keyInputCallback );

	if( !window ) {
		throw std::runtime_error( "Failed to Create GLFW Window" );
	}
}

void Application::initVulkan() {
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createDepthResources();
	createFrameBuffers();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
	
	initWorld();


	/*createVertexBuffer();
	createIndexBuffer();*/
	
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createSyncObjects();

}

void Application::mainLoop() {
	while( !glfwWindowShouldClose( window ) ) {
		glfwPollEvents();
		processInputs( window );
		drawFrame();
	}

	vkDeviceWaitIdle( device );
}

void Application::drawFrame() {
	// WAIT for fence to be signaled as available for the current frame
	vkWaitForFences( device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX );

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR( device, swapChain, UINT32_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex );

	// IF the swapchain is incompatible with the current surface
	if( result == VK_ERROR_OUT_OF_DATE_KHR ) {
		// RECREATE the swapchain
		recreateSwapChain();
		return;
	}
	// ELSE IF the aquire image operation failed AND the swapchain cannot be used
	else if( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR ) {
		// THROW error
		throw std::runtime_error( "Failed to acquire swapchain image" );
	}

	// IF a previous frame is using the current image
	if( imagesInFlight[imageIndex] != VK_NULL_HANDLE ) {
		// WAIT for the image to be available
		vkWaitForFences( device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX );
	}
	// MARK the current image as being in-use by this frame
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];



	// UNSIGNAL the current fence as available 
	vkResetFences( device, 1, &inFlightFences[currentFrame] );

	vkResetCommandBuffer( commandBuffers[currentFrame], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );
	recordCommandBuffer( commandBuffers[currentFrame], imageIndex );

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;

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


	// SUBMIT cmd information to graphics queue
	if( vkQueueSubmit( graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame] ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to submit draw command buffer" );
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	// PRESENT image to the swapchain
	result = vkQueuePresentKHR( presentQueue, &presentInfo );

	// IF the swapchain is incompatible with the current surface or suboptimal for it
	if( (result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR) || frameBufferResized ) {
		recreateSwapChain();
		frameBufferResized = false;
	}
	// ELSE IF presenting did not work
	else if( result != VK_SUCCESS ) {
		// THROW error
		throw std::runtime_error( "Failed to present swap chain image" );
	}

	/*appGui->guiBeginFrame();
	appGui->guiRenderString( "TEST" );
	appGui->guiEndFrame();*/

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::cleanup() {
	// WAIT for device to be idle
	vkDeviceWaitIdle( device );

	// CLEAN vk objs
	cleanupSwapChain();

	/*ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();*/

	/* Could have texture cleanup in separate func if we switch to array/vector */
	vkDestroySampler( device, textureSampler, nullptr );
	vkDestroyImageView( device, textureImageView, nullptr );
	vkDestroyImage( device, textureImage, nullptr );
	vkFreeMemory( device, textureImageMemory, nullptr );

	vkDestroyDescriptorPool( device, descriptorPool, nullptr );

	vkDestroyDescriptorSetLayout( device, descriptorSetLayout, nullptr );

	vkDestroyBuffer( device, vertexBuffer, nullptr );
	vkFreeMemory( device, vertexBufferMemory, nullptr );

	vkDestroyBuffer( device, indexBuffer, nullptr );
	vkFreeMemory( device, indexBufferMemory, nullptr );

	vkDestroyBuffer(device, worldVertexBuffer, nullptr);
	vkFreeMemory(device, worldVertexBufferMemory, nullptr);

	vkDestroyBuffer(device, worldIndexBuffer, nullptr);
	vkFreeMemory(device, worldIndexBufferMemory, nullptr);

	delete world;

	for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {
		vkDestroySemaphore( device, imageAvailableSemaphores[i], nullptr );
		vkDestroySemaphore( device, renderFinishedSemaphores[i], nullptr );
		vkDestroyFence( device, inFlightFences[i], nullptr );
	}

	vkDestroyCommandPool(device, commandPool, nullptr);

	vkDestroyDevice( device, nullptr );

	vkDestroySurfaceKHR( instance, surface, nullptr );

	vkDestroyInstance( instance, nullptr );

	// CLEAN glfw 
	glfwDestroyWindow( window );
	glfwTerminate();

}

void Application::createInstance() {

	if( enableValidationLayers && !checkValidationLayerSupport() ) {
		throw std::runtime_error( "Validation layers requested but not available" );
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "Triangle Test";
	appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.apiVersion = VK_API_VERSION_1_0;
	
	// GET VK instance extensions required by glfw
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	
	glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

	// GET current supported instance extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );
	std::vector<VkExtensionProperties> extensions( extensionCount );
	vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, extensions.data() );

	std::cout << "Available Instance Extensions:\n";
	for( const auto& extension : extensions ) {
		std::cout << '\t' << extension.extensionName << std::endl;
	}
	// CHECK if required extensions list matches exist within supported extensions
	if( !checkExtensionMatch( glfwExtensionCount, glfwExtensions, extensionCount, extensions ) ) {
		throw std::runtime_error( "Error required GLFW extensions not supported" );
	}

	// CREATE VK instance
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;

	if( enableValidationLayers ) {
		createInfo.enabledLayerCount = ( uint32_t )validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	if( vkCreateInstance( &createInfo, nullptr, &instance ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to Create Vulkan Instance" );
	}
}

void Application::setupDebugMessenger() {
	// TODO
}

void Application::pickPhysicalDevice() {

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices( instance, &deviceCount, nullptr );

	if( deviceCount == 0 ) {
		throw std::runtime_error( "Failed to find any physical devices with VK support" );
	}

	std::vector<VkPhysicalDevice> devices( deviceCount );
	vkEnumeratePhysicalDevices( instance, &deviceCount, devices.data() );

	std::multimap<int, VkPhysicalDevice> candidates;

	// FOR each device, if suitable, give the device a score and pick the device with the highest score
	for( const auto& device : devices ) {
		
		if( isDeviceSuitable( device ) ) {
			int score = rateDeviceSuitability( device );
			candidates.insert( std::make_pair( score, device ) );
		}

		if( candidates.rbegin()->first > 0 ) {
			physicalDevice = candidates.rbegin()->second;
		}
		else {
			throw std::runtime_error( "Failed to find suitable physical device" );
		}
	}
	if( physicalDevice == VK_NULL_HANDLE ) {
		throw std::runtime_error( "Failed to find suitable physical device" );
	}
}

void Application::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies( physicalDevice );

	float queuePriority = 1.0f;	// For now, just set it as the highest priority

	// Create set of unique queue families
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsFamily.value(), indices.presentFamily.value()
	};

	for( uint32_t queueFamily : uniqueQueueFamilies ) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = nullptr;
		queueCreateInfo.flags = 0;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back( queueCreateInfo );
	}


	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.queueCreateInfoCount = ( uint32_t )queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	if( enableValidationLayers ) {
		createInfo.enabledLayerCount = ( uint32_t )validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}
	createInfo.enabledExtensionCount = ( uint32_t )deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	if( vkCreateDevice( physicalDevice, &createInfo, nullptr, &device ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create logical device" );
	}

	vkGetDeviceQueue( device, indices.graphicsFamily.value(), 0, &graphicsQueue );
	vkGetDeviceQueue( device, indices.presentFamily.value(), 0, &presentQueue );
}

void Application::createImage( uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory ) {
	VkImageCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = 1; // 2D Image, 1 texel 
	createInfo.mipLevels = 1; // Ignoring mipmaps for now
	createInfo.arrayLayers = 1; // Texture is not an array
	createInfo.format = format; // Keep format same as pixel format when possible
	createInfo.tiling = tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Undefined -> discard data on first operation
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Image will only be used by one queue
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;	// No multisampling
	createInfo.flags = 0;

	if( vkCreateImage( device, &createInfo, nullptr, &image ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create texture image!" );
	}

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements( device, image, &memReqs );

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memReqs.size;
	allocateInfo.memoryTypeIndex = findMemoryType( memReqs.memoryTypeBits, properties );

	if( vkAllocateMemory( device, &allocateInfo, nullptr, &imageMemory ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to allocate texture image memory!" );
	}

	vkBindImageMemory( device, image, imageMemory, 0 );
}

void Application::createTextureImage() {
	int width, height, channels;
	
	/* We load in 4 channels (R,G,B,A) */
	stbi_uc* imagePixels = stbi_load( "textures/texture.jpg", &width, &height, &channels, STBI_rgb_alpha );

	/* Each image is laid out row by row with 4 bytes per pixel ( for each channel ) */
	/* Specify just enough memory to load in the image */
	VkDeviceSize imageSize = width * height * 4;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	/* Load image pixel data into staging buffer */
	createBuffer( imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory );
	void* data;
	vkMapMemory( device, stagingBufferMemory, 0, imageSize, 0, &data );
	memcpy( data, imagePixels, ( size_t ) imageSize );
	vkUnmapMemory( device, stagingBufferMemory );

	/* Image no longer needed, can be freed */
	stbi_image_free( imagePixels );

	imageFormat = findTextureFormat();

	createImage( width, height, imageFormat /*VK_FORMAT_R8G8B8A8_SRGB*/, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory );
	
	/* Set image layout to optimal transfer dst layout */
	transitionImageLayout( textureImage, imageFormat/*VK_FORMAT_R8G8B8A8_SRGB*/, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

	/* Copy staging buffer contents to texture image */
	copyBufferToImage( stagingBuffer, textureImage, width, height );

	/* Set image back to usable layout */
	transitionImageLayout( textureImage, imageFormat/*VK_FORMAT_R8G8B8A8_SRGB*/, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	vkDestroyBuffer( device, stagingBuffer, nullptr );
	vkFreeMemory( device, stagingBufferMemory, nullptr );
}

void Application::createTextureImageView() {
	textureImageView = createImageView( textureImage, imageFormat/*VK_FORMAT_R8G8B8A8_SRGB*/, VK_IMAGE_ASPECT_COLOR_BIT );
}

void Application::createTextureSampler() {
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties( physicalDevice, &properties );

	VkSamplerCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	/* Bilinear sampling for now, maybe switch to nearest for voxels */
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;

	/* Tile texture if sampling out of space */
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

	createInfo.anisotropyEnable = VK_TRUE;
	createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;

	if( vkCreateSampler( device, &createInfo, nullptr, &textureSampler ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create sampler!" );
	}
}

void Application::createSurface() {
	if( glfwCreateWindowSurface( instance, window, nullptr, &surface ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create window surface" );
	}
}

void Application::createSwapChain() {
	SwapChainSupportDetails details = querySwapChainDetails( physicalDevice );
	QueueFamilyIndices indices = findQueueFamilies( physicalDevice );
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( details.formats );
	VkPresentModeKHR presentMode = chooseSwapPresentMode( details.presentModes );
	VkExtent2D extent = chooseSwapExtent( details.capabilities );

	uint32_t imageCount = details.capabilities.minImageCount + 1;

	// IF desired imagecount is greater than the max supported images
	if( (details.capabilities.maxImageCount > 0) && (details.capabilities.maxImageCount < imageCount) ) {
		// SET image count to max supported images
		imageCount = details.capabilities.maxImageCount;
	}

	// CREATE swapchain
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// IF the graphics queue is NOT the same as the presentation queue
	if( indices.graphicsFamily != indices.presentFamily ) {
		// SET image to be accessed concurrently
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	// ELSE (graphics queue == presentation queue)
	else {
		// SET images to be exclusive per-queue
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = details.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if( vkCreateSwapchainKHR( device, &createInfo, nullptr, &swapChain ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create swapchain" );
	}

	// GET handles to images in the swapchain
	vkGetSwapchainImagesKHR( device, swapChain, &imageCount, nullptr );
	swapChainImages.resize( imageCount );
	vkGetSwapchainImagesKHR( device, swapChain, &imageCount, swapChainImages.data() );

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Application::createImageViews() {
	swapChainImageViews.resize( swapChainImages.size() );

	for( size_t i = 0; i < swapChainImages.size(); i++ ) {
		swapChainImageViews[i] = createImageView( swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT );
	}
}

void Application::createRenderPass() {
	// SET renderpass attachments
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// SET renderpass subpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	// SET subpass dependencies
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depthDependency{};
	depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depthDependency.dstSubpass = 0;
	depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.srcAccessMask = 0;
	depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	std::array<VkSubpassDependency, 2> dependencies = { dependency, depthDependency };

	// CREATE renderpass
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.flags = 0;
	renderPassInfo.attachmentCount = ( uint32_t )attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = ( uint32_t )dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	if( vkCreateRenderPass( device, &renderPassInfo, nullptr, &renderPass ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create render pass" );
	}


}

void Application::createGraphicsPipeline() {
	// GET shaders and create shader modules
	auto vertShaderCode = u_readFile( "shaders/vert.spv" );
	auto fragShaderCode = u_readFile( "shaders/frag.spv" );
	VkShaderModule vertShaderModule = createShaderModule( vertShaderCode );
	VkShaderModule fragShaderModule = createShaderModule( fragShaderCode );

	// SET shader stages for each shader module
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.pNext = nullptr;
	vertShaderStageInfo.flags = 0;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.pNext = nullptr;
	fragShaderStageInfo.flags = 0;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// SET pipeline fixed function infos
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pNext = nullptr;
	vertexInputInfo.flags = 0;

	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	auto bindingDescription = Vertex::getBindingDescription();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.pNext = nullptr;
	inputAssembly.flags = 0;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewPort{};
	viewPort.x = 0.0f;
	viewPort.y = 0.0f;
	viewPort.width = ( float )swapChainExtent.width;
	viewPort.height = ( float )swapChainExtent.height;
	viewPort.minDepth = 0.0f;
	viewPort.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewPortState{};
	viewPortState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewPortState.pNext = nullptr;
	viewPortState.flags = 0;
	viewPortState.viewportCount = 1;
	viewPortState.pViewports = &viewPort;
	viewPortState.scissorCount = 1;
	viewPortState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.pNext = nullptr;
	rasterizer.flags = 0;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = polygonMode;	// -> change to VK_POLYGON_MODE_LINE to get mesh rendering, must create another pipeline though and some special gpu feature (google it)
	rasterizer.lineWidth = 1.0f;					// Anything larger than 1.0f requires wideLines
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;//VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Flip from CW -> CCW because we flipped y-axis in updateuniform to match glm gl to vk
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.pNext = nullptr;
	multisampling.flags = 0;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;	// Lower depth -> closer obj
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;
	colorBlending.flags = 0;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = nullptr;
	dynamicState.flags = 0;
	dynamicState.dynamicStateCount = 3;
	dynamicState.pDynamicStates = dynamicStates;

	// Setup Push Constants
	VkPushConstantRange vertexPushConstant{};
	vertexPushConstant.offset = 0;
	vertexPushConstant.size = sizeof( VertexPushConstant );
	vertexPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// CREATE pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pNext = nullptr;
	pipelineLayoutInfo.flags = 0;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &vertexPushConstant;

	if( vkCreatePipelineLayout( device, &pipelineLayoutInfo, nullptr, &pipelineLayout ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create pipeline layout" );
	}

	// CREATE graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewPortState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if( vkCreateGraphicsPipelines( device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create graphics pipeline" );
	}

	// DESTROY (now) unneeded shader modules
	vkDestroyShaderModule( device, vertShaderModule, nullptr );
	vkDestroyShaderModule( device, fragShaderModule, nullptr );
}

void Application::createFrameBuffers() {
	swapChainFrameBuffers.resize( swapChainImageViews.size() );

	for( size_t i = 0; i < swapChainImageViews.size(); i++ ) {
		std::array<VkImageView, 2> attachments = {
			swapChainImageViews[i],
			depthImageView
		};

		VkFramebufferCreateInfo frameBufferInfo{};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = renderPass;
		frameBufferInfo.attachmentCount = ( uint32_t )attachments.size();
		frameBufferInfo.pAttachments = attachments.data();
		frameBufferInfo.width = swapChainExtent.width;
		frameBufferInfo.height = swapChainExtent.height;
		frameBufferInfo.layers = 1;

		if( vkCreateFramebuffer( device, &frameBufferInfo, nullptr, &swapChainFrameBuffers[i] ) != VK_SUCCESS ) {
			throw std::runtime_error( "Failed to create framebuffer" );
		}
	}
}

void Application::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies( physicalDevice );

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;	//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT might want this for continuous updates
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if( vkCreateCommandPool( device, &poolInfo, nullptr, &commandPool ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create command pool" );
	}


}

void Application::createCommandBuffers() {
	commandBuffers.resize( swapChainImageViews.size() );

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = ( uint32_t )commandBuffers.size();

	if( vkAllocateCommandBuffers( device, &allocInfo, commandBuffers.data() ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to allocate command buffers" );
	}
}

void Application::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	// BEGIN recording on the commandbuffer
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if( vkBeginCommandBuffer( commandBuffer, &beginInfo ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to begin recording command buffers!" );
	}

	// RECORD cmd to start the renderpass on the current commandbuffer
	VkRenderPassBeginInfo  renderPassInfo{};

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.53f, 0.81f, 0.92f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	//VkClearValue clearColor = { {{0.53f, 0.81f, 0.92f, 1.0f}} };
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFrameBuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChainExtent;
	renderPassInfo.clearValueCount = ( uint32_t )clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

	// RECORD cmd to bind the graphics pipeline to the current commandbuffer
	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline );

	//VkBuffer vertexBuffers[] = { vertexBuffer };

	if ( world->worldUpdated ) {
		updateWorldBuffers();
		world->worldUpdated = false;
	}

	VkBuffer vertexBuffers[] = { worldVertexBuffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers( commandBuffer, 0, 1, vertexBuffers, offsets );
	//vkCmdBindIndexBuffer( commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32 );
	vkCmdBindIndexBuffer(commandBuffer, worldIndexBuffer, 0, VK_INDEX_TYPE_UINT32);


	// Update uniform data
	// At the moment not used in shader code, only used to update ubo obj
	// Keeping for now since ubo will likely be needed later on
	updateUniformBuffer( currentFrame );


	// Bind descriptor sets (update data being sent to shaders)
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr );

	/*  
	* What should be happening here is that we render the chunk with a world obj
	* Instead of binding and rendering multiple cubes, once the mesh is generated
	* A singular mesh should be bound and drawn
	* 
	*/
	
	glm::vec3 cubePositions[] = {
		glm::vec3( 0.0f,  0.0f,  0.0f ),
		glm::vec3( 2.0f,  5.0f, -15.0f ),
		glm::vec3( -1.5f, -2.2f, -2.5f ),
		glm::vec3( -3.8f, -2.0f, -12.3f ),
		glm::vec3( 2.4f, -0.4f, -3.5f ),
		glm::vec3( -1.7f,  3.0f, -7.5f ),
		glm::vec3( 1.3f, -2.0f, -2.5f ),
		glm::vec3( 1.5f,  2.0f, -2.5f ),
		glm::vec3( 1.5f,  0.2f, -1.5f ),
		glm::vec3( -1.3f,  1.0f, -1.5f )
	};


	VertexPushConstant constants;

	//for( int i = 0; i < 10; i++ )
	{
		/* Update and send push constants for each cube */
		constants.renderMatrix = ubo.proj * ubo.view * glm::mat4(1.0f);//glm::translate(glm::mat4(1.0f), cubePositions[i]);//glm::rotate( glm::translate( glm::mat4( 1.0f ), cubePositions[i] ), deltaTime * glm::radians( 90.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
		vkCmdPushConstants( commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( VertexPushConstant ), &constants );

		// RECORD cmd to draw to the current commandbuffer
		//vkCmdDraw( commandBuffer, ( uint32_t )world->getVertexCount(), 1, 0, 0);
		vkCmdDrawIndexed( commandBuffer, ( uint32_t )world->getIndexCount(), 1, 0, 0, 0);

	}
	
	// RECORD cmd to end the renderpass on the current commandbuffer
	vkCmdEndRenderPass( commandBuffer );

	// END recording on the current commandbuffer
	if( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to record command buffer" );
	}
}

void Application::createSyncObjects() {
	imageAvailableSemaphores.resize( ( size_t )MAX_FRAMES_IN_FLIGHT );
	renderFinishedSemaphores.resize( ( size_t )MAX_FRAMES_IN_FLIGHT );
	inFlightFences.resize( ( size_t )MAX_FRAMES_IN_FLIGHT );
	imagesInFlight.resize( swapChainImages.size(), VK_NULL_HANDLE );

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


	for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {

		if( (vkCreateSemaphore( device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i] ) != VK_SUCCESS) ||
			(vkCreateSemaphore( device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i] ) != VK_SUCCESS) || 
			(vkCreateFence( device, &fenceInfo, nullptr, &inFlightFences[i] ) != VK_SUCCESS ) ){
			throw std::runtime_error( "Failed to create sync objects for a frame" );
		}
	}
	
}

void Application::cleanupSwapChain() {
	for( auto frameBuffer : swapChainFrameBuffers ) {
		vkDestroyFramebuffer( device, frameBuffer, nullptr );
	}

	vkFreeCommandBuffers( device, commandPool, ( uint32_t )commandBuffers.size(), commandBuffers.data() );

	vkDestroyPipeline( device, graphicsPipeline, nullptr );
	vkDestroyPipelineLayout( device, pipelineLayout, nullptr );
	vkDestroyRenderPass( device, renderPass, nullptr );

	for( auto imageView : swapChainImageViews ) {
		vkDestroyImageView( device, imageView, nullptr );
	}

	vkDestroySwapchainKHR( device, swapChain, nullptr );

	for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {
		vkDestroyBuffer( device, uniformBuffers[i], nullptr );
		vkFreeMemory( device, uniformBuffersMemory[i], nullptr );
	}
}

void Application::recreateSwapChain() {
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize( window, &width, &height );

	// WHILE the window size is 0
	while( width == 0 || height == 0 ) {
		// WAIT for window to be unminimized
		glfwGetFramebufferSize( window, &width, &height );
		glfwWaitEvents();
	}

	vkDeviceWaitIdle( device );

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFrameBuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
}

void Application::createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof( vertices[0] ) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory );

	void* data;
	vkMapMemory( device, stagingBufferMemory, 0, bufferSize, 0, &data );
	memcpy( data, vertices.data(), ( uint32_t )bufferSize );
	vkUnmapMemory( device, stagingBufferMemory );

	createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory );

	copyBuffer( stagingBuffer, vertexBuffer, bufferSize );

	vkDestroyBuffer( device, stagingBuffer, nullptr );
	vkFreeMemory( device, stagingBufferMemory, nullptr );
}

void Application::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult status = vkCreateBuffer( device, &bufferInfo, nullptr, &buffer );

	if( status != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create buffer!" );
	}

	VkMemoryRequirements memRequirements{};
	vkGetBufferMemoryRequirements( device, buffer, &memRequirements );

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, properties );

	if( vkAllocateMemory( device, &allocInfo, nullptr, &bufferMemory ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to allocate memory for buffer" );
	}

	vkBindBufferMemory( device, buffer, bufferMemory, 0 );
}

void Application::createIndexBuffer() {
	VkDeviceSize bufferSize = sizeof( indices[0] ) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory );

	void* data;
	vkMapMemory( device, stagingBufferMemory, 0, bufferSize, 0, &data );
	memcpy( data, indices.data(), ( uint32_t )bufferSize );
	vkUnmapMemory( device, stagingBufferMemory );

	createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory );

	copyBuffer( stagingBuffer, indexBuffer, bufferSize );
	vkDestroyBuffer( device, stagingBuffer, nullptr );
	vkFreeMemory( device, stagingBufferMemory, nullptr );
}

void Application::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = ( uint32_t )(bindings.size());
	createInfo.pBindings = bindings.data();

	if( vkCreateDescriptorSetLayout( device, &createInfo, nullptr, &descriptorSetLayout ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create descriptor set layout" );
	}
}

void Application::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof( UniformBufferObject );

	uniformBuffers.resize( (size_t)MAX_FRAMES_IN_FLIGHT );
	uniformBuffersMemory.resize( (size_t)MAX_FRAMES_IN_FLIGHT );

	for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {
		createBuffer( bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i] );
	}
}

void Application::updateUniformBuffer( uint32_t currentImage ) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

	fps = 1.0f / (time - deltaTime);
	// std::cout << "FPS: " << fps << std::endl;

	deltaTime = time; // Set for camera update operations

	ubo.model = glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 90.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
	ubo.view = player.getViewMatrix();
	ubo.proj = glm::perspective( glm::radians( fovDegrees ), ( float )swapChainExtent.width / swapChainExtent.height, 0.01f, 100.0f );

	ubo.proj[1][1] *= -1; // OpenGL vs Vulkan y-axis is flipped, must flip y-axis scaling factor

	void* data;
	vkMapMemory( device, uniformBuffersMemory[currentImage], 0, sizeof( ubo ), 0, &data );
	memcpy( data, &ubo, sizeof( ubo ) );
	vkUnmapMemory( device, uniformBuffersMemory[currentImage] );

}

void Application::updateMVP(uint32_t currentImage) {
	/* Maps and copies MVP ubo to device */
	void* data;
	vkMapMemory( device, uniformBuffersMemory[currentImage], 0, sizeof( ubo ), 0, &data );
	memcpy( data, &ubo, sizeof( ubo ) );
	vkUnmapMemory( device, uniformBuffersMemory[currentImage] );

}

void Application::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].descriptorCount = ( uint32_t )(MAX_FRAMES_IN_FLIGHT);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	
	poolSizes[1].descriptorCount = ( uint32_t )(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = ( uint32_t )(poolSizes.size());
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT;

	if( vkCreateDescriptorPool( device, &createInfo, nullptr, &descriptorPool ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create descriptor pool" );
	}
}

void Application::createDescriptorSets() {
	// Creating a descriptor set for each layout
	std::vector<VkDescriptorSetLayout> layouts( MAX_FRAMES_IN_FLIGHT, descriptorSetLayout );

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = ( uint32_t )MAX_FRAMES_IN_FLIGHT;
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize( MAX_FRAMES_IN_FLIGHT );
	if( vkAllocateDescriptorSets( device, &allocInfo, descriptorSets.data() ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to allocate descriptor sets" );
	}

	// Populate descriptor
	for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {
		// Descriptor is UBO thus, we use vkdescriptorbufferinfo 
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof( UniformBufferObject );

		/* Will need to move this elsewhere when doing multiple textures */
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets( device, ( uint32_t )(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr );
	}
}

void Application::createDepthResources() {
	VkFormat depthFormat = findDepthFormat();

	createImage( swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory );
	depthImageView = createImageView( depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT );

	//transitionImageLayout( depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL );
}









bool Application::checkExtensionMatch( uint32_t neededExtCount, const char** neededExtList, uint32_t vkSupportedExtCount, std::vector<VkExtensionProperties>vkSupportedExtVector ) {
	bool retVal = true;
	bool match = false;
	if( neededExtCount > vkSupportedExtCount ) {
		retVal = false;
	}
	for( uint32_t i = 0; i < neededExtCount; i++ ) {
		const char* requiredExtName = neededExtList[i];
		match = false;
		for( const auto& extension : vkSupportedExtVector ) {
			if( u_strcmp(extension.extensionName, requiredExtName) ) {
				match = true;
				break;
			}
		}
		if( !match ) {
			retVal = false;
		}
	}

	return retVal;
}

bool Application::checkValidationLayerSupport() {
	uint32_t layerCount;
	bool retVal = true;
	vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

	std::vector<VkLayerProperties> availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

	for( const char* layerName : validationLayers ) {
		bool layerFound = false;

		for( const auto& layerProperties : availableLayers ) {
			if( u_strcmp( layerProperties.layerName, layerName ) ) {
				layerFound = true;
				break;
			}
		}
		if( !layerFound ) {
			retVal = false;
		}
	}

	return retVal;
}

bool Application::isDeviceSuitable( VkPhysicalDevice device ) {
	QueueFamilyIndices indices = findQueueFamilies( device );

	bool extensionsSupported = checkDeviceExtensionSupport( device );
	bool swapChainAdequate = false;

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures( device, &supportedFeatures );

	if( extensionsSupported ) {
		SwapChainSupportDetails details = querySwapChainDetails( device );
		swapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
	}

	/* For now, only reqs are graphics queue and sampler anisotropy and non solid fill (for debugging) */
	return indices.graphicsFamily.has_value() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy && supportedFeatures.fillModeNonSolid; 
}

bool Application::checkDeviceExtensionSupport( VkPhysicalDevice device ) {
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );

	std::vector<VkExtensionProperties> availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data() );

	bool result = checkExtensionMatch( ( uint32_t )deviceExtensions.size(), ( const char** )deviceExtensions.data(), ( uint32_t )availableExtensions.size(), availableExtensions );

	return result;
}

int Application::rateDeviceSuitability( VkPhysicalDevice device ) {
	int score = 0;

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties( device, &deviceProperties );

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures( device, &deviceFeatures );

	if( deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) {
		score += 1000;
	}

	score += deviceProperties.limits.maxImageDimension2D;

	return score;
}

Application::QueueFamilyIndices Application::findQueueFamilies( VkPhysicalDevice device ) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

	std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

	uint32_t i = 0;
	for( const auto& queueFamily : queueFamilies ) {

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );

		if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
			indices.graphicsFamily = i;
		}
		if( presentSupport ) {
			indices.presentFamily = i;
		}

		if( indices.isComplete() ) {
			break;
		}

		i++;
	}
	return indices;
}

Application::SwapChainSupportDetails Application::querySwapChainDetails( VkPhysicalDevice device ) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );
	
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr );
	if( formatCount > 0 ) {
		details.formats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, details.formats.data() );
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr );
	if( presentModeCount > 0 ) {
		details.presentModes.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, details.presentModes.data() );
	}

	return details;
}

VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&availableFormats) {
	for( const auto& availableFormat : availableFormats ) {
		if( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
			return availableFormat;
		}
	}
	// Could have code ranking how "good" available formats are (i.e. prefer 4 component 32bit formats)
	return availableFormats[0];
}

VkPresentModeKHR Application::chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes ) {
	for( const auto& availablePresentMode : availablePresentModes ) {
		if( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR ) {
			return VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR; //-> Guaranteed to be available
}

VkExtent2D Application::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities) {
	if( capabilities.currentExtent.width != UINT32_MAX ) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize( window, &width, &height );

		VkExtent2D actualExtent = {
			( uint32_t )width,
			( uint32_t )height
		};

		actualExtent.width = std::clamp( actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
		actualExtent.height = std::clamp( actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

		return actualExtent;
	}
}

VkShaderModule Application::createShaderModule(const std::vector<char>&code) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = 0;
	createInfo.flags = 0;
	createInfo.codeSize = code.size();
	createInfo.pCode = ( const uint32_t* )code.data();

	VkShaderModule shaderModule;
	if( vkCreateShaderModule( device, &createInfo, nullptr, &shaderModule ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create shader module" );
	}
	return shaderModule;
}

uint32_t Application::findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties ) {
	// Get physical device memory properties
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties( physicalDevice, &memProperties );

	// Find suitable memory type
	for( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ ) {
		if( ( typeFilter & (1 << i) ) && ( ( memProperties.memoryTypes[i].propertyFlags & properties ) == properties ) ){
			return i;
		}
	}

	throw std::runtime_error( "Failed to find suitable memory type" );

}

void Application::copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size ) {
	// Should create separate command pool and use 
	// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT when scaling to use multiple buffers

	// All buffer copying is done with a command buffer, so a command buffer must 
	// be created, and recorded on to copy src -> dst

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );

	endSingleTimeCommands( commandBuffer );
}

VkCommandBuffer Application::beginSingleTimeCommands() {
	/* Creates, begins, and returns a command buffer to be recorded into */

	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = commandPool;
	allocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	
	if( vkAllocateCommandBuffers( device, &allocateInfo, &commandBuffer ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to allocate command buffer!" );
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer( commandBuffer, &beginInfo );

	return commandBuffer;
}

void Application::endSingleTimeCommands( VkCommandBuffer commandBuffer ) {
	/* Ends and submits a written command buffer to the graphics queue */

	vkEndCommandBuffer( commandBuffer );
	VkSubmitInfo submitInfo{};

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// We can use the graphics queue here as graphics/compute queues can handle transfers
	// Alternatively, we can create a separate transfer queue and use that here instead
	// If we do so, we'd need to implement a method to share data between queues
	if( vkQueueSubmit( graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE ) != VK_SUCCESS ) {
		throw std::runtime_error( " Failed to submit command buffer to graphics queue " );
	}
	vkQueueWaitIdle( graphicsQueue );

	vkFreeCommandBuffers( device, commandPool, 1, &commandBuffer );

}

void Application::transitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout ) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	/* Barrier will be used to transition image layouts, not transfer queue family ownership */
	/* If queue family ownership is to be transfered, replace ignored macro with queue family indices */
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	/* Want image color ( pixels ) */
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	/* For now, no mipmapping and image is not in array */
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage, dstStage;
	if( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		throw std::runtime_error( "Unsupported layout transition!" );
	}

	vkCmdPipelineBarrier( 
		commandBuffer,
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands( commandBuffer );
}

void Application::copyBufferToImage( VkBuffer buffer, VkImage image, uint32_t width, uint32_t height ) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy copyRegion{};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;

	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Want colour of image (pixel data if texture)

	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	
	copyRegion.imageOffset = { 0, 0, 0 };
	copyRegion.imageExtent = { width, height, 1 };
	
	/* Assuming the image has already been transitioned to the optimal transfer dst layout */
	vkCmdCopyBufferToImage( commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion );

	endSingleTimeCommands( commandBuffer );
}

VkImageView Application::createImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags ) {
	VkImageView imageView;
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	if( vkCreateImageView( device, &createInfo, nullptr, &imageView ) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create image view" );
	}

	return imageView;
}

//void Application::initGui() {
//	/* Should just have this stored */
//	QueueFamilyIndices familyIndices = findQueueFamilies( physicalDevice );
//
//	ImGui_ImplVulkan_InitInfo initInfo{};
//	initInfo.Instance = instance;
//	initInfo.PhysicalDevice = physicalDevice;
//	initInfo.Device = device;
//	initInfo.QueueFamily = familyIndices.graphicsFamily.value();
//	initInfo.Queue = graphicsQueue;
//	initInfo.PipelineCache = VK_NULL_HANDLE;
//	initInfo.Subpass = 0;
//	initInfo.MinImageCount = 2;
//	initInfo.ImageCount = MAX_FRAMES_IN_FLIGHT;
//	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
//	initInfo.Allocator = VK_NULL_HANDLE;
//	initInfo.CheckVkResultFn = VK_NULL_HANDLE;
//
//
//	VkCommandBufferAllocateInfo allocateInfo{};
//	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//	allocateInfo.commandPool = commandPool;
//	allocateInfo.commandBufferCount = 1;
//
//	VkCommandBuffer commandBuffer;
//
//	if( vkAllocateCommandBuffers( device, &allocateInfo, &commandBuffer ) != VK_SUCCESS ) {
//		throw std::runtime_error( "Failed to allocate command buffer!" );
//	}
//
//	VkDescriptorPool descriptorPool;
//	VkDescriptorPoolSize poolSizes[] =
//	{
//		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
//		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
//		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
//		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
//		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
//		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
//		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
//		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
//		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
//		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
//		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
//	};
//	VkDescriptorPoolCreateInfo createInfo = {};
//	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
//	createInfo.maxSets = 1000 * IM_ARRAYSIZE( poolSizes );
//	createInfo.poolSizeCount = ( uint32_t )IM_ARRAYSIZE( poolSizes );
//	createInfo.pPoolSizes = poolSizes;
//	if( vkCreateDescriptorPool( device, &createInfo, VK_NULL_HANDLE, &descriptorPool ) != VK_SUCCESS ) {
//		throw std::runtime_error( "Failed to allocated descriptor pool for gui!" );
//	}
//
//	initInfo.DescriptorPool = descriptorPool;
//
//	appGui = new Gui(window, &initInfo, renderPass, commandBuffer, graphicsPipeline, commandPool );
//}

VkFormat Application::findSupportedFormat( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features ) {
	for( VkFormat format : candidates ) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties( physicalDevice, format, &properties );

		if( tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features ) {
			return format;
		}
		else if( tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features ) {
			return format;
		}

	}

	throw std::runtime_error( "Failed to find supported format!" );
}

VkFormat Application::findDepthFormat() {
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat Application::findTextureFormat() {
	return findSupportedFormat(
		{ VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SNORM},
		VK_IMAGE_TILING_LINEAR,
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT
	);
}

bool Application::hasStencilComponent( VkFormat format ) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}



void reconstructVertices(std::vector<Vertex>& vec, float* data, unsigned int dataSize) {
	for ( size_t i = 0; i < dataSize; i += VERTEX_NUM_FLOATS ) {
		vec.push_back(Vertex{ {data[i], data[i + 1], data[i + 2]}, {data[i + 3], data[i + 4], data[i + 5]}, {data[i + 6], data[i + 7]} });
	}
}

void Application::initWorld() {
	world = new World();


	/* Create new vertex and index buffers for the world */
	createWorldBuffers();
}

/* The general 2:3 ratio of vertices : indices should be rethought when more efficient mesh algos are used */

void Application::createWorldBuffers() {
	{
		/* Vertex Buffer Block */

		unsigned int vertCount = world->getVertexCount();
		if ( (vertCount / 4) * 6 > WORLD_MAX_INDICES ) {
			WORLD_MAX_INDICES = (vertCount / 4) * 6;
		}

		VkDeviceSize bufferSize = sizeof(Vertex) * vertCount;

		/* Reconstruct float data into parseable vertexes */
		std::vector<Vertex> vec{};
		reconstructVertices(vec, world->releaseVertexData(), world->getVertexCount() * VERTEX_NUM_FLOATS);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vec.data(), (uint32_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(sizeof(Vertex) * WORLD_MAX_INDICES, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, worldVertexBuffer, worldVertexBufferMemory);

		copyBuffer(stagingBuffer, worldVertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}



	{
		/* Index Buffer Block */

		unsigned int indCount = world->getIndexCount();
		if ( indCount > WORLD_MAX_INDICES ) {
			WORLD_MAX_INDICES = indCount;
		}

		VkDeviceSize bufferSize2 = sizeof(unsigned int) * indCount;

		VkBuffer stagingBuffer2;
		VkDeviceMemory stagingBufferMemory2;
		createBuffer(bufferSize2, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer2, stagingBufferMemory2);

		void* data2;
		vkMapMemory(device, stagingBufferMemory2, 0, bufferSize2, 0, &data2);
		memcpy(data2, world->releaseIndices(), (uint32_t)bufferSize2);
		vkUnmapMemory(device, stagingBufferMemory2);

		createBuffer(sizeof(Vertex) * WORLD_MAX_INDICES, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, worldIndexBuffer, worldIndexBufferMemory);

		copyBuffer(stagingBuffer2, worldIndexBuffer, bufferSize2);
		vkDestroyBuffer(device, stagingBuffer2, nullptr);
		vkFreeMemory(device, stagingBufferMemory2, nullptr);
	}
}

void Application::updateWorldBuffers() {
	unsigned int indCount = world->getIndexCount();
	if ( indCount > WORLD_MAX_INDICES ) {
		/* Buffer space is not sufficient, needs to be recreated */
		WORLD_MAX_INDICES = indCount;

		deleteWorldBuffers();
		createWorldBuffers();
		return;
	}


	{
		/* Vertex Buffer Block */
		VkDeviceSize bufferSize = sizeof(Vertex) * world->getVertexCount();

		/* Reconstruct float data into parseable vertexes */
		std::vector<Vertex> vec{};
		reconstructVertices(vec, world->releaseVertexData(), world->getVertexCount() * VERTEX_NUM_FLOATS);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vec.data(), (uint32_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		copyBuffer(stagingBuffer, worldVertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}



	{
		/* Index Buffer Block */
		VkDeviceSize bufferSize2 = sizeof(unsigned int) * world->getIndexCount();

		VkBuffer stagingBuffer2;
		VkDeviceMemory stagingBufferMemory2;
		createBuffer(bufferSize2, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer2, stagingBufferMemory2);

		void* data2;
		vkMapMemory(device, stagingBufferMemory2, 0, bufferSize2, 0, &data2);
		memcpy(data2, world->releaseIndices(), (uint32_t)bufferSize2);
		vkUnmapMemory(device, stagingBufferMemory2);

		copyBuffer(stagingBuffer2, worldIndexBuffer, bufferSize2);

		vkDestroyBuffer(device, stagingBuffer2, nullptr);
		vkFreeMemory(device, stagingBufferMemory2, nullptr);
	}
}

void Application::deleteWorldBuffers() {
	vkDestroyBuffer(device, worldVertexBuffer, nullptr);
	vkFreeMemory(device, worldVertexBufferMemory, nullptr);

	vkDestroyBuffer(device, worldIndexBuffer, nullptr);
	vkFreeMemory(device, worldIndexBufferMemory, nullptr);
}