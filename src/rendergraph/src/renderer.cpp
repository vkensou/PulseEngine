#include "renderer.h"

#include <vector>
#include "hash.h"
#include "rendergraph.h"
#include <bit>
#include "drawer.h"
#include "compare.h"

namespace HGEGraphics
{
	std::unique_ptr<Shader> create_shader(CGPUDeviceId device, const uint8_t* vert_data, uint32_t vert_length, const uint8_t* frag_data, uint32_t frag_length, const CGPUBlendStateDescriptor& blend_desc, const CGPUDepthStateDescriptor& depth_desc, const CGPURasterizerStateDescriptor& rasterizer_state)
	{
		CGPUShaderLibraryDescriptor vs_desc = {
			.name = "VertexShaderLibrary",
			.code_size = vert_length,
			.p_codes = vert_data,
			.stage = CGPU_SHADER_STAGE_VERTEX,
		};
		CGPUShaderLibraryDescriptor ps_desc = {
			.name = "FragmentShaderLibrary",
			.code_size = (uint32_t)frag_length,
			.p_codes = frag_data,
			.stage = CGPU_SHADER_STAGE_FRAGMENT,
		};
		CGPUShaderLibraryId vertex_shader = cgpu_device_create_shader_library(device, &vs_desc);
		CGPUShaderLibraryId fragment_shader = cgpu_device_create_shader_library(device, &ps_desc);
		CGPUShaderEntryDescriptor ppl_shaders[2];
		ppl_shaders[0].stage = CGPU_SHADER_STAGE_VERTEX;
		ppl_shaders[0].entry = "main";
		ppl_shaders[0].library = vertex_shader;
		ppl_shaders[1].stage = CGPU_SHADER_STAGE_FRAGMENT;
		ppl_shaders[1].entry = "main";
		ppl_shaders[1].library = fragment_shader;
		CGPURootSignatureDescriptor rs_desc = {
			.shader_count = 2,
			.p_shaders = ppl_shaders,
		};
		auto root_sig = cgpu_device_create_root_signature(device, &rs_desc);

		auto shader = new Shader();
		shader->root_sig = root_sig;
		shader->vs = ppl_shaders[0];
		shader->ps = ppl_shaders[1];
		shader->blend_desc = blend_desc;
		shader->blend_attachment_states.resize(blend_desc.attachment_count);
		std::copy(blend_desc.p_attachments, blend_desc.p_attachments + blend_desc.attachment_count, shader->blend_attachment_states.begin());
		shader->blend_desc.p_attachments = shader->blend_attachment_states.data();
		shader->depth_desc = depth_desc;
		shader->rasterizer_state = rasterizer_state;
		return std::unique_ptr<Shader>(shader);
	}

	Shader::~Shader()
	{
		cgpu_device_free_root_signature(root_sig->device, root_sig);
		cgpu_device_free_shader_library(vs.library->device, vs.library);
		cgpu_device_free_shader_library(ps.library->device, ps.library);
	}

	std::unique_ptr<ComputeShader> create_compute_shader(CGPUDeviceId device, const uint8_t* comp_data, uint32_t comp_length)
	{
		CGPUShaderLibraryDescriptor cs_desc = {
			.name = "ComputeShaderLibrary",
			.code_size = (uint32_t)comp_length,
			.p_codes = comp_data,
			.stage = CGPU_SHADER_STAGE_COMPUTE,
		};
		CGPUShaderLibraryId comp_shader = cgpu_device_create_shader_library(device, &cs_desc);

		CGPUShaderEntryDescriptor ppl_shaders[1];
		ppl_shaders[0].stage = CGPU_SHADER_STAGE_COMPUTE;
		ppl_shaders[0].entry = "main";
		ppl_shaders[0].library = comp_shader;
		CGPURootSignatureDescriptor rs_desc = {
			.shader_count = 1,
			.p_shaders = ppl_shaders,
		};
		auto root_sig = cgpu_device_create_root_signature(device, &rs_desc);

		auto shader = new ComputeShader();
		shader->root_sig = root_sig;
		shader->cs = ppl_shaders[0];
		return std::unique_ptr<ComputeShader>(shader);
	}

	ComputeShader::~ComputeShader()
	{
		cgpu_device_free_root_signature(root_sig->device, root_sig);
		cgpu_device_free_shader_library(cs.library->device, cs.library);
	}

