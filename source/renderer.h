#pragma once
// minimalistic code to draw a single triangle, this is not part of the API.
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#ifdef _WIN32 // must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif

#define KHRONOS_STATIC
#include <ktxvulkan.h>

#include <Windows.h>
#include <commdlg.h>
#include "VkHelper.h"
#include "leveldata.h"


#define MAX_MESH_PER_DRAW 1024
#define MAX_LIGHT_PER_DRAW 1024
#define MAX_TEXTURES 1024
#define NEAR_PLANE 0.1f
#define FAR_PLANE 1000.0f

struct SHADER_DATA
{
	GW::MATH::GVECTORF lightDirection, lightColor, lightCount;
	GW::MATH::GVECTORF ambient, camPos;
	GW::MATH::GMATRIXF view, projection;
	GW::MATH::GMATRIXF matricies[MAX_MESH_PER_DRAW];
	H2B::ATTRIBUTES materials[MAX_MESH_PER_DRAW];
	H2B::LIGHT lights[MAX_LIGHT_PER_DRAW];

	SHADER_DATA() = default;
};

struct RENDER_INSTANCE_DATA
{
	unsigned int meshID;
	unsigned int materialID;
	unsigned int hasColorTexture;
};

struct VK_TEXURING
{
	ktxVulkanTexture texture;
	VkImageView view;
};

// Creation, Rendering & Cleanup
class Renderer
{
private:
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;

	GW::INPUT::GInput kbmProxy;
	GW::INPUT::GController controllerProxy;
	GW::INPUT::GBufferedInput bufferedInput;
	GW::CORE::GEventResponder eventResponder;
	bool dialogBoxOpen = false;

	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;

	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	VkBuffer indexHandle = nullptr;
	VkDeviceMemory indexData = nullptr;

	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;
	VkShaderModule vertexShaderSkybox = nullptr;
	VkShaderModule pixelShaderSkybox = nullptr;

	// pipeline settings for drawing (also required)
	VkPipeline meshPipeline = nullptr;
	VkPipelineLayout meshPipelineLayout = nullptr;
	VkPipeline skyboxPipeline = nullptr;
	VkPipelineLayout skyboxPipelineLayout = nullptr;

	// descriptor set(s), pool
	std::vector<VkBuffer> storageBufferHandle;
	std::vector<VkDeviceMemory> storageBufferData;
	std::vector<VkDescriptorSet> descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout = nullptr;
	VkDescriptorSetLayout skyboxDescriptorSetLayout = nullptr;
	VkDescriptorPool descriptorPool = nullptr;

	VkSampler textureSampler = nullptr;
	std::vector<ktxVulkanTexture> textures2D;
	std::vector<VkImageView> textureViews2D;
	std::vector<ktxVulkanTexture> textures3D;
	std::vector<VkImageView> textureViews3D;
	ktxVulkanTexture default2DTexture;
	VkImageView defaultTexture2DView;
	ktxVulkanTexture default3DTexture;
	VkImageView defaultTexture3DView;

	SHADER_DATA* modelData = nullptr;

	GW::MATH::GMatrix matrixProxy;
	GW::MATH::GVector vectorProxy;

	GW::MATH::GMATRIXF worldCamera;
	GW::MATH::GMATRIXF directionalLight;

	LEVELDATA currentLevel;

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk);
	void Update(float deltaTime);
	void Render();

private:
	void CleanUp();
	std::string ShaderAsString(const char* shaderFilePath);
	bool CreateVertexShader(const char* shaderFilePath, shaderc_compiler_t compiler, shaderc_compile_options_t options, VkShaderModule* shaderModule);
	bool CreatePixelShader(const char* shaderFilePath, shaderc_compiler_t compiler, shaderc_compile_options_t options, VkShaderModule* shaderModule);
	bool CreateVulkanBuffer(VkPhysicalDevice physicalDevice, const VkBufferUsageFlags& bufferType, const void* pData, const VkDeviceSize& sizeInBytes, VkBuffer& pBuffer, VkDeviceMemory& pMemory);
	bool LoadDataFromLevel(const VkPhysicalDevice& physicalDevice, const std::string levelFilePath);
	void UpdateCamera(float deltaTime);
	bool OpenFileDialogBox(GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE windowHandle, std::string& fileName);
	bool LoadTexture(std::string texturePath, ktxVulkanTexture& texture, VkImageView& textureView);
};

