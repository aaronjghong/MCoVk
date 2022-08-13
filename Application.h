#ifndef APPLICATION_H
#define APPLICATION_H

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // Might break nested structures -> use alignas(16) if nested
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define MAX_FRAMES_IN_FLIGHT 2	// How many frames should be processed concurrently

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <algorithm>
#include <array>

#include "util.h"
#include "Camera.h"

#include "Vertex.h"
#include "World.h"

//#include "ImGui/imgui.h"
//#include "ImGui/imgui_impl_glfw.h"
//#include  "ImGui/imgui_impl_vulkan.h"

class Application {
public:
	void run();

	bool frameBufferResized = false;

	inline static float fovDegrees = 60.0f;

	void refreshGraphics();

	/* Not sure if this should be private as of now */
	//class gui {
	//	public:
	//		gui( glfwwindow* window, imgui_implvulkan_initinfo *initinfo, vkrenderpass renderpass, vkcommandbuffer commandbuffer, vkpipeline pipeline, vkcommandpool commandpool ) :io( new imguiio() ) {
	//			imgui_checkversion();
	//			imgui::createcontext();
	//			*io = imgui::getio();
	//			currwindow = window;
	//			this->commandbuffer = commandbuffer;
	//			this->commandpool = commandpool;
	//			this->device = initinfo->device;
	//			this->pipeline = pipeline;
	//			queue = initinfo->queue;

	//			imgui_implglfw_initforvulkan( window, true );
	//			imgui_implvulkan_init( initinfo, renderpass );

	//			imgui::stylecolorsdark();

	//			vkcommandbufferbegininfo begininfo{};
	//			begininfo.stype = vk_structure_type_command_buffer_begin_info;
	//			begininfo.flags |= vk_command_buffer_usage_one_time_submit_bit;
	//			vkbegincommandbuffer( commandbuffer, &begininfo );

	//			imgui_implvulkan_createfontstexture( commandbuffer );

	//			vksubmitinfo submitinfo{};
	//			submitinfo.stype = vk_structure_type_submit_info;
	//			submitinfo.commandbuffercount = 1;
	//			submitinfo.pcommandbuffers = &commandbuffer;
	//			if( vkendcommandbuffer( commandbuffer ) != vk_success ) {
	//				throw std::runtime_error( "failed to end command buffer for font rasterization" );
	//			}
	//			if( vkqueuesubmit(queue, 1, &submitinfo, vk_null_handle) != vk_success ) {
	//				throw std::runtime_error( "failed to submit queue for font rasterization" );
	//			}
	//			vkdevicewaitidle( initinfo->device );
	//			imgui_implvulkan_destroyfontuploadobjects();

	//		}
	//		void guibeginframe() {
	//			imgui_implvulkan_newframe();
	//			imgui_implglfw_newframe();
	//			imgui::newframe();
	//		}
	//		void guirenderstring( const char* str ) {
	//			imgui::begin( "window" );
	//			imgui::text( str );
	//			imgui::end();
	//		}
	//		void guiendframe() {
	//			vkcommandbufferallocateinfo allocateinfo{};
	//			allocateinfo.stype = vk_structure_type_command_buffer_allocate_info;
	//			allocateinfo.level = vk_command_buffer_level_primary;
	//			allocateinfo.commandpool = commandpool;
	//			allocateinfo.commandbuffercount = 1;

	//			if( vkallocatecommandbuffers( device, &allocateinfo, &commandbuffer ) != vk_success ) {
	//				throw std::runtime_error( "failed to allocate command buffer!" );
	//			}

	//			vkcommandbufferbegininfo begininfo{};
	//			begininfo.stype = vk_structure_type_command_buffer_begin_info;
	//			begininfo.flags |= vk_command_buffer_usage_one_time_submit_bit;
	//			vkbegincommandbuffer( commandbuffer, &begininfo );
	//			imgui::render();
	//			imdrawdata* drawdata = imgui::getdrawdata();

	//			vkrenderpassbegininfo renderpassinfo{};
	//			renderpassinfo.stype = vk_structure_type_render_pass_begin_info;
	//			renderpassinfo.renderpass = &renderpass;
	//			renderpassinfo.framebuffer = swapchainframebuffers[imageindex];
	//			renderpassinfo.renderarea.offset = { 0, 0 };
	//			renderpassinfo.renderarea.extent = swapchainextent;
	//			vkclearvalue clearcolor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	//			renderpassinfo.clearvaluecount = 1;
	//			renderpassinfo.pclearvalues = &clearcolor;

	//			vkcmdbeginrenderpass(commandbuffer, )

	//			imgui_implvulkan_renderdrawdata( drawdata, commandbuffer, pipeline );

	//			

	//			//rendertoframe( currwindow, drawdata );
	//			//presentframe( currwindow );
	//		}