	std::unique_ptr<Buffer> create_empty_buffer()
	{
		auto buffer = new Buffer();
		buffer->handle = CGPU_NULLPTR;
		buffer->type = CGPU_RESOURCE_TYPE_NONE;
		buffer->cur_state = CGPU_RESOURCE_STATE_UNDEFINED;
		buffer->dynamic_handle = {};
		return std::unique_ptr<Buffer>(buffer);
	}

	std::unique_ptr<Buffer> create_buffer(CGPUDeviceId device, const CGPUBufferDescriptor& desc)
	{
		auto buffer = create_empty_buffer();
		buffer->handle = cgpu_device_create_buffer(device, &desc);
		buffer->type = (ECGPUResourceTypeFlags)desc.descriptors;
		return buffer;
	}

	Buffer::~Buffer()
	{
		if (handle)
			cgpu_device_free_buffer(handle->device, handle);
	}

	std::unique_ptr<Mesh> create_empty_mesh()
	{
		auto mesh = new Mesh();
		mesh->vertex_layout = {};
		mesh->prim_topology = CGPU_PRIMITIVE_TOPOLOGY_POINT_LIST;
		mesh->vertices_count = 0;
		mesh->index_count = 0;
		mesh->index_stride = 0;
		mesh->vertex_buffer = nullptr;
		mesh->index_buffer = nullptr;
		mesh->prepared = false;
		return std::unique_ptr<Mesh>(mesh);
	}

	void init_mesh(Mesh* mesh, CGPUDeviceId device, uint32_t vertex_count, uint32_t index_count, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride, bool update_vertex_data_from_compute_shader, bool update_index_data_from_compute_shader)
	{
		mesh->vertex_layout = vertex_layout;
		mesh->vertex_attributes.resize(vertex_layout.attribute_count);
		std::copy(vertex_layout.p_attributes, vertex_layout.p_attributes + vertex_layout.attribute_count, mesh->vertex_attributes.begin());
		mesh->vertex_layout.p_attributes = mesh->vertex_attributes.data();
		mesh->prim_topology = prim_topology;
		mesh->vertices_count = vertex_count;
		mesh->index_count = index_count;
		for (auto i = 0; i < vertex_layout.attribute_count; ++i)
		{
			mesh->vertex_stride += vertex_layout.p_attributes[i].elem_stride;
		}
		mesh->index_stride = index_stride;

		CGPUBufferDescriptor vertex_buffer_desc = {};
		vertex_buffer_desc.name = "vertex buffer";
		vertex_buffer_desc.flags = CGPU_BUFFER_CREATION_USAGE_PERSISTENT_MAP;
		vertex_buffer_desc.descriptors = update_vertex_data_from_compute_shader ? CGPU_RESOURCE_TYPE_VERTEX_BUFFER | CGPU_RESOURCE_TYPE_RW_BUFFER : CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
		vertex_buffer_desc.memory_usage = CGPU_MEMORY_USAGE_GPU_ONLY;
		vertex_buffer_desc.size = vertex_count * mesh->vertex_stride;
		mesh->vertex_buffer = create_buffer(device, vertex_buffer_desc);

		if (index_count > 0)
		{
			CGPUBufferDescriptor index_buffer_desc = {};
			index_buffer_desc.name = "index buffer";
			index_buffer_desc.flags = CGPU_BUFFER_CREATION_USAGE_PERSISTENT_MAP;
			index_buffer_desc.descriptors = update_index_data_from_compute_shader ? CGPU_RESOURCE_TYPE_INDEX_BUFFER | CGPU_RESOURCE_TYPE_RW_BUFFER : CGPU_RESOURCE_TYPE_INDEX_BUFFER;
			index_buffer_desc.memory_usage = CGPU_MEMORY_USAGE_GPU_ONLY;
			index_buffer_desc.size = index_count * mesh->index_stride;
			mesh->index_buffer = create_buffer(device, index_buffer_desc);
		}
		mesh->prepared = false;
	}

	std::unique_ptr<Mesh> create_mesh(CGPUDeviceId device, uint32_t vertex_count, uint32_t index_count, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride, bool update_vertex_data_from_compute_shader, bool update_index_data_from_compute_shader)
	{
		auto mesh = create_empty_mesh();
		init_mesh(mesh.get(), device, vertex_count, index_count, prim_topology, vertex_layout, index_stride, update_vertex_data_from_compute_shader, update_index_data_from_compute_shader);
		return mesh;
	}