Renderer::Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
{
	win = _win;
	vlk = _vlk;
	unsigned int width, height;
	+win.GetClientWidth(width);
	+win.GetClientHeight(height);
	GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh;
	+win.GetWindowHandle(uwh);

	+kbmProxy.Create(_win);
	+controllerProxy.Create();
	+bufferedInput.Create(_win);

	unsigned int maxNumFrames = 0;
	vlk.GetSwapchainImageCount(maxNumFrames);
	storageBufferHandle.resize(maxNumFrames);
	storageBufferData.resize(maxNumFrames);
	descriptorSet.resize(maxNumFrames);


	/***************** GEOMETRY INTIALIZATION ******************/
	// Grab the device & physical device so we can allocate some stuff
	VkPhysicalDevice physicalDevice = nullptr;
	vlk.GetDevice((void**)&device);
	vlk.GetPhysicalDevice((void**)&physicalDevice);

	+eventResponder.Create([=](const GW::GEvent& e)
	{
		GW::INPUT::GBufferedInput::Events q;
		GW::INPUT::GBufferedInput::EVENT_DATA qd;
		bool bReadIsGood = +e.Read(q, qd);
		bool bBufferedInputKey1 = (q == GW::INPUT::GBufferedInput::Events::KEYRELEASED && qd.data == G_KEY_1);
		if (bReadIsGood && bBufferedInputKey1 && !dialogBoxOpen)
		{
			dialogBoxOpen = true;
			std::string levelName = std::string("");
			if (OpenFileDialogBox(uwh, levelName))
			{
				vkDeviceWaitIdle(device);
				LoadDataFromLevel(physicalDevice, levelName);
			}
			dialogBoxOpen = false;
		}
	});
	+bufferedInput.Register(eventResponder);

	modelData = new SHADER_DATA();

	for (size_t i = 0; i < maxNumFrames; ++i)
	{
		CreateVulkanBuffer(physicalDevice, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			modelData, sizeof(SHADER_DATA),
			storageBufferHandle[i], storageBufferData[i]);
	}

	bool bTexture2D = LoadTexture("../textures/default2dtexture.ktx", default2DTexture, defaultTexture2DView);
	bool bTexture3D = LoadTexture("../textures/default3dtexture.ktx", default3DTexture, defaultTexture3DView);

	/***************** SHADER INTIALIZATION ******************/
	// Intialize runtime shader compiler HLSL -> SPIRV
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compile_options_t options = shaderc_compile_options_initialize();
	shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
	shaderc_compile_options_set_invert_y(options, false);
#ifndef NDEBUG
	shaderc_compile_options_set_generate_debug_info(options);
#endif
	// Create Vertex Shader
	if (!CreateVertexShader("../shaders/VertexShader.hlsl", compiler, options, &vertexShader))
	{
		OutputDebugString(L"***ERROR!***\nVertexShader did not load properly!\n***ERROR***\n");
	}
	// Create Pixel Shader
	if (!CreatePixelShader("../shaders/PixelShader.hlsl", compiler, options, &pixelShader))
	{
		OutputDebugString(L"***ERROR!***\nPixelShader did not load properly!\n***ERROR***\n");
	}
	// Create Pixel Shader
	if (!CreateVertexShader("../shaders/VertexShaderSkybox.hlsl", compiler, options, &vertexShaderSkybox))
	{
		OutputDebugString(L"***ERROR!***\nVertexShaderSkybox did not load properly!\n***ERROR***\n");
	}
	// Create Pixel Shader
	if (!CreatePixelShader("../shaders/PixelShaderSkybox.hlsl", compiler, options, &pixelShaderSkybox))
	{
		OutputDebugString(L"***ERROR!***\nPixelShaderSkybox did not load properly!\n***ERROR***\n");
	}
	// Free runtime shader compiler resources
	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);

	/***************** PIPELINE INTIALIZATION ******************/
	// Create Pipeline & Layout (Thanks Tiny!)
	VkRenderPass renderPass;
	vlk.GetRenderPass((void**)&renderPass);

	CVkPipelineShaderStageCreateInfo stage_create_info[] =
	{
		CVkPipelineShaderStageCreateInfo("main", vertexShader, VK_SHADER_STAGE_VERTEX_BIT),
		CVkPipelineShaderStageCreateInfo("main", pixelShader, VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	// Assembly State
	CVkPipelineInputAssemblyStateCreateInfo assembly_create_info =
		CVkPipelineInputAssemblyStateCreateInfo(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	// Vertex Input State 
	CVkVertexInputBindingDescription vertex_binding_description =
		CVkVertexInputBindingDescription(
			0,
			sizeof(H2B::VERTEX),
			VK_VERTEX_INPUT_RATE_VERTEX);

	// Vertex Input Attributes
	CVkVertexInputAttributeDescription vertex_attribute_description[] =
	{
		CVkVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(H2B::VERTEX, pos)), // position
		CVkVertexInputAttributeDescription(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(H2B::VERTEX, uvw)), // texcoords
		CVkVertexInputAttributeDescription(2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(H2B::VERTEX, nrm))  // normals
	};

	// Pipeline Vertex Input State Create Information
	CVkPipelineVertexInputStateCreateInfo input_vertex_info =
		CVkPipelineVertexInputStateCreateInfo(
			1, &vertex_binding_description,
			ARRAYSIZE(vertex_attribute_description), vertex_attribute_description);
	// Viewport State (we still need to set this up even though we will overwrite the values)
	CVkViewport viewport =
		CVkViewport(0, 0,
			static_cast<float>(width), static_cast<float>(height),
			0, 1);
	// Vulkan Scissor rect
	CVkRect2D scissor =
		CVkRect2D(
			0, 0,
			width, height);

	// Viewport Create Information
	CVkPipelineViewportStateCreateInfo viewport_create_info =
		CVkPipelineViewportStateCreateInfo(
			1, &viewport,
			1, &scissor);

	// Rasterizer State
	CVkPipelineRasterizationStateCreateInfo rasterization_create_info =
		CVkPipelineRasterizationStateCreateInfo(
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_BACK_BIT,
			VK_FRONT_FACE_CLOCKWISE,
			1.0f);

	// Multisampling State
	CVkPipelineMultisampleStateCreateInfo multisample_create_info =
		CVkPipelineMultisampleStateCreateInfo(
			VK_SAMPLE_COUNT_8_BIT);

	// Depth-Stencil State
	VkStencilOpState opStates[2] = {};
	CVkPipelineDepthStencilStateCreateInfo depth_stencil_create_info =
		CVkPipelineDepthStencilStateCreateInfo(
			VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, 0.0f, 1.0f,
			VK_FALSE, opStates[0], opStates[1]);

	// Color Blending Attachment & State
	VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
	color_blend_attachment_state.colorWriteMask = 0xF;
	color_blend_attachment_state.blendEnable = VK_FALSE;
	color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
	color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
	color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
	color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_create_info.logicOpEnable = VK_FALSE;
	color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
	color_blend_create_info.attachmentCount = 1;
	color_blend_create_info.pAttachments = &color_blend_attachment_state;
	color_blend_create_info.blendConstants[0] = 0.0f;
	color_blend_create_info.blendConstants[1] = 0.0f;
	color_blend_create_info.blendConstants[2] = 0.0f;
	color_blend_create_info.blendConstants[3] = 0.0f;
	// Dynamic State 
	VkDynamicState dynamic_state[2] =
	{
		// By setting these we do not need to re-create the pipeline on Resize
		VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
	dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_create_info.dynamicStateCount = 2;
	dynamic_create_info.pDynamicStates = dynamic_state;

	// is this ext feature supported?
	VkDescriptorBindingFlags descriptor_binding_flags[2] =
	{
		0,
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
	};

	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT layout_binding_flags_create_info = {};
	layout_binding_flags_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	layout_binding_flags_create_info.bindingCount = ARRAYSIZE(descriptor_binding_flags);
	layout_binding_flags_create_info.pBindingFlags = descriptor_binding_flags;

	// Descriptor Set Layout Binding
	CVkDescriptorSetLayoutBinding set_layout_binding[] =
	{
		CVkDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
		CVkDescriptorSetLayoutBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
		CVkDescriptorSetLayoutBinding(2, MAX_TEXTURES, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	// Descriptor Set Layout Create Information
	CVkDescriptorSetLayoutCreateInfo set_layout_create_info = CVkDescriptorSetLayoutCreateInfo(
		set_layout_binding,
		ARRAYSIZE(set_layout_binding)
	);
	vkCreateDescriptorSetLayout(device, &set_layout_create_info, nullptr, &descriptorSetLayout);

	// create the the image view and sampler
	CVkSamplerCreateInfo samplerInfo(
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		VK_FILTER_LINEAR,
		10
	);
	VkResult vr = vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler);

	// Descriptor Pool Size
	VkDescriptorPoolSize descriptor_pool_size[] =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxNumFrames },							// storage buffer, 1 per backbuffer
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * maxNumFrames },				// skybox texture, 1 per backbuffer
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURES * maxNumFrames },		// "bindless texture" 1000 per backbuffer

	};
	uint32_t numDescriptorSets = 0;
	for (size_t i = 0; i < ARRAYSIZE(descriptor_pool_size); i++)
	{
		numDescriptorSets += descriptor_pool_size[i].descriptorCount;
	}

	// Descriptor Pool Create Information
	VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
	descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.pNext = nullptr;
	descriptor_pool_create_info.flags = 0; // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptor_pool_create_info.maxSets = numDescriptorSets;	//( number of sets in this pool )
	descriptor_pool_create_info.poolSizeCount = ARRAYSIZE(descriptor_pool_size);
	descriptor_pool_create_info.pPoolSizes = descriptor_pool_size;
	vkCreateDescriptorPool(device, &descriptor_pool_create_info, nullptr, &descriptorPool);

	// Descriptor Set Allocate Information
	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.pNext = nullptr;
	descriptor_set_allocate_info.descriptorPool = descriptorPool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts = &descriptorSetLayout;

	VkDescriptorImageInfo diinfo2d = {};
	diinfo2d.imageLayout = default2DTexture.imageLayout;
	diinfo2d.imageView = defaultTexture2DView;
	diinfo2d.sampler = textureSampler;

	VkDescriptorImageInfo diinfo3d = {};
	diinfo3d.imageLayout = default3DTexture.imageLayout;
	diinfo3d.imageView = defaultTexture3DView;
	diinfo3d.sampler = textureSampler;

	VkWriteDescriptorSet write_descriptor_set_binding_2[MAX_TEXTURES] = {};
	// Allocate and Update Descriptor Sets
	for (size_t i = 0; i < maxNumFrames; i++)
	{
		VkResult r = vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptorSet[i]);

		VkDescriptorBufferInfo descriptor_buffer_info = {};
		descriptor_buffer_info.buffer = storageBufferHandle[i];
		descriptor_buffer_info.offset = 0;
		descriptor_buffer_info.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet write_descriptor_set = {};
		// storage buffer at binding 0
		write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_set.pNext = nullptr;
		write_descriptor_set.dstSet = descriptorSet[i];
		write_descriptor_set.dstBinding = 0;
		write_descriptor_set.dstArrayElement = 0;
		write_descriptor_set.descriptorCount = 1;
		write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write_descriptor_set.pImageInfo = nullptr;
		write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
		write_descriptor_set.pTexelBufferView = nullptr;
		vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);

		// cubemap at binding 1
		write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_set.pNext = nullptr;
		write_descriptor_set.dstSet = descriptorSet[i];
		write_descriptor_set.dstBinding = 1;
		write_descriptor_set.dstArrayElement = 0;
		write_descriptor_set.descriptorCount = 1;
		write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_descriptor_set.pImageInfo = &diinfo3d;
		write_descriptor_set.pBufferInfo = nullptr;
		write_descriptor_set.pTexelBufferView = nullptr;
		vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);

		// bindless 2d textures at binding 2
		for (size_t j = 0; j < MAX_TEXTURES; j++)
		{
			write_descriptor_set_binding_2[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_descriptor_set_binding_2[j].pNext = nullptr;
			write_descriptor_set_binding_2[j].dstSet = descriptorSet[i];
			write_descriptor_set_binding_2[j].dstBinding = 2;
			write_descriptor_set_binding_2[j].dstArrayElement = j;
			write_descriptor_set_binding_2[j].descriptorCount = 1;
			write_descriptor_set_binding_2[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write_descriptor_set_binding_2[j].pImageInfo = &diinfo2d;
			write_descriptor_set_binding_2[j].pBufferInfo = nullptr;
			write_descriptor_set_binding_2[j].pTexelBufferView = nullptr;
		}

		vkUpdateDescriptorSets(device, MAX_TEXTURES, write_descriptor_set_binding_2, 0, nullptr);
	}

	// Push Constant Range
	VkPushConstantRange push_constant_range = {};
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(RENDER_INSTANCE_DATA);

	// Descriptor pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = 1;
	pipeline_layout_create_info.pSetLayouts = &descriptorSetLayout;
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
	vkCreatePipelineLayout(device, &pipeline_layout_create_info,
		nullptr, &meshPipelineLayout);

	// Pipeline State... (FINALLY) 
	VkGraphicsPipelineCreateInfo pipeline_create_info = {};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount = ARRAYSIZE(stage_create_info);
	pipeline_create_info.pStages = stage_create_info;
	pipeline_create_info.pInputAssemblyState = &assembly_create_info;
	pipeline_create_info.pVertexInputState = &input_vertex_info;
	pipeline_create_info.pViewportState = &viewport_create_info;
	pipeline_create_info.pRasterizationState = &rasterization_create_info;
	pipeline_create_info.pMultisampleState = &multisample_create_info;
	pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
	pipeline_create_info.pColorBlendState = &color_blend_create_info;
	pipeline_create_info.pDynamicState = &dynamic_create_info;
	pipeline_create_info.layout = meshPipelineLayout;
	pipeline_create_info.renderPass = renderPass;
	pipeline_create_info.subpass = 0;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
		&pipeline_create_info, nullptr, &meshPipeline);


	// Skybox pipeline / descriptor set
	{
		// TODO: set depth testing to equal
		//depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;

		// Create Stage Info for Vertex Shader
		stage_create_info[0] = CVkPipelineShaderStageCreateInfo("main", vertexShaderSkybox, VK_SHADER_STAGE_VERTEX_BIT);
		stage_create_info[1] = CVkPipelineShaderStageCreateInfo("main", pixelShaderSkybox, VK_SHADER_STAGE_FRAGMENT_BIT);

		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pStages = stage_create_info;
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
			&pipeline_create_info, nullptr, &skyboxPipeline);
	}

	/***************** CLEANUP / SHUTDOWN ******************/
	// GVulkanSurface will inform us when to release any allocated resources
	shutdown.Create(vlk, [&]() {
		if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
			CleanUp(); // unlike D3D we must be careful about destroy timing
		}
		});
}

