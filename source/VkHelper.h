#pragma once
#pragma warning(push)
#pragma warning(disable : 26812) // disabling a warning when including a header works normally for most warnings.

#include <vulkan\vulkan.h>

#pragma warning(pop)

#define VK_DESTROY_BUFFER(d, p) { if(d && p) { vkDestroyBuffer(d, p, nullptr); p = nullptr; }}
#define VK_FREE_MEMORY(d, p) { if(d && p) { vkFreeMemory(d, p, nullptr); p = nullptr; }}
#define VK_DESTROY_DESCRIPTOR_SET_LAYOUT(d, p) { if(d && p) { vkDestroyDescriptorSetLayout(d, p, nullptr); p = nullptr; }}
#define VK_DESTROY_DESCRIPTOR_POOL(d, p) { if(d && p) { vkDestroyDescriptorPool(d, p, nullptr); p = nullptr; }}
#define VK_DESTROY_SHADER(d, p) { if (d && p) { vkDestroyShaderModule(d, p, nullptr); p = nullptr; }}
#define VK_DESTROY_PIPELINE(d, p) { if(d && p) { vkDestroyPipeline(d, p, nullptr); p = nullptr; }}
#define VK_DESTROY_PIPELINE_LAYOUT(d, p) { if(d && p) { vkDestroyPipelineLayout(d, p, nullptr); p = nullptr; }}
#define VK_DESTROY_SAMPLER(d, p) {if(d && p) { vkDestroySampler(d, p, nullptr); p = nullptr;}}
#define VK_DESTROY_IMAGE_VIEW(d, p) { if(d && p) { vkDestroyImageView(d, p, nullptr); p = nullptr; }}

struct CVkPipelineShaderStageCreateInfo : public VkPipelineShaderStageCreateInfo
{
	CVkPipelineShaderStageCreateInfo() = default;

	explicit CVkPipelineShaderStageCreateInfo(const VkPipelineShaderStageCreateInfo& o) noexcept :
		VkPipelineShaderStageCreateInfo(o)
	{

	}

	explicit CVkPipelineShaderStageCreateInfo(const char* name,
		VkShaderModule shaderModule,
		VkShaderStageFlagBits shaderStage) noexcept
	{
		this->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		this->pNext = VK_NULL_HANDLE;
		this->flags = 0;
		this->stage = shaderStage;
		this->module = shaderModule;
		this->pName = name;
		this->pSpecializationInfo = VK_NULL_HANDLE;
	}
};

struct CVkPipelineInputAssemblyStateCreateInfo : public VkPipelineInputAssemblyStateCreateInfo
{
	CVkPipelineInputAssemblyStateCreateInfo() = default;

	explicit CVkPipelineInputAssemblyStateCreateInfo(const VkPipelineInputAssemblyStateCreateInfo& o) noexcept :
		VkPipelineInputAssemblyStateCreateInfo(o)
	{

	}

	explicit CVkPipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology,
		VkBool32 primitiveRestart = false) noexcept
	{

		this->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		this->pNext = VK_NULL_HANDLE;
		this->flags = 0;
		this->topology = topology;
		this->primitiveRestartEnable = primitiveRestart;
	}

};

struct CVkVertexInputBindingDescription : public VkVertexInputBindingDescription
{
	CVkVertexInputBindingDescription() = default;

	explicit CVkVertexInputBindingDescription(const VkVertexInputBindingDescription& o) noexcept :
		VkVertexInputBindingDescription(o)
	{

	}

	explicit CVkVertexInputBindingDescription(uint32_t binding,
		uint32_t stride,
		VkVertexInputRate inputRate) noexcept
	{
		this->binding = binding;
		this->stride = stride;
		this->inputRate = inputRate;
	}
};

struct CVkVertexInputAttributeDescription : public VkVertexInputAttributeDescription
{
	CVkVertexInputAttributeDescription() = default;

	explicit CVkVertexInputAttributeDescription(const VkVertexInputAttributeDescription& o) noexcept :
		VkVertexInputAttributeDescription(o)
	{

	}

	explicit CVkVertexInputAttributeDescription(uint32_t location,
		uint32_t binding,
		VkFormat format,
		uint32_t offset) noexcept
	{
		this->location = location;
		this->binding = binding;
		this->format = format;
		this->offset = offset;
	}
};