	std::unique_ptr<Mesh> create_dynamic_mesh(ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride)
	{
		auto mesh = create_empty_mesh();
		mesh->vertex_layout = vertex_layout;
		mesh->vertex_attributes.resize(vertex_layout.attribute_count);
		std::copy(vertex_layout.p_attributes, vertex_layout.p_attributes + vertex_layout.attribute_count, mesh->vertex_attributes.begin());
		mesh->vertex_layout.p_attributes = mesh->vertex_attributes.data();
		mesh->prim_topology = prim_topology;
		mesh->vertices_count = 0;
		mesh->vertex_stride = 0;
		for (auto i = 0; i < vertex_layout.attribute_count; ++i)
		{
			mesh->vertex_stride += vertex_layout.p_attributes[i].elem_stride;
		}
		mesh->index_stride = index_stride;
		mesh->vertex_buffer = create_empty_buffer();
		mesh->index_buffer = create_empty_buffer();
		mesh->prepared = true;
		return mesh;
	}

	buffer_handle_t declare_dynamic_vertex_buffer(Mesh* mesh, rendergraph_t* rg, uint32_t count)
	{
		auto dynamic_vertex_buffer = rendergraph_import_dynamic_buffer(rg, mesh->vertex_buffer.get());
		rg_buffer_set_size(rg, dynamic_vertex_buffer, count * mesh->vertex_stride);
		rg_buffer_set_type(rg, dynamic_vertex_buffer, CGPU_RESOURCE_TYPE_VERTEX_BUFFER);
		rg_buffer_set_usage(rg, dynamic_vertex_buffer, CGPU_MEMORY_USAGE_GPU_ONLY);
		mesh->vertex_buffer->dynamic_handle = dynamic_vertex_buffer;
		mesh->vertices_count = count;
		return mesh->vertex_buffer->dynamic_handle;
	}

	buffer_handle_t declare_dynamic_index_buffer(Mesh* mesh, rendergraph_t* rg, uint32_t count)
	{
		auto dynamic_index_buffer = rendergraph_import_dynamic_buffer(rg, mesh->index_buffer.get());
		rg_buffer_set_size(rg, dynamic_index_buffer, count * mesh->index_stride);
		rg_buffer_set_type(rg, dynamic_index_buffer, CGPU_RESOURCE_TYPE_INDEX_BUFFER);
		rg_buffer_set_usage(rg, dynamic_index_buffer, CGPU_MEMORY_USAGE_GPU_ONLY);
		mesh->index_buffer->dynamic_handle = dynamic_index_buffer;
		mesh->index_count = count;
		return mesh->index_buffer->dynamic_handle;
	}

	void dynamic_mesh_reset(Mesh* mesh)
	{
		mesh->vertices_count = 0;
		mesh->index_count = 0;
		mesh->vertex_buffer->dynamic_handle = {};
		mesh->index_buffer->dynamic_handle = {};
	}

	std::unique_ptr<Texture> create_empty_texture()
	{
		auto texture = new Texture();
		texture->handle = CGPU_NULLPTR;
		texture->view = CGPU_NULLPTR;
		texture->cur_states.clear();
		texture->states_consistent = false;
		texture->prepared = false;
		texture->dynamic_handle = {};
		return std::unique_ptr<Texture>(texture);
	}

	void init_texture(Texture* texture, CGPUDeviceId device, const CGPUTextureDescriptor& desc)
	{
		CGPUTextureDescriptor new_desc = desc;
		if (desc.depth == 1 && desc.height == 1)
			new_desc.flags |= CGPU_TEXTURE_CREATION_USAGE_FORCE2D;

		texture->handle = cgpu_device_create_texture(device, &new_desc);
		texture->cur_states.resize(new_desc.array_size * new_desc.mip_levels);
		std::fill(texture->cur_states.begin(), texture->cur_states.end(), CGPU_RESOURCE_STATE_UNDEFINED);
		texture->states_consistent = true;

		uint32_t arrayCount = texture->handle->info->array_size_minus_one + 1;
		ECGPUTextureDimension dims = CGPU_TEXTURE_DIMENSION_2D;
		if (CGPU_RESOURCE_TYPE_TEXTURE_CUBE == (new_desc.descriptors & CGPU_RESOURCE_TYPE_TEXTURE_CUBE))
			dims = CGPU_TEXTURE_DIMENSION_CUBE;
		else if (new_desc.depth > 1)
			dims = CGPU_TEXTURE_DIMENSION_3D;
		CGPUTextureViewDescriptor view_desc;
		view_desc.texture = texture->handle;
		view_desc.format = texture->handle->info->format;
		view_desc.usages = CGPU_TEXTURE_VIEW_USAGE_SRV;
		view_desc.aspects = CGPU_TEXTURE_VIEW_ASPECT_COLOR;
		view_desc.dims = dims;
		view_desc.base_array_layer = 0;
		view_desc.array_layer_count = arrayCount;
		view_desc.base_mip_level = 0;
		view_desc.mip_level_count = texture->handle->info->mip_levels;
		texture->view = cgpu_device_create_texture_view(device, &view_desc);
		texture->prepared = false;
	}