void Renderer::Update(float deltaTime)
{
	UpdateCamera(deltaTime);

	VkPhysicalDevice physicalDevice = nullptr;
	vlk.GetPhysicalDevice((void**)&physicalDevice);

	float aspectRatio = 1.0f;
	GW::GReturn gr;
	gr = vlk.GetAspectRatio(aspectRatio);
	gr = matrixProxy.ProjectionVulkanLHF(G2D_DEGREE_TO_RADIAN(65),
		aspectRatio, NEAR_PLANE, FAR_PLANE,
		modelData->projection);

	modelData->camPos = worldCamera.row4;

	for (auto& skybox : currentLevel.uniqueSkyboxes)
	{
		matrixProxy.TranslateGlobalF(GW::MATH::GIdentityMatrixF, modelData->camPos, modelData->matricies[skybox.second.meshID]);
	}

	// grab the current Vulkan swapchain index
	unsigned int currentBufferIndex;
	vlk.GetSwapchainCurrentImage(currentBufferIndex);
	GvkHelper::write_to_buffer(device, storageBufferData[currentBufferIndex], modelData, sizeof(SHADER_DATA));
}

void Renderer::Render()
{

	// If the current level is empty, leave render
	if (currentLevel.uniqueMeshes.empty() && currentLevel.uniqueSkyboxes.empty())
		return;

	// grab the current Vulkan swapchain index
	unsigned int currentBufferIndex;
	vlk.GetSwapchainCurrentImage(currentBufferIndex);
	// grab the current Vulkan commandBuffer
	VkCommandBuffer commandBuffer;
	vlk.GetCommandBuffer(currentBufferIndex, (void**)&commandBuffer);
	// what is the current client area dimensions?
	unsigned int width, height;
	win.GetClientWidth(width);
	win.GetClientHeight(height);
	// setup the pipeline's dynamic settings
	VkViewport viewport =
	{
		0, 0,
		static_cast<float>(width), static_cast<float>(height),
		0, 1
	};
	VkRect2D scissor =
	{
		{0, 0},
		{width, height}
	};

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

	// now we can draw
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexHandle, 0, VK_INDEX_TYPE_UINT32);


	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelineLayout, 0, 1,
		&descriptorSet[currentBufferIndex], 0, nullptr);

	for (const auto& mesh : currentLevel.uniqueMeshes)
	{
		for (const auto& submesh : mesh.second.subMeshes)
		{
			RENDER_INSTANCE_DATA rid =
			{
				mesh.second.meshID,
				submesh.materialIndex,
				submesh.hasColorTexture
			};
			vkCmdPushConstants(commandBuffer, meshPipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(RENDER_INSTANCE_DATA),
				&rid);

			vkCmdDrawIndexed(commandBuffer,
				submesh.drawInfo.indexCount,
				mesh.second.numInstances,
				submesh.drawInfo.indexOffset,
				mesh.second.vertexOffset,
				0);
		}
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);

	for (const auto& skybox : currentLevel.uniqueSkyboxes)
	{
		for (const auto& submesh : skybox.second.subMeshes)
		{
			RENDER_INSTANCE_DATA rid =
			{
				skybox.second.meshID,
				submesh.materialIndex,
				submesh.hasColorTexture
			};

			vkCmdPushConstants(commandBuffer, meshPipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(RENDER_INSTANCE_DATA),
				&rid);

			vkCmdDrawIndexed(commandBuffer,
				submesh.drawInfo.indexCount,
				skybox.second.numInstances,
				submesh.drawInfo.indexOffset,
				skybox.second.vertexOffset,
				0);
		}
	}
}

