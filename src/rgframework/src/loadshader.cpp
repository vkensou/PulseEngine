#include "cgpu_device.h"

HGEGraphics::Shader* oval_create_shader(oval_device_t* device, const std::string& vertPath, const std::string& fragPath, const CGPUBlendStateDescriptor& blend_desc, const CGPUDepthStateDescriptor& depth_desc, const CGPURasterizerStateDescriptor& rasterizer_state)
{
	auto vertShaderCode = readfile((const char*)vertPath.c_str());
	auto fragShaderCode = readfile((const char*)fragPath.c_str());
	return oval_create_shader(device, reinterpret_cast<const uint8_t*>(vertShaderCode.data()), (uint32_t)vertShaderCode.size(), reinterpret_cast<const uint8_t*>(fragShaderCode.data()), (uint32_t)fragShaderCode.size(),
		blend_desc, depth_desc, rasterizer_state);
}

HGEGraphics::Shader* oval_create_shader(oval_device_t* device, const uint8_t* vert_data, uint32_t vert_length, const uint8_t* frag_data, uint32_t frag_length, const CGPUBlendStateDescriptor& blend_desc, const CGPUDepthStateDescriptor& depth_desc, const CGPURasterizerStateDescriptor& rasterizer_state)
{
	auto D = (oval_cgpu_device_t*)device;
	auto shader = HGEGraphics::create_shader(D->device, vert_data, vert_length, frag_data, frag_length,
		blend_desc, depth_desc, rasterizer_state);
	auto ptr = shader.get();
	D->shaders.push_back(std::move(shader));
	return ptr;
}

void oval_free_shader(oval_device_t* device, HGEGraphics::Shader* shader)
{
	//HGEGraphics::free_shader(shader);
}

HGEGraphics::ComputeShader* oval_create_compute_shader(oval_device_t* device, const std::string& compPath)
{
	auto D = (oval_cgpu_device_t*)device;
	auto compShaderCode = readfile((const char*)compPath.c_str());
	auto computeShader = HGEGraphics::create_compute_shader(D->device, reinterpret_cast<const uint8_t*>(compShaderCode.data()), compShaderCode.size());
	auto ptr = computeShader.get();
	D->computeShaders.push_back(std::move(computeShader));
	return ptr;
}

void oval_free_compute_shader(oval_device_t* device, HGEGraphics::ComputeShader* shader)
{
	//HGEGraphics::free_compute_shader(shader);
}