	std::unique_ptr<Texture> create_texture(CGPUDeviceId device, const CGPUTextureDescriptor& desc)
	{
		auto texture = create_empty_texture();
		init_texture(texture.get(), device, desc);
		return texture;
	}

	Texture::~Texture()
	{
		if (view)
			cgpu_device_free_texture_view(view->device, view);
		if (handle)
			cgpu_device_free_texture(handle->device, handle);
	}

	Material::Material(CGPUDeviceId device, Shader* shader)
		: device(device), shader(shader)
	{

	}

	Material::~Material()
	{
		textures.clear();
		samplers.clear();
		buffers.clear();
		shader = nullptr;
		device = nullptr;
	}

	void Material::bindTexture(int set, int bind, Texture* texture)
	{
		textures.emplace_back(set, bind, texture);
	}

	void Material::bindSampler(int set, int bind, CGPUSamplerId sampler)
	{
		samplers.emplace_back(set, bind, sampler);
	}

	void Material::bindBuffer(int set, int bind, size_t size, const void* data)
	{
		auto align_size = std::bit_ceil(size);
		auto desc = CGPUBufferDescriptor{
			.size = align_size,
			.name = "Material Data Buffer",
			.descriptors = CGPU_RESOURCE_TYPE_UNIFORM_BUFFER,
			.memory_usage = CGPU_MEMORY_USAGE_CPU_TO_GPU,
		};
		auto buffer = create_buffer(device, desc);
		cgpu_buffer_map(buffer->handle, nullptr);
		memcpy(buffer->handle->info->cpu_mapped_address, data, size);
		cgpu_buffer_unmap(buffer->handle);
		buffers.emplace_back(set, bind, buffer.get());
		ownedBuffers.push_back(std::move(buffer));
	}

	void init_backbuffer(Backbuffer* backbuffer, CGPUSwapChainId swapchain, int index)
	{
		backbuffer->texture.handle = swapchain->p_back_buffers[index];
		backbuffer->texture.view = CGPU_NULLPTR;
		backbuffer->texture.cur_states.resize(1);
		backbuffer->texture.cur_states[0] = CGPU_RESOURCE_STATE_UNDEFINED;
		backbuffer->texture.states_consistent = true;
		backbuffer->texture.dynamic_handle = {};
	}

	Backbuffer::~Backbuffer()
	{
		texture.handle = CGPU_NULLPTR;
		texture.view = CGPU_NULLPTR;
		texture.cur_states.clear();
		texture.states_consistent = false;
	}

	void set_viewport(RenderPassEncoder* encoder, float x, float y, float width, float height, float min_depth, float max_depth)
	{
		cgpu_render_pass_encoder_set_viewport(encoder->encoder, x, y, width, height, min_depth, max_depth);
	}

	void set_scissor(RenderPassEncoder* encoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		cgpu_render_pass_encoder_set_scissor(encoder->encoder, x, y, width, height);
	}

	void push_constants(RenderPassEncoder* encoder, Shader* shader, const char* name, const void* data)
	{
		cgpu_render_pass_encoder_push_constants(encoder->encoder, shader->root_sig, name, data);
	}

	void update_render_pipeline(RenderPassEncoder* encoder, Shader* shader, ECGPUPrimitiveTopology mesh_topology, const CGPUVertexLayout& vertex_layout)
	{
		auto pipeline = encoder->context->pipelinePool.getGraphicsPipeline(encoder, shader, mesh_topology, vertex_layout);
		if (pipeline && pipeline->handle != encoder->last_render_pipeline)
		{
			cgpu_render_pass_encoder_bind_render_pipeline(encoder->encoder, pipeline->handle);
			if (encoder->context->pipelinePool.dynamicStateT1Enabled())
			{
				cgpu_raster_state_encoder_set_cull_mode(encoder->raster_state_encoder, shader->rasterizer_state.cull_mode);
				cgpu_raster_state_encoder_set_front_face(encoder->raster_state_encoder, shader->rasterizer_state.front_face);
				cgpu_raster_state_encoder_set_primitive_topology(encoder->raster_state_encoder, mesh_topology);
				cgpu_raster_state_encoder_set_depth_test_enabled(encoder->raster_state_encoder, shader->depth_desc.depth_test);
				cgpu_raster_state_encoder_set_depth_write_enabled(encoder->raster_state_encoder, shader->depth_desc.depth_write);
				cgpu_raster_state_encoder_set_depth_compare_op(encoder->raster_state_encoder, shader->depth_desc.depth_op);
			}
			encoder->last_render_pipeline = pipeline->handle;
			memset(encoder->last_bind_resources, 0, sizeof(encoder->last_bind_resources));
		}
	}