void Renderer::CleanUp()
{
	// wait till everything has completed
	vkDeviceWaitIdle(device);

	if (modelData)
	{
		delete modelData;
		modelData = nullptr;
	}

	size_t size = -1;

	// Release allocated buffers, shaders & pipeline
	VK_DESTROY_BUFFER(device, vertexHandle);
	VK_FREE_MEMORY(device, vertexData);

	VK_DESTROY_BUFFER(device, indexHandle);
	VK_FREE_MEMORY(device, indexData);

	size = storageBufferHandle.size();
	for (size_t i = 0; i < size; i++)
	{
		VK_DESTROY_BUFFER(device, storageBufferHandle[i]);
		VK_FREE_MEMORY(device, storageBufferData[i]);
	}

	VK_DESTROY_DESCRIPTOR_SET_LAYOUT(device, skyboxDescriptorSetLayout);
	VK_DESTROY_DESCRIPTOR_SET_LAYOUT(device, descriptorSetLayout);
	VK_DESTROY_DESCRIPTOR_POOL(device, descriptorPool);

	VK_DESTROY_SHADER(device, vertexShader);
	VK_DESTROY_SHADER(device, pixelShader);

	VK_DESTROY_SHADER(device, vertexShaderSkybox);
	VK_DESTROY_SHADER(device, pixelShaderSkybox);

	VK_DESTROY_PIPELINE(device, meshPipeline);
	VK_DESTROY_PIPELINE_LAYOUT(device, meshPipelineLayout);

	VK_DESTROY_PIPELINE(device, skyboxPipeline);
	VK_DESTROY_PIPELINE_LAYOUT(device, skyboxPipelineLayout);

	VK_DESTROY_SAMPLER(device, textureSampler);

	size = textures2D.size();
	for (size_t i = 0; i < size; i++)
	{
		VK_DESTROY_IMAGE_VIEW(device, textureViews2D[i]);
		KTX_DESTROY_TEXTURE(device, textures2D[i]);
	}
	size = textures3D.size();
	for (size_t i = 0; i < size; i++)
	{
		VK_DESTROY_IMAGE_VIEW(device, textureViews3D[i]);
		KTX_DESTROY_TEXTURE(device, textures3D[i]);
	}

	VK_DESTROY_IMAGE_VIEW(device, defaultTexture2DView);
	KTX_DESTROY_TEXTURE(device, default2DTexture);
	VK_DESTROY_IMAGE_VIEW(device, defaultTexture3DView);
	KTX_DESTROY_TEXTURE(device, default3DTexture);
}