struct CVkPipelineVertexInputStateCreateInfo : public VkPipelineVertexInputStateCreateInfo
{
	CVkPipelineVertexInputStateCreateInfo() = default;

	explicit CVkPipelineVertexInputStateCreateInfo(const VkPipelineVertexInputStateCreateInfo& o) noexcept :
		VkPipelineVertexInputStateCreateInfo(o)
	{

	}

	explicit CVkPipelineVertexInputStateCreateInfo(uint32_t bindingDescCount,
		VkVertexInputBindingDescription* bindingDesc,
		uint32_t attributeDescCount,
		VkVertexInputAttributeDescription* attributeDesc) noexcept
	{
		this->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		this->pNext = VK_NULL_HANDLE;
		this->flags = 0;
		this->vertexBindingDescriptionCount = bindingDescCount;
		this->pVertexBindingDescriptions = bindingDesc;
		this->vertexAttributeDescriptionCount = attributeDescCount;
		this->pVertexAttributeDescriptions = attributeDesc;
	}
};

struct CVkViewport : public VkViewport
{
	CVkViewport() = default;

	explicit CVkViewport(const VkViewport& o) noexcept : 
		VkViewport(o)
	{

	}

	explicit CVkViewport(
		float x, float y,
		float width, float height,
		float minDepth, float maxDepth) noexcept
	{
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
		this->minDepth = minDepth;
		this->maxDepth = maxDepth;
	}
};

struct CVkRect2D : public VkRect2D
{
	CVkRect2D() = default;

	explicit CVkRect2D(const VkRect2D& o) noexcept :
		VkRect2D(o)
	{

	}

	explicit CVkRect2D(
		int32_t x, int32_t y, 
		uint32_t width, uint32_t height) noexcept
	{
		this->offset = { x, y };
		this->extent = { width, height };
	}
};

struct CVkPipelineViewportStateCreateInfo : public VkPipelineViewportStateCreateInfo
{
	CVkPipelineViewportStateCreateInfo() = default;

	explicit CVkPipelineViewportStateCreateInfo(const VkPipelineViewportStateCreateInfo& o) noexcept :
		VkPipelineViewportStateCreateInfo(o)
	{

	}

	explicit CVkPipelineViewportStateCreateInfo(
		uint32_t viewportCount,
		VkViewport* viewport,
		uint32_t scissorCount,
		VkRect2D* scissor) noexcept
	{
		this->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		this->pNext = VK_NULL_HANDLE;
		this->flags = 0;
		this->viewportCount = viewportCount;
		this->pViewports = viewport;
		this->scissorCount = scissorCount;
		this->pScissors = scissor;
	}
};

struct CVkPipelineRasterizationStateCreateInfo : public VkPipelineRasterizationStateCreateInfo
{
	CVkPipelineRasterizationStateCreateInfo() = default;

	explicit CVkPipelineRasterizationStateCreateInfo(const VkPipelineRasterizationStateCreateInfo& o) noexcept :
		VkPipelineRasterizationStateCreateInfo(o)
	{

	}

	explicit CVkPipelineRasterizationStateCreateInfo(
		VkPolygonMode polygonMode,
		VkCullModeFlagBits cullMode,
		VkFrontFace frontFace,
		float lineWidth = 1.0f) noexcept
	{
		this->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		this->pNext = VK_NULL_HANDLE;
		this->flags = 0;
		this->depthClampEnable = VK_FALSE;
		this->rasterizerDiscardEnable = VK_FALSE;
		this->polygonMode = polygonMode;
		this->cullMode = cullMode;
		this->frontFace = frontFace;
		this->depthBiasEnable = VK_FALSE;
		this->depthBiasConstantFactor = 0.0f;
		this->depthBiasClamp = 0.0f;
		this->depthBiasSlopeFactor = 0.0f;
		this->lineWidth = lineWidth;
	}
};

struct CVkPipelineMultisampleStateCreateInfo : public VkPipelineMultisampleStateCreateInfo
{
	CVkPipelineMultisampleStateCreateInfo() = default;

	explicit CVkPipelineMultisampleStateCreateInfo(const VkPipelineMultisampleStateCreateInfo& o) noexcept :
		VkPipelineMultisampleStateCreateInfo(o)
	{

	}