	void update_descriptor_set(RenderPassEncoder* encoder, CGPURootSignatureId root_sig, bool is_graphics)
	{
		for (uint32_t i = 0; i < std::min(4u, root_sig->table_count); ++i)
		{
			auto& table = root_sig->p_tables[i];
			CGPUDescriptorSetDescriptor dset_desc =
			{
				.root_signature = root_sig,
				.set_index = table.set_index,
			};
			
			auto dset = encoder->context->descriptorSetPool.getDescriptorSet(dset_desc);
			encoder->context->allocated_dsets.push_back(dset);

			const uint32_t data_size = 64;
			CGPUDescriptorData datas[data_size] = { 0 };
			uint32_t data_count = 0;
			uint32_t texture_view_count = 0;
			uint32_t sampler_count = 0;
			uint32_t buffer_count = 0;
			uint32_t offset_size_count = 0;
			for (uint32_t j = 0; j < std::min(data_size, table.resources_count); ++j)
			{
				auto& res = table.p_resources[j];
				CGPUDescriptorData data =
				{
					.binding = res.binding,
					.binding_type = res.type,
					.count = 1,
				};
				if (res.type == CGPU_RESOURCE_TYPE_TEXTURE)
				{
					CGPUTextureViewId textureview = CGPU_NULLPTR;
					for (auto iter = encoder->context->global_texture_table.rbegin(); iter != encoder->context->global_texture_table.rend(); ++iter)
					{
						auto& binder = *iter;
						if (binder.set == i && binder.bind == res.binding)
						{
							if (rendergraph_texture_handle_valid(binder.texture_handle))
								textureview = rendergraph_resolve_texture_view(encoder, binder.texture_handle);
							else if (binder.texture && binder.texture->prepared)
								textureview = binder.texture->view;
							break;
						}
					}
					if (!textureview)
						textureview = encoder->context->default_texture;
					encoder->textureviews[texture_view_count] = textureview;
					data.resources.textures = encoder->textureviews + texture_view_count;
					++texture_view_count;
				}
				else if (res.type == CGPU_RESOURCE_TYPE_SAMPLER)
				{
					CGPUSamplerId sampler = CGPU_NULLPTR;
					for (auto iter = encoder->context->global_sampler_table.rbegin(); iter != encoder->context->global_sampler_table.rend(); ++iter)
					{
						auto& binder = *iter;
						if (binder.set == i && binder.bind == res.binding)
						{
							sampler = binder.sampler;
							break;
						}
					}
					if (!sampler)
						;	// TODO
					encoder->samplers[sampler_count] = sampler;
					data.resources.samplers = encoder->samplers + sampler_count;
					++sampler_count;
				}
				else if (res.type == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER || res.type == CGPU_RESOURCE_TYPE_RW_BUFFER)
				{
					for (auto iter = encoder->context->global_buffer_table.rbegin(); iter != encoder->context->global_buffer_table.rend(); ++iter)
					{
						auto& binder = *iter;
						if (binder.set == i && binder.bind == res.binding)
						{
							CGPUBufferId buffer;
							if (rendergraph_buffer_handle_valid(binder.buffer_handle))
								buffer = rendergraph_resolve_buffer(encoder, binder.buffer_handle);
							else
								buffer = binder.buffer->handle;
							encoder->buffers[buffer_count] = buffer;
							if (binder.offset != 0 || binder.size != 0)
							{
								encoder->buffer_offset_sizes[offset_size_count] = binder.offset;
								data.params.buffers_params.offsets = encoder->buffer_offset_sizes + (offset_size_count++);
								encoder->buffer_offset_sizes[offset_size_count] = binder.size;
								data.params.buffers_params.sizes = encoder->buffer_offset_sizes + (offset_size_count++);
							}
							data.resources.buffers = encoder->buffers + buffer_count;
							++buffer_count;
							break;
						}
					}
				}
				if (data.resources.ptrs != nullptr)
					datas[data_count++] = data;
			}

			if (data_count > 0)
			{
				bool dset_dirty = !compare(datas, encoder->last_bind_resources[i], data_count);
				bool offset_size_dirty = memcmp(encoder->buffer_offset_sizes, encoder->last_buffer_offset_sizes[i], sizeof(float) * 2 * offset_size_count);
				if (dset_dirty || offset_size_dirty)
				{
					cgpu_descriptor_set_update(dset->handle, data_count, datas);
					if (is_graphics)
						cgpu_render_pass_encoder_bind_descriptor_set(encoder->encoder, dset->handle);
					else
						cgpu_compute_pass_encoder_bind_descriptor_set(encoder->compute_encoder, dset->handle);
					memcpy(encoder->last_bind_resources[i], datas, sizeof(CGPUDescriptorData) * data_count);
					memcpy(encoder->last_buffer_offset_sizes[i], encoder->buffer_offset_sizes, sizeof(float) * 2 * offset_size_count);
				}
			}
		}
	}