std::string Renderer::ShaderAsString(const char* shaderFilePath)
{
	std::string output;
	unsigned int stringLength = 0;
	GW::SYSTEM::GFile file; file.Create();
	file.GetFileSize(shaderFilePath, stringLength);
	if (stringLength && +file.OpenBinaryRead(shaderFilePath))
	{
		output.resize(stringLength);
		file.Read(&output[0], stringLength);
	}
	return output;
}

bool Renderer::CreateVertexShader(const char* shaderFilePath, shaderc_compiler_t compiler, shaderc_compile_options_t options, VkShaderModule* shaderModule)
{
	VkResult vr = VkResult::VK_SUCCESS;
	std::string vertexShaderSource = ShaderAsString(shaderFilePath);
	if (vertexShaderSource.empty())
	{
		return false;
	}
	shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
		compiler, vertexShaderSource.c_str(), strlen(vertexShaderSource.c_str()),
		shaderc_vertex_shader, "main.vert", "main", options);
	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
	vr = GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
		(char*)shaderc_result_get_bytes(result), shaderModule);
	if (vr != VkResult::VK_SUCCESS)
	{
		return false;
	}
	shaderc_result_release(result); // done
	return true;
}

bool Renderer::CreatePixelShader(const char* shaderFilePath, shaderc_compiler_t compiler, shaderc_compile_options_t options, VkShaderModule* shaderModule)
{
	VkResult vr = VkResult::VK_SUCCESS;
	std::string pixelShaderSource = ShaderAsString(shaderFilePath);
	if (pixelShaderSource.empty())
	{
		return false;
	}
	shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
		compiler, pixelShaderSource.c_str(), strlen(pixelShaderSource.c_str()),
		shaderc_fragment_shader, "main.frag", "main", options);
	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
	vr = GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
		(char*)shaderc_result_get_bytes(result), shaderModule);
	if (vr != VkResult::VK_SUCCESS)
	{
		return false;
	}
	shaderc_result_release(result); // done
	return true;
}

bool Renderer::CreateVulkanBuffer(VkPhysicalDevice physicalDevice, const VkBufferUsageFlags& bufferType, const void* pData, const VkDeviceSize& sizeInBytes, VkBuffer& pBuffer, VkDeviceMemory& pMemory)
{
	VkResult vr = VkResult::VK_SUCCESS;
	vr = GvkHelper::create_buffer(physicalDevice, device, sizeInBytes,
		bufferType, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &pBuffer, &pMemory);
	if (vr != VkResult::VK_SUCCESS)
	{
		return false;
	}
	vr = GvkHelper::write_to_buffer(device, pMemory, pData, sizeInBytes);
	if (vr != VkResult::VK_SUCCESS)
	{
		return false;
	}
	return true;
}