	explicit CVkPipelineMultisampleStateCreateInfo(
		VkSampleCountFlagBits samples) noexcept
	{
		this->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		this->pNext = VK_NULL_HANDLE;
		this->flags = 0;
		this->rasterizationSamples = samples;
		this->sampleShadingEnable = VK_FALSE;
		this->minSampleShading = 1.0f;
		this->pSampleMask = VK_NULL_HANDLE;
		this->alphaToCoverageEnable = VK_FALSE;
		this->alphaToOneEnable = VK_FALSE;
	}
};

struct CVkPipelineDepthStencilStateCreateInfo : public VkPipelineDepthStencilStateCreateInfo
{
	CVkPipelineDepthStencilStateCreateInfo() = default;

	explicit CVkPipelineDepthStencilStateCreateInfo(const VkPipelineDepthStencilStateCreateInfo& o) noexcept :
		VkPipelineDepthStencilStateCreateInfo(o)
	{

	}

	explicit CVkPipelineDepthStencilStateCreateInfo(
		VkBool32 depthTestEnable,
		VkBool32 depthWriteEnable,
		VkCompareOp depthCompareOp,
		VkBool32 depthBoundsTestEnable,
		float minDepth, float maxDepth,
		VkBool32 stencilTestEnable,
		VkStencilOpState frontStencilOp,
		VkStencilOpState backStencilOp) noexcept
	{
		this->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		this->pNext = VK_NULL_HANDLE;
		this->flags = 0;
		this->depthTestEnable = depthTestEnable;
		this->depthWriteEnable = depthWriteEnable;
		this->depthCompareOp = depthCompareOp;
		this->depthBoundsTestEnable = depthBoundsTestEnable;
		this->stencilTestEnable = stencilTestEnable;
		this->front = frontStencilOp;
		this->back = backStencilOp;
		this->minDepthBounds = minDepth;
		this->maxDepthBounds = maxDepth;
	}
};

struct CVkSamplerCreateInfo : public VkSamplerCreateInfo
{
	CVkSamplerCreateInfo() = default;

	explicit CVkSamplerCreateInfo(const VkSamplerCreateInfo& o) noexcept :
		VkSamplerCreateInfo(o) 
	{

	}

	explicit CVkSamplerCreateInfo(VkSamplerAddressMode mode, 
		VkFilter filter, 
		float maxMips) noexcept
	{
		this->sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		this->flags = 0;
		this->addressModeU = mode; // REPEAT IS COMMON
		this->addressModeV = mode;
		this->addressModeW = mode;
		this->magFilter = filter;
		this->minFilter = filter;
		this->mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		this->mipLodBias = 0;
		this->minLod = 0;
		this->maxLod = maxMips;	// fixed size of mips
		this->anisotropyEnable = false; // increases visual quality
		this->maxAnisotropy = 1.0;
		this->borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		this->compareEnable = VK_FALSE;
		this->compareOp = VK_COMPARE_OP_LESS;
		this->unnormalizedCoordinates = VK_FALSE;
		this->pNext = VK_NULL_HANDLE;
	}
};

struct CVkDescriptorSetLayoutCreateInfo : public VkDescriptorSetLayoutCreateInfo
{
	CVkDescriptorSetLayoutCreateInfo() = default;

	explicit CVkDescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutCreateInfo& o) noexcept :
		VkDescriptorSetLayoutCreateInfo(o)
	{

	}

	explicit CVkDescriptorSetLayoutCreateInfo(VkDescriptorSetLayoutBinding* binding,
		uint32_t bindingCount,
		VkDescriptorSetLayoutCreateFlags flags = 0) noexcept
	{
		this->sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		this->pNext = VK_NULL_HANDLE;
		this->flags = flags;
		this->bindingCount = bindingCount;
		this->pBindings = binding;		
	}
};

struct CVkDescriptorSetLayoutBinding : public VkDescriptorSetLayoutBinding
{
	CVkDescriptorSetLayoutBinding() = default;

	explicit CVkDescriptorSetLayoutBinding(const VkDescriptorSetLayoutBinding& o) noexcept :
		VkDescriptorSetLayoutBinding(o)
	{

	}

	explicit CVkDescriptorSetLayoutBinding(uint32_t binding, 
		uint32_t count,
		VkDescriptorType type,
		VkShaderStageFlags flags,
		VkSampler* samplers = VK_NULL_HANDLE) noexcept
	{
		this->binding = binding;
		this->descriptorCount = count;
		this->descriptorType = type;
		this->pImmutableSamplers = samplers;
		this->stageFlags = flags;
	}
};