	void update_material(RenderPassEncoder* encoder, Material* material)
	{
		for (auto& bind : material->buffers)
			set_global_buffer(encoder, bind.buffer, bind.set, bind.bind);
		for (auto& bind : material->textures)
			set_global_texture(encoder, bind.texture, bind.set, bind.bind);
		for (auto& bind : material->samplers)
			set_global_sampler(encoder, bind.sampler, bind.set, bind.bind);
	}

	void update_mesh(RenderPassEncoder* encoder, Mesh* mesh)
	{
		CGPUBufferId vertex_buffer = CGPU_NULLPTR;
		if (rendergraph_buffer_handle_valid(mesh->vertex_buffer->dynamic_handle))
		{
			auto vertex_buffer_handle = mesh->vertex_buffer->dynamic_handle;
			vertex_buffer = rendergraph_resolve_buffer(encoder, vertex_buffer_handle);
		}
		else if (mesh->vertex_buffer)
		{
			vertex_buffer = mesh->vertex_buffer->handle;
		}
		const uint32_t vert_stride = mesh->vertex_stride;
		if (encoder->last_vertex_buffer != vertex_buffer || encoder->last_vertex_buffer_stride != vert_stride)
		{
			if (vertex_buffer)
				cgpu_render_pass_encoder_bind_vertex_buffers(encoder->encoder, 1, &vertex_buffer, &vert_stride, nullptr);
			encoder->last_vertex_buffer = vertex_buffer;
			encoder->last_vertex_buffer_stride = vert_stride;
		}

		CGPUBufferId index_buffer = CGPU_NULLPTR;
		if (mesh->index_buffer)
		{
			if (rendergraph_buffer_handle_valid(mesh->index_buffer->dynamic_handle))
			{
				auto index_buffer_handle = mesh->index_buffer->dynamic_handle;
				index_buffer = rendergraph_resolve_buffer(encoder, index_buffer_handle);
			}
			else
			{
				index_buffer = mesh->index_buffer->handle;
			}
		}
		const uint32_t index_stride = mesh->index_stride;
		if (encoder->last_index_buffer != index_buffer || encoder->last_index_buffer_stride != index_stride)
		{
			if (index_buffer)
				cgpu_render_pass_encoder_bind_index_buffer(encoder->encoder, index_buffer, index_stride, 0);
			encoder->last_index_buffer = index_buffer;
			encoder->last_index_buffer_stride = index_stride;
		}
	}

	void draw(RenderPassEncoder* encoder, Shader* shader, Mesh* mesh)
	{
		if (!mesh->prepared)
			return;
		update_render_pipeline(encoder, shader, mesh->prim_topology, mesh->vertex_layout);
		update_descriptor_set(encoder, shader->root_sig, true);
		update_mesh(encoder, mesh);
		if (encoder->last_index_buffer)
			cgpu_render_pass_encoder_draw_indexed(encoder->encoder, mesh->index_count, 0, 0);
		else
			cgpu_render_pass_encoder_draw(encoder->encoder, mesh->vertices_count, 0);
	}