bool Renderer::LoadDataFromLevel(const VkPhysicalDevice& physicalDevice, const std::string levelFilePath)
{
	unsigned int maxNumFrames = 0;
	vlk.GetSwapchainImageCount(maxNumFrames);

	size_t size = -1;
	// Release allocated buffers
	VK_DESTROY_BUFFER(device, vertexHandle);
	VK_FREE_MEMORY(device, vertexData);
	VK_DESTROY_BUFFER(device, indexHandle);
	VK_FREE_MEMORY(device, indexData);

	size = textures2D.size();
	for (size_t i = 0; i <size; i++)
	{
		KTX_DESTROY_TEXTURE(device, textures2D[i]);
		VK_DESTROY_IMAGE_VIEW(device, textureViews2D[i]);
	}
	size = textures3D.size();
	for (size_t i = 0; i < size; i++)
	{
		KTX_DESTROY_TEXTURE(device, textures3D[i]);
		VK_DESTROY_IMAGE_VIEW(device, textureViews3D[i]);
	}

	textures2D.clear();
	textureViews2D.clear();
	textures3D.clear();
	textureViews3D.clear();

	currentLevel.Clear();

	if (!currentLevel.LoadLevel(levelFilePath.c_str()))
	{
		std::cout << "Level failed to load!" << std::endl;
		return false;
	}
	// Transfer vertex data to the vertex buffer. (staging would be prefered here)
	if (!currentLevel.vertices.size() ||
		!CreateVulkanBuffer(physicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			currentLevel.vertices.data(), sizeof(H2B::VERTEX) * currentLevel.vertices.size(),
			vertexHandle, vertexData))
	{
		std::cout << "Vertex Buffer failed to create!" << std::endl;
		return false;
	}
	// Transfer index data to the index buffer. (staging would be prefered here)
	if (!currentLevel.indices.size() ||
		!CreateVulkanBuffer(physicalDevice, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			currentLevel.indices.data(), sizeof(unsigned int) * currentLevel.indices.size(),
			indexHandle, indexData))
	{
		std::cout << "Index Buffer failed to create!" << std::endl;
		return false;
	}

	if (modelData)
	{
		ZeroMemory(modelData, sizeof(SHADER_DATA));

		GW::GReturn gr = GW::GReturn::FAILURE;
		worldCamera = currentLevel.world_camera;

		gr = matrixProxy.InverseF(worldCamera, modelData->view);
		float aspectRatio = 1.0f;
		gr = vlk.GetAspectRatio(aspectRatio);
		gr = matrixProxy.ProjectionVulkanLHF(G2D_DEGREE_TO_RADIAN(65),
			aspectRatio, 0.1f, 100.0f,
			modelData->projection);

		modelData->lightDirection = { -1.0f, -1.0f, 3.0f, 0.0f };
		modelData->lightColor = { 0.75f, 0.9f, 1.0f, 1.0f };
		modelData->ambient = { 0.25f, 0.25f, 0.35f, 1.0f };
		modelData->camPos = worldCamera.row4;

		size_t matrixIndex = 0;
		size_t materialIndex = 0;
		size_t lightIndex = 0;
		for (const auto& mesh : currentLevel.uniqueMeshes)
		{
			for (const auto& matrix : mesh.second.matrices)
			{
				modelData->matricies[matrixIndex] = matrix;
				matrixIndex++;
			}
		}
		for (const auto& material : currentLevel.materials2D)
		{
			modelData->materials[materialIndex] = material.attrib;
			materialIndex++;
		}
		for (const auto& light : currentLevel.uniqueLights)
		{
			modelData->lights[lightIndex] = light;
			lightIndex++;
		}
		modelData->lightCount.x = currentLevel.uniqueLights.size();
	}

	for (const auto& material : currentLevel.materials2D)
	{
		if (!material.map_Kd.empty())
		{
			ktxVulkanTexture tex = {};
			VkImageView view = nullptr;
			std::string filePath = "../textures/";
			filePath += material.map_Kd.substr(0, material.map_Kd.size() - 4);
			filePath += ".ktx";
			if (LoadTexture(filePath, tex, view))
			{
				textures2D.push_back(tex);
				textureViews2D.push_back(view);
			}
		}
	}

	for (const auto& material : currentLevel.materials3D)
	{
		if (!material.map_Kd.empty())
		{
			ktxVulkanTexture tex = {};
			VkImageView view = nullptr;
			std::string filePath = "../textures/";
			filePath += material.map_Kd.substr(0, material.map_Kd.size() - 4);
			filePath += ".ktx";
			if (LoadTexture(filePath, tex, view))
			{
				textures3D.push_back(tex);
				textureViews3D.push_back(view);
			}
		}
	}

	std::vector<VkDescriptorImageInfo> image_info_2D;
	for (size_t i = 0; i < textureViews2D.size(); i++)
	{
		VkDescriptorImageInfo info = {};
		info.imageView = textureViews2D[i];
		info.imageLayout = textures2D[i].imageLayout;
		info.sampler = textureSampler;
		image_info_2D.push_back(info);
	}

	std::vector<VkDescriptorImageInfo> image_info_3D;
	for (size_t i = 0; i < textureViews3D.size(); i++)
	{
		VkDescriptorImageInfo info = {};
		info.imageView = textureViews3D[i];
		info.imageLayout = textures3D[i].imageLayout;
		info.sampler = textureSampler;
		image_info_3D.push_back(info);
	}

	VkDescriptorImageInfo default2DImage = {};
	default2DImage.imageLayout = default2DTexture.imageLayout;
	default2DImage.imageView = defaultTexture2DView;
	default2DImage.sampler = textureSampler;

	VkDescriptorImageInfo default3DImage = {};
	default3DImage.imageLayout = default3DTexture.imageLayout;
	default3DImage.imageView = defaultTexture3DView;
	default3DImage.sampler = textureSampler;

	VkWriteDescriptorSet write_descriptor_set_binding_1 = {};
	VkWriteDescriptorSet write_descriptor_set_binding_2[MAX_TEXTURES] = {};

	for (size_t i = 0; i < maxNumFrames; i++)
	{
		write_descriptor_set_binding_1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_set_binding_1.pNext = nullptr;
		write_descriptor_set_binding_1.dstSet = descriptorSet[i];
		write_descriptor_set_binding_1.dstBinding = 1;
		write_descriptor_set_binding_1.dstArrayElement = 0;
		write_descriptor_set_binding_1.descriptorCount = 1;
		write_descriptor_set_binding_1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_descriptor_set_binding_1.pImageInfo = &default3DImage;
		write_descriptor_set_binding_1.pBufferInfo = nullptr;
		write_descriptor_set_binding_1.pTexelBufferView = nullptr;
		vkUpdateDescriptorSets(device, 1, &write_descriptor_set_binding_1, 0, nullptr);

		// bindless 2d textures at binding 2
		for (size_t j = 0; j < MAX_TEXTURES; j++)
		{
			write_descriptor_set_binding_2[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_descriptor_set_binding_2[j].pNext = nullptr;
			write_descriptor_set_binding_2[j].dstSet = descriptorSet[i];
			write_descriptor_set_binding_2[j].dstBinding = 2;
			write_descriptor_set_binding_2[j].dstArrayElement = j;
			write_descriptor_set_binding_2[j].descriptorCount = 1;
			write_descriptor_set_binding_2[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write_descriptor_set_binding_2[j].pImageInfo = &default2DImage;
			write_descriptor_set_binding_2[j].pBufferInfo = nullptr;
			write_descriptor_set_binding_2[j].pTexelBufferView = nullptr;
		}
		vkUpdateDescriptorSets(device, MAX_TEXTURES, write_descriptor_set_binding_2, 0, nullptr);
	}


	VkDescriptorImageInfo texture3D = (image_info_3D.size()) ? image_info_3D[0] : default3DImage;

	for (size_t i = 0; i < maxNumFrames; i++)
	{
		write_descriptor_set_binding_1.dstSet = descriptorSet[i];
		write_descriptor_set_binding_1.pImageInfo = &texture3D;
		vkUpdateDescriptorSets(device, 1, &write_descriptor_set_binding_1, 0, nullptr);


		for (size_t j = 0; j < image_info_2D.size(); j++)
		{
			write_descriptor_set_binding_2[j].dstSet = descriptorSet[i];
			write_descriptor_set_binding_2[j].pImageInfo = &image_info_2D[j];
		}
		vkUpdateDescriptorSets(device, image_info_2D.size(), write_descriptor_set_binding_2, 0, nullptr);
	}

	return true;
}

void Renderer::UpdateCamera(float deltaTime)
{
	float kbm_y_change_up = 0.0f;
	float kbm_y_change_down = 0.0f;
	float ctr_y_change_up = 0.0f;
	float ctr_y_change_down = 0.0f;
	const float camera_speed = 0.5f;
	float mouse_r_button = 0.0f;
	float per_frame_speed = camera_speed * deltaTime;
	float kbm_z_change_forward = 0.0f;
	float kbm_z_change_backward = 0.0f;
	float ctr_z_change = 0.0f;
	float kbm_x_change_left = 0.0f;
	float kbm_x_change_right = 0.0f;
	float ctr_x_change = 0.0f;

	bool move_up_changed =
		(G_PASS(kbmProxy.GetState(G_KEY_SPACE, kbm_y_change_up)) && kbm_y_change_up) ||
		(G_PASS(controllerProxy.GetState(0, G_RIGHT_TRIGGER_AXIS, ctr_y_change_up)) && ctr_y_change_up);
	bool move_down_changed =
		(G_PASS(kbmProxy.GetState(G_KEY_LEFTSHIFT, kbm_y_change_down)) && kbm_y_change_down) ||
		(G_PASS(controllerProxy.GetState(0, G_LEFT_TRIGGER_AXIS, ctr_y_change_down)) && ctr_y_change_down);

	if (move_up_changed || move_down_changed)
	{
		float total_y_change = kbm_y_change_up - kbm_y_change_down + ctr_y_change_up - ctr_y_change_down;
		worldCamera.row4.y += total_y_change * camera_speed * deltaTime;
	}

	bool move_forward_changed =
		(G_PASS(kbmProxy.GetState(G_KEY_W, kbm_z_change_forward)) && kbm_z_change_forward) ||
		(G_PASS(controllerProxy.GetState(0, G_LY_AXIS, ctr_z_change)) && ctr_z_change);
	bool move_backward_changed =
		(G_PASS(kbmProxy.GetState(G_KEY_S, kbm_z_change_backward)) && kbm_z_change_backward) ||
		(G_PASS(controllerProxy.GetState(0, G_LY_AXIS, ctr_z_change)) && ctr_z_change);
	bool move_left_changed =
		(G_PASS(kbmProxy.GetState(G_KEY_A, kbm_x_change_left)) && kbm_x_change_left) ||
		(G_PASS(controllerProxy.GetState(0, G_LX_AXIS, ctr_x_change)) && ctr_x_change);
	bool move_right_changed =
		(G_PASS(kbmProxy.GetState(G_KEY_D, kbm_x_change_right)) && kbm_x_change_right) ||
		(G_PASS(controllerProxy.GetState(0, G_LX_AXIS, ctr_x_change)) && ctr_x_change);

	if (move_forward_changed || move_backward_changed || move_left_changed || move_right_changed)
	{
		float total_x_change = kbm_x_change_right - kbm_x_change_left + ctr_x_change;
		float total_z_change = kbm_z_change_forward - kbm_z_change_backward + ctr_z_change;
		GW::MATH::GVECTORF translation = { total_x_change * per_frame_speed, 0.0f, total_z_change * per_frame_speed, 1.0f };
		matrixProxy.TranslateLocalF(worldCamera, translation, worldCamera);
	}

	float thumb_speed = G_PI * deltaTime * 0.05f;
	float aspect_ratio = 0.0f;
	unsigned int screen_width = 0;
	unsigned int screen_height = 0;

	win.GetWidth(screen_width);
	win.GetHeight(screen_height);
	vlk.GetAspectRatio(aspect_ratio);

	float mouse_x_delta = 0.0f;
	float mouse_y_delta = 0.0f;

	GW::GReturn result = kbmProxy.GetMouseDelta(mouse_x_delta, mouse_y_delta);
	bool mouse_r_button_pressed =
		(G_PASS(kbmProxy.GetState(G_BUTTON_RIGHT, mouse_r_button)) && mouse_r_button);
	bool mouse_moved = G_PASS(result) && result != GW::GReturn::REDUNDANT && mouse_r_button_pressed;

	if (!mouse_moved)
	{
		mouse_x_delta = 0.0f;
		mouse_y_delta = 0.0f;
	}

	float ctr_aim_x_change = 0.0f;
	float ctr_aim_y_change = 0.0f;

	bool aim_left_right_changed =
		(G_PASS(controllerProxy.GetState(0, G_RY_AXIS, ctr_aim_y_change)) && ctr_aim_y_change);
	bool aim_up_down_changed =
		(G_PASS(controllerProxy.GetState(0, G_RX_AXIS, ctr_aim_x_change)) && ctr_aim_x_change);

	if (mouse_moved || aim_left_right_changed)
	{
		float total_pitch = G_DEGREE_TO_RADIAN(65.0f) * mouse_y_delta / screen_height + ctr_aim_y_change * thumb_speed;
		GW::MATH::GMATRIXF x_rotation = GW::MATH::GIdentityMatrixF;
		matrixProxy.RotateXLocalF(x_rotation, total_pitch, x_rotation);
		matrixProxy.MultiplyMatrixF(x_rotation, worldCamera, worldCamera);
	}
	if (mouse_moved || aim_up_down_changed)
	{
		float total_yaw = G_DEGREE_TO_RADIAN(65.0f) * aspect_ratio * mouse_x_delta / screen_width + ctr_aim_x_change * thumb_speed;
		GW::MATH::GMATRIXF y_rotation = GW::MATH::GIdentityMatrixF;
		matrixProxy.RotateYLocalF(y_rotation, total_yaw, y_rotation);
		GW::MATH::GVECTORF position = worldCamera.row4;
		matrixProxy.MultiplyMatrixF(worldCamera, y_rotation, worldCamera);
		worldCamera.row4 = position;
	}
	matrixProxy.InverseF(worldCamera, modelData->view);

	float aspectRatio = 1.0f;
	GW::GReturn gr = vlk.GetAspectRatio(aspectRatio);
	gr = matrixProxy.ProjectionVulkanLHF(G2D_DEGREE_TO_RADIAN(65),
		aspectRatio, NEAR_PLANE, FAR_PLANE,
		modelData->projection);
}

bool Renderer::OpenFileDialogBox(GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE windowHandle, std::string& fileName)
{
	bool result = false;
	OPENFILENAME ofn = { 0 };       // common dialog box structure
	WCHAR szFile[260];       // buffer for file name 
	//HWND hwnd;              // owner window

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = (HWND)windowHandle.window;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileNameW(&ofn) == TRUE)
	{
		std::wstring ws(ofn.lpstrFile);
		fileName = std::string(ws.begin(), ws.end());
		result = true;
	}
	return result;
}

bool Renderer::LoadTexture(std::string texturePath, ktxVulkanTexture& texture, VkImageView& textureView)
{
	// Gateware, access to underlying Vulkan queue and command pool & physical device
	VkQueue graphicsQueue;
	VkCommandPool cmdPool;
	VkPhysicalDevice physicalDevice;
	vlk.GetGraphicsQueue((void**)&graphicsQueue);
	vlk.GetCommandPool((void**)&cmdPool);
	vlk.GetPhysicalDevice((void**)&physicalDevice);
	// libktx, temporary variables
	ktxTexture* kTexture;
	KTX_error_code ktxresult;
	ktxVulkanDeviceInfo vdi;
	// used to transfer texture CPU memory to GPU. just need one
	ktxresult = ktxVulkanDeviceInfo_Construct(&vdi, physicalDevice, device,
		graphicsQueue, cmdPool, nullptr);
	if (ktxresult != KTX_error_code::KTX_SUCCESS)
		return false;
	// load texture into CPU memory from file
	ktxresult = ktxTexture_CreateFromNamedFile(texturePath.c_str(),
		KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture);
	if (ktxresult != KTX_error_code::KTX_SUCCESS)
		return false;
	// This gets mad if you don't encode/save the .ktx file in a format Vulkan likes
	ktxresult = ktxTexture_VkUploadEx(kTexture, &vdi, &texture,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	if (ktxresult != KTX_error_code::KTX_SUCCESS)
		return false;
	// after loading all textures you don't need these anymore
	ktxTexture_Destroy(kTexture);
	ktxVulkanDeviceInfo_Destruct(&vdi);

	// Create image view.
	// Textures are not directly accessed by the shaders and are abstracted
	// by image views containing additional information and sub resource ranges.
	VkImageViewCreateInfo viewInfo = {};
	// Set the non-default values.
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.flags = 0;
	viewInfo.components = {
		VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
	};
	viewInfo.image = texture.image;
	viewInfo.format = texture.imageFormat;
	viewInfo.viewType = texture.viewType;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.layerCount = texture.layerCount;
	viewInfo.subresourceRange.levelCount = texture.levelCount;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.pNext = nullptr;
	VkResult vr = vkCreateImageView(device, &viewInfo, nullptr, &textureView);
	if (vr != VkResult::VK_SUCCESS)
		return false;

	return true;
}