	//	private:
	//		glfwwindow* currwindow;
	//		imguiio* io;
	//		vkcommandbuffer commandbuffer;
	//		vkpipeline pipeline;
	//		vkqueue queue;
	//		vkcommandpool commandpool;
	//		vkdevice device;
	//		
	//		void rendertoframe( glfwwindow* window, imdrawdata* drawdata );
	//		void presentframe( glfwwindow* window );
	//};

	//Gui* appGui;

	World* world;


private:
	GLFWwindow* window;

	VkInstance instance;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	VkDevice device;

	VkQueue graphicsQueue;

	VkQueue presentQueue;

	VkSurfaceKHR surface;

	VkSwapchainKHR swapChain;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFrameBuffers;

	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;

	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;

	std::vector<VkCommandBuffer> commandBuffers;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	
	size_t currentFrame = 0;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 800;

	VkFormat imageFormat;

	/* Could have these 4 as arrays/vectors for multiple textures and store idx in hashmap */
	VkImage textureImage;
	VkImageView textureImageView;
	VkDeviceMemory textureImageMemory;
	VkSampler textureSampler;

	/* Need a depth image per concurrent draw op */
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value(); // For now, only care about graphics queue family
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	/*struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription	bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof( Vertex );
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof( Vertex, pos );

			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof( Vertex, color );
			
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof( Vertex, texCoord );
			
			return attributeDescriptions;
		}
	};*/

	struct VertexPushConstant {
		glm::vec4 data;
		glm::mat4 renderMatrix;
	};

	const std::vector<Vertex> vertices = {
		// Front
		{{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},	// 0
		{{1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},	// 1
		{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},	// 2
		{{0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},	// 3

		// Back
		{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},	// 4
		{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},	// 5
		{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},	// 6
		{{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}	// 7
	};

	/*
	
		7----6
		|	 |
		4----5
			3----2
			|	 |
			0----1
	*/

	const std::vector<uint32_t> indices = {
		// front
		0, 1, 2,
		2, 3, 0,
		// right
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// left
		4, 0, 3,
		3, 7, 4,
		// bottom
		4, 5, 1,
		1, 0, 4,
		// top
		3, 2, 6,
		6, 7, 3
	};

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	UniformBufferObject ubo;

	VkBuffer worldVertexBuffer;
	VkDeviceMemory worldVertexBufferMemory;
	VkBuffer worldIndexBuffer;
	VkDeviceMemory worldIndexBufferMemory;

	const bool enableValidationLayers = false; //->false for release builds

	void initWindow();

	void initVulkan();

	void mainLoop();

	void drawFrame();

	void cleanup();

	void createInstance();

	void createSurface();

	void setupDebugMessenger();

	void pickPhysicalDevice();

	void createLogicalDevice();

	void createSwapChain();

	void createImageViews();

	void createRenderPass();

	void createGraphicsPipeline();

	void createFrameBuffers();

	void createCommandPool();

	void createVertexBuffer();

	void createCommandBuffers();

	void recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex );

	void createSyncObjects();

	void cleanupSwapChain();

	void recreateSwapChain();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);

	void createIndexBuffer();

	void createDescriptorSetLayout();

	void createUniformBuffers();

	void updateUniformBuffer(uint32_t currentImage);

	void createDescriptorPool();

	void createDescriptorSets();

	void createImage( uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory );

	void createTextureImage();

	void createTextureImageView();

	void createTextureSampler();

	void createDepthResources();



	/* These could be taken out of the class */

	bool checkExtensionMatch( uint32_t neededExtCount, const char** neededExtList, uint32_t vkSupportedExtCount, std::vector<VkExtensionProperties>vkSupportedExtVector );

	bool checkValidationLayerSupport();

	bool isDeviceSuitable( VkPhysicalDevice device );

	bool checkDeviceExtensionSupport( VkPhysicalDevice device );

	int rateDeviceSuitability( VkPhysicalDevice device );

	QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device );

	SwapChainSupportDetails querySwapChainDetails( VkPhysicalDevice device );

	VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats );

	VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes );

	VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities );

	VkShaderModule createShaderModule( const std::vector<char>& code );

	uint32_t findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties );

	void copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size );

	VkCommandBuffer beginSingleTimeCommands();

	void endSingleTimeCommands( VkCommandBuffer commandBuffer );

	void transitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout );

	void copyBufferToImage( VkBuffer buffer, VkImage image, uint32_t width, uint32_t height );

	VkImageView createImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags );

	//void initGui();

	VkFormat findSupportedFormat( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features );

	VkFormat findDepthFormat();

	VkFormat findTextureFormat();

	bool hasStencilComponent( VkFormat format );

	void updateMVP(uint32_t currentImage);

	void initWorld();

	void createWorldBuffers();

	void updateWorldBuffers();

	void deleteWorldBuffers();
};

#endif