	void draw_submesh(RenderPassEncoder* encoder, Shader* shader, Mesh* mesh, uint32_t index_count, uint32_t first_index, uint32_t vertex_count, uint32_t first_vertex)
	{
		if (!mesh->prepared)
			return;
		update_render_pipeline(encoder, shader, mesh->prim_topology, mesh->vertex_layout);
		update_descriptor_set(encoder, shader->root_sig, true);
		update_mesh(encoder, mesh);
		if (encoder->last_index_buffer)
			cgpu_render_pass_encoder_draw_indexed(encoder->encoder, index_count, first_index, first_vertex);
		else
			cgpu_render_pass_encoder_draw(encoder->encoder, vertex_count, first_vertex);
	}

	static CGPUVertexLayout procedure_vertex_layout = { .attribute_count = 0 };
	void draw_procedure(RenderPassEncoder* encoder, Shader* shader, ECGPUPrimitiveTopology mesh_topology, uint32_t vertex_count)
	{
		update_render_pipeline(encoder, shader, mesh_topology, procedure_vertex_layout);
		update_descriptor_set(encoder, shader->root_sig, true);
		cgpu_render_pass_encoder_draw(encoder->encoder, vertex_count, 0);
	}

	void draw(RenderPassEncoder* encoder, Material* material, Mesh* mesh)
	{
		if (!mesh->prepared || !material)
			return;
		update_material(encoder, material);
		auto shader = material->shader;
		update_render_pipeline(encoder, shader, mesh->prim_topology, mesh->vertex_layout);
		update_descriptor_set(encoder, shader->root_sig, true);
		update_mesh(encoder, mesh);
		if (encoder->last_index_buffer)
			cgpu_render_pass_encoder_draw_indexed(encoder->encoder, mesh->index_count, 0, 0);
		else
			cgpu_render_pass_encoder_draw(encoder->encoder, mesh->vertices_count, 0);
	}

	void draw_submesh(RenderPassEncoder* encoder, Material* material, Mesh* mesh, uint32_t index_count, uint32_t first_index, uint32_t vertex_count, uint32_t first_vertex)
	{
		if (!mesh->prepared)
			return;
		update_material(encoder, material);
		auto shader = material->shader;
		update_render_pipeline(encoder, shader, mesh->prim_topology, mesh->vertex_layout);
		update_descriptor_set(encoder, shader->root_sig, true);
		update_mesh(encoder, mesh);
		if (encoder->last_index_buffer)
			cgpu_render_pass_encoder_draw_indexed(encoder->encoder, index_count, first_index, first_vertex);
		else
			cgpu_render_pass_encoder_draw(encoder->encoder, vertex_count, first_vertex);
	}

	void draw_procedure(RenderPassEncoder* encoder, Material* material, ECGPUPrimitiveTopology mesh_topology, uint32_t vertex_count)
	{
		update_material(encoder, material);
		auto shader = material->shader;
		update_render_pipeline(encoder, shader, mesh_topology, procedure_vertex_layout);
		update_descriptor_set(encoder, shader->root_sig, true);
		cgpu_render_pass_encoder_draw(encoder->encoder, vertex_count, 0);
	}

	void update_compute_pipeline(RenderPassEncoder* encoder, ComputeShader* shader)
	{
		auto pipeline = encoder->context->computePipelinePool.getComputePipeline(shader);
		if (pipeline && pipeline->handle != encoder->last_compute_pipeline)
		{
			cgpu_compute_pass_encoder_bind_compute_pipeline(encoder->compute_encoder, pipeline->handle);
			encoder->last_compute_pipeline = pipeline->handle;
			memset(encoder->last_bind_resources, 0, sizeof(encoder->last_bind_resources));
		}
	}

	void dispatch(RenderPassEncoder* encoder, ComputeShader* shader, uint32_t thread_x, uint32_t thread_y, uint32_t thread_z)
	{
		update_compute_pipeline(encoder, shader);
		update_descriptor_set(encoder, shader->root_sig, false);
		cgpu_compute_pass_encoder_dispatch(encoder->compute_encoder, thread_x, thread_y, thread_z);
	}

	void set_global_texture(RenderPassEncoder* encoder, Texture* texture, int set, int slot)
	{
		encoder->context->global_texture_table.push_back({ texture, {}, set, slot });
	}

	void set_global_texture_handle(RenderPassEncoder* encoder, texture_handle_t texture, int set, int slot)
	{
		encoder->context->global_texture_table.push_back({ nullptr, texture, set, slot });
	}

	void set_global_sampler(RenderPassEncoder* encoder, CGPUSamplerId sampler, int set, int slot)
	{
		encoder->context->global_sampler_table.push_back({ sampler, set, slot });
	}

	void set_global_buffer(RenderPassEncoder* encoder, Buffer* buffer, int set, int slot)
	{
		encoder->context->global_buffer_table.push_back({ buffer, {}, set, slot, 0, 0 });
	}

	void set_global_dynamic_buffer(RenderPassEncoder* encoder, buffer_handle_t buffer, int set, int slot)
	{
		encoder->context->global_buffer_table.push_back({ nullptr, buffer, set, slot, 0, 0 });
	}

	void set_global_buffer_with_offset_size(RenderPassEncoder* encoder, buffer_handle_t buffer, int set, int slot, uint64_t offset, uint64_t size)
	{
		encoder->context->global_buffer_table.push_back({ nullptr, buffer, set, slot, offset, size });
	}

	void upload(UploadEncoder* encoder, uint64_t offset, uint64_t length, void* data)
	{
		char* address = (char*)encoder->address + offset;
		memcpy(address, data, length);
	}

	ExecutorContext::ExecutorContext(CGPUDeviceId device, CGPUQueueId gfx_queue, bool profile, std::pmr::memory_resource* memory_resource)
		: device(device), memory_resource(memory_resource), renderPassPool(device, memory_resource), framebufferPool(device, memory_resource), texturePool(device, gfx_queue, nullptr, memory_resource), pipelinePool(device, nullptr, memory_resource), computePipelinePool(device, nullptr, memory_resource), textureViewPool(nullptr, memory_resource), bufferPool(device, nullptr, memory_resource), descriptorSetPool(device, memory_resource), allocated_dsets(memory_resource)
		, cmds(memory_resource), allocated_cmds(memory_resource), global_texture_table(memory_resource), global_sampler_table(memory_resource), global_buffer_table(memory_resource)
	{
		cmdPool = cgpu_queue_create_command_pool(gfx_queue, CGPU_NULLPTR);
		if (profile)
			profiler = new Profiler(device, gfx_queue, memory_resource);
		auto adapter_detail = cgpu_adapter_query_adapter_detail(device->adapter);
		support_shading_rate = adapter_detail->support_shading_rate;
	}

	void ExecutorContext::newFrame()
	{
		++timestamp;

		cgpu_command_pool_reset(cmdPool);

		for (auto cmd : allocated_cmds)
			cmds.push_back(cmd);
		allocated_cmds.clear();

		global_texture_table.clear();
		global_sampler_table.clear();
		global_buffer_table.clear();

		framebufferPool.newFrame();
		descriptorSetPool.newFrame();
		textureViewPool.newFrame();
		bufferPool.newFrame();
		pipelinePool.newFrame();
		computePipelinePool.newFrame();
		renderPassPool.newFrame();
		texturePool.newFrame();

		for (auto& dset : allocated_dsets)
			descriptorSetPool.releaseResource(dset);
		allocated_dsets.clear();
	}

	CGPUCommandBufferId ExecutorContext::requestCmd()
	{
		CGPUCommandBufferId cmd;
		if (!cmds.empty())
		{
			cmd = cmds.back();
			cmds.pop_back();
		}
		else
		{
			CGPUCommandBufferDescriptor cmd_desc = { .is_secondary = false };
			cmd = cgpu_command_pool_create_command_buffer(cmdPool, &cmd_desc);
		}

		allocated_cmds.push_back(cmd);
		return cmd;
	}

	void ExecutorContext::destroy()
	{
		delete profiler;
		profiler = nullptr;
		framebufferPool.destroy();
		textureViewPool.destroy();
		pipelinePool.destroy();
		computePipelinePool.destroy();
		renderPassPool.destroy();
		texturePool.destroy();
		bufferPool.destroy();
		for (auto cmd : cmds)
		{
			cgpu_command_pool_free_command_buffer(cmdPool, cmd);
		}
		cmds.clear();
		for (auto cmd : allocated_cmds)
		{
			cgpu_command_pool_free_command_buffer(cmdPool, cmd);
		}
		allocated_cmds.clear();
		if (cmdPool)
			cgpu_queue_free_command_pool(cmdPool->queue, cmdPool);
		cmdPool = CGPU_NULLPTR;
		global_texture_table.clear();
		global_sampler_table.clear();
		global_buffer_table.clear();
		device = CGPU_NULLPTR;
	}
	void ExecutorContext::pre_destroy()
	{
		for (auto& dset : allocated_dsets)
		{
			descriptorSetPool.releaseResource(dset);
		}
		allocated_dsets.clear();
		descriptorSetPool.destroy();
	}
}