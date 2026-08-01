#include "renderer.h"

#include <vector>
#include "hash.h"
#include "rendergraph_compiler_internal.h"
#include <bit>
#include "drawer.h"
#include "compare.h"

namespace HGEGraphics
{
	std::unique_ptr<pulse_shader_data_t> create_shader(CGPUDeviceId device, const uint8_t* vert_data, uint32_t vert_length, const uint8_t* frag_data, uint32_t frag_length, const CGPUBlendStateDescriptor& blend_desc, const CGPUDepthStateDescriptor& depth_desc, const CGPURasterizerStateDescriptor& rasterizer_state)
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
		return create_shader_from_libraries(device, vertex_shader, fragment_shader, blend_desc, depth_desc, rasterizer_state);
	}

	std::unique_ptr<pulse_shader_data_t> create_shader_from_libraries(
		CGPUDeviceId device,
		CGPUShaderLibraryId vs_library,
		CGPUShaderLibraryId ps_library,
		const CGPUBlendStateDescriptor& blend_desc,
		const CGPUDepthStateDescriptor& depth_desc,
		const CGPURasterizerStateDescriptor& rasterizer_state)
	{
		CGPUShaderEntryDescriptor ppl_shaders[2];
		ppl_shaders[0].stage = CGPU_SHADER_STAGE_VERTEX;
		ppl_shaders[0].entry = "main";
		ppl_shaders[0].library = vs_library;
		ppl_shaders[1].stage = CGPU_SHADER_STAGE_FRAGMENT;
		ppl_shaders[1].entry = "main";
		ppl_shaders[1].library = ps_library;
		CGPURootSignatureDescriptor rs_desc = {
			.shader_count = 2,
			.p_shaders = ppl_shaders,
		};
		auto root_sig = cgpu_device_create_root_signature(device, &rs_desc);

		auto shader = new pulse_shader_data_t();
		shader->root_sig = root_sig;
		shader->vs = ppl_shaders[0];
		shader->ps = ppl_shaders[1];
		shader->blend_desc = blend_desc;
		if (blend_desc.attachment_count > 0 && blend_desc.p_attachments)
		{
			shader->blend_attachment_states_count = blend_desc.attachment_count;
			shader->p_blend_attachment_states = new CGPUBlendAttachmentState[shader->blend_attachment_states_count];
			std::copy(blend_desc.p_attachments, blend_desc.p_attachments + blend_desc.attachment_count, shader->p_blend_attachment_states);
		}
		else
		{
			shader->blend_attachment_states_count = 0;
			shader->p_blend_attachment_states = nullptr;
		}
		shader->blend_desc.p_attachments = shader->p_blend_attachment_states;
		shader->depth_desc = depth_desc;
		shader->rasterizer_state = rasterizer_state;
		return std::unique_ptr<pulse_shader_data_t>(shader);
	}

	void free_shader(pulse_shader_data_t* shader)
	{
		if (shader->p_blend_attachment_states)
		{
			delete[] shader->p_blend_attachment_states;
		}
		shader->p_blend_attachment_states = nullptr;
		shader->blend_attachment_states_count = 0;

		if (shader->p_properties)
		{
			for (uint32_t i = 0; i < shader->property_count; ++i)
				delete[] shader->p_properties[i].name;
			delete[] shader->p_properties;
		}
		shader->p_properties = nullptr;
		shader->property_count = 0;

		if (shader->p_ubo_infos)
		{
			delete[] shader->p_ubo_infos;
		}
		shader->p_ubo_infos = nullptr;
		shader->ubo_info_count = 0;

		if (shader->p_set_infos)
		{
			delete[] shader->p_set_infos;
		}
		shader->p_set_infos = nullptr;
		shader->set_info_count = 0;

		if (shader->root_sig)
			cgpu_device_free_root_signature(shader->root_sig->device, shader->root_sig);
		if (shader->vs.library)
			cgpu_device_free_shader_library(shader->vs.library->device, shader->vs.library);
		if (shader->ps.library)
			cgpu_device_free_shader_library(shader->ps.library->device, shader->ps.library);
	}

	std::unique_ptr<pulse_compute_shader_data_t> create_compute_shader(CGPUDeviceId device, const uint8_t* comp_data, uint32_t comp_length)
	{
		CGPUShaderLibraryDescriptor cs_desc = {
			.name = "ComputeShaderLibrary",
			.code_size = (uint32_t)comp_length,
			.p_codes = comp_data,
			.stage = CGPU_SHADER_STAGE_COMPUTE,
		};
		CGPUShaderLibraryId comp_shader = cgpu_device_create_shader_library(device, &cs_desc);
		return create_compute_shader_from_library(device, comp_shader);
	}

	std::unique_ptr<pulse_compute_shader_data_t> create_compute_shader_from_library(
		CGPUDeviceId device,
		CGPUShaderLibraryId cs_library)
	{
		CGPUShaderEntryDescriptor ppl_shaders[1];
		ppl_shaders[0].stage = CGPU_SHADER_STAGE_COMPUTE;
		ppl_shaders[0].entry = "main";
		ppl_shaders[0].library = cs_library;
		CGPURootSignatureDescriptor rs_desc = {
			.shader_count = 1,
			.p_shaders = ppl_shaders,
		};
		auto root_sig = cgpu_device_create_root_signature(device, &rs_desc);

		auto shader = new pulse_compute_shader_data_t();
		shader->root_sig = root_sig;
		shader->cs = ppl_shaders[0];
		return std::unique_ptr<pulse_compute_shader_data_t>(shader);
	}

	void free_compute_shader(pulse_compute_shader_data_t* compute_shader)
	{
		if (compute_shader->root_sig)
			cgpu_device_free_root_signature(compute_shader->root_sig->device, compute_shader->root_sig);
		if (compute_shader->cs.library)
			cgpu_device_free_shader_library(compute_shader->cs.library->device, compute_shader->cs.library);
	}

	pulse_buffer_data_t* create_empty_buffer()
	{
		auto buffer = new pulse_buffer_data_t();
		buffer->handle = CGPU_NULLPTR;
		buffer->type = CGPU_RESOURCE_TYPE_NONE;
		buffer->cur_state = CGPU_RESOURCE_STATE_UNDEFINED;
		buffer->dynamic_handle = {};
		return buffer;
	}

	pulse_buffer_data_t* create_buffer(CGPUDeviceId device, const CGPUBufferDescriptor& desc)
	{
		auto buffer = create_empty_buffer();
		buffer->handle = cgpu_device_create_buffer(device, &desc);
		buffer->type = (ECGPUResourceTypeFlags)desc.descriptors;
		return buffer;
	}

	void free_buffer(pulse_buffer_data_t* buffer)
	{
		if (buffer->handle)
			cgpu_device_free_buffer(buffer->handle->device, buffer->handle);
		delete buffer;
	}

	std::unique_ptr<pulse_mesh_data_t> create_empty_mesh()
	{
		auto mesh = new pulse_mesh_data_t();
		mesh->vertex_layout = {};
		mesh->p_vertex_attributes = nullptr;
		mesh->prim_topology = CGPU_PRIMITIVE_TOPOLOGY_POINT_LIST;
		mesh->vertices_count = 0;
		mesh->index_count = 0;
		mesh->index_stride = 0;
		mesh->vertex_buffer = nullptr;
		mesh->index_buffer = nullptr;
		mesh->prepared = false;
		return std::unique_ptr<pulse_mesh_data_t>(mesh);
	}

	void init_mesh(pulse_mesh_data_t* mesh, CGPUDeviceId device, uint32_t vertex_count, uint32_t index_count, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride, bool update_vertex_data_from_compute_shader, bool update_index_data_from_compute_shader)
	{
		mesh->vertex_layout = vertex_layout;
		mesh->p_vertex_attributes = new CGPUVertexAttribute[vertex_layout.attribute_count];
		std::copy(vertex_layout.p_attributes, vertex_layout.p_attributes + vertex_layout.attribute_count, mesh->p_vertex_attributes);
		mesh->vertex_layout.p_attributes = mesh->p_vertex_attributes;
		mesh->prim_topology = prim_topology;
		mesh->vertices_count = vertex_count;
		mesh->index_count = index_count;
		mesh->vertex_stride = 0;
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

	std::unique_ptr<pulse_mesh_data_t> create_mesh(CGPUDeviceId device, uint32_t vertex_count, uint32_t index_count, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride, bool update_vertex_data_from_compute_shader, bool update_index_data_from_compute_shader)
	{
		auto mesh = create_empty_mesh();
		init_mesh(mesh.get(), device, vertex_count, index_count, prim_topology, vertex_layout, index_stride, update_vertex_data_from_compute_shader, update_index_data_from_compute_shader);
		return mesh;
	}

	std::unique_ptr<pulse_mesh_data_t> create_dynamic_mesh(ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride)
	{
		auto mesh = create_empty_mesh();
		mesh->vertex_layout = vertex_layout;
		mesh->p_vertex_attributes = new CGPUVertexAttribute[vertex_layout.attribute_count];
		std::copy(vertex_layout.p_attributes, vertex_layout.p_attributes + vertex_layout.attribute_count, mesh->p_vertex_attributes);
		mesh->vertex_layout.p_attributes = mesh->p_vertex_attributes;
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

	PulseRGBufferHandle declare_dynamic_vertex_buffer(pulse_mesh_data_t* mesh, PulseRenderGraphId rg, uint32_t count)
	{
		auto dynamic_vertex_buffer = pulse_render_graph_import_dynamic_buffer(rg, mesh->vertex_buffer);
		pulse_render_graph_buffer_set_size(rg, dynamic_vertex_buffer, count * mesh->vertex_stride);
		pulse_render_graph_buffer_set_type(rg, dynamic_vertex_buffer, CGPU_RESOURCE_TYPE_VERTEX_BUFFER);
		pulse_render_graph_buffer_set_usage(rg, dynamic_vertex_buffer, CGPU_MEMORY_USAGE_GPU_ONLY);
		mesh->vertex_buffer->dynamic_handle = dynamic_vertex_buffer;
		mesh->vertices_count = count;
		return mesh->vertex_buffer->dynamic_handle;
	}

	PulseRGBufferHandle declare_dynamic_index_buffer(pulse_mesh_data_t* mesh, PulseRenderGraphId rg, uint32_t count)
	{
		auto dynamic_index_buffer = pulse_render_graph_import_dynamic_buffer(rg, mesh->index_buffer);
		pulse_render_graph_buffer_set_size(rg, dynamic_index_buffer, count * mesh->index_stride);
		pulse_render_graph_buffer_set_type(rg, dynamic_index_buffer, CGPU_RESOURCE_TYPE_INDEX_BUFFER);
		pulse_render_graph_buffer_set_usage(rg, dynamic_index_buffer, CGPU_MEMORY_USAGE_GPU_ONLY);
		mesh->index_buffer->dynamic_handle = dynamic_index_buffer;
		mesh->index_count = count;
		return mesh->index_buffer->dynamic_handle;
	}

	void dynamic_mesh_reset(pulse_mesh_data_t* mesh)
	{
		mesh->vertices_count = 0;
		mesh->index_count = 0;
		mesh->vertex_buffer->dynamic_handle = {};
		mesh->index_buffer->dynamic_handle = {};
	}

	void free_mesh(pulse_mesh_data_t* mesh)
	{
		if (mesh->p_vertex_attributes)
		{
			delete[] mesh->p_vertex_attributes;
			mesh->p_vertex_attributes = nullptr;
		}
		if (mesh->vertex_buffer)
		{
			free_buffer(mesh->vertex_buffer);
			mesh->vertex_buffer = nullptr;
		}
		if (mesh->index_buffer)
		{
			free_buffer(mesh->index_buffer);
			mesh->index_buffer = nullptr;
		}
	}

	pulse_texture_data_t* create_empty_texture()
	{
		auto texture = new pulse_texture_data_t();
		texture->handle = CGPU_NULLPTR;
		texture->view = CGPU_NULLPTR;
		texture->cur_state_count = 0;
		texture->p_cur_states = nullptr;
		texture->states_consistent = false;
		texture->prepared = false;
		texture->dynamic_handle = {};
		return texture;
	}

	void init_texture(pulse_texture_data_t* texture, CGPUDeviceId device, const CGPUTextureDescriptor& desc)
	{
		CGPUTextureDescriptor new_desc = desc;
		if (desc.depth == 1 && desc.height == 1)
			new_desc.flags |= CGPU_TEXTURE_CREATION_USAGE_FORCE2D;

		texture->handle = cgpu_device_create_texture(device, &new_desc);
		texture->cur_state_count = new_desc.array_size * new_desc.mip_levels;
		texture->p_cur_states = new ECGPUResourceStateFlags[new_desc.array_size * new_desc.mip_levels];
		std::fill(texture->p_cur_states, texture->p_cur_states + texture->cur_state_count, CGPU_RESOURCE_STATE_UNDEFINED);
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
		texture->dynamic_handle = {};
	}

	pulse_texture_data_t* create_texture(CGPUDeviceId device, const CGPUTextureDescriptor& desc)
	{
		auto texture = create_empty_texture();
		init_texture(texture, device, desc);
		return texture;
	}

	void free_texture(pulse_texture_data_t* texture)
	{
		if (texture->view)
			cgpu_device_free_texture_view(texture->view->device, texture->view);
		if (texture->handle)
			cgpu_device_free_texture(texture->handle->device, texture->handle);
		if (texture->p_cur_states)
		{
			delete[] texture->p_cur_states;
			texture->p_cur_states = nullptr;
			texture->cur_state_count = 0;
		}
	}

	template<typename T>
	void simple_vector_init(T*& data, int& size, int& capacity) {
		data = nullptr;
		size = 0;
		capacity = 0;
	}

	template<typename T>
	void simple_vector_push_back(T*& data, int& size, int& capacity, T& value) {
		if (size >= capacity) {
			int new_capacity = capacity == 0 ? 4 : capacity * 2;
			T* new_data = (T*)realloc(data, new_capacity * sizeof(T));

			if (new_data == nullptr) {
				printf("allocate failed\n");
				return;
			}

			data = new_data;
			capacity = new_capacity;
		}

		data[size] = value;
		size++;
	}

	void simple_vector_clear(int& size) {
		size = 0;
	}

	template<typename T>
	void simple_vector_free(T*& data, int& size, int& capacity) {
		if (data != nullptr) {
			free(data);
			data = nullptr;
		}
		size = 0;
		capacity = 0;
	}

	void init_material(pulse_material_data_t* material, CGPUDeviceId device, pulse_shader_data_t* shader)
	{
		material->device = device;
		material->shader = shader;
		simple_vector_init(material->buffers.data, material->buffers.size, material->buffers.capacity);
		simple_vector_init(material->textures.data, material->textures.size, material->textures.capacity);
		simple_vector_init(material->samplers.data, material->samplers.size, material->samplers.capacity);
		simple_vector_init(material->uboColumns.data, material->uboColumns.size, material->uboColumns.capacity);
		simple_vector_init(material->ownedBuffers.data, material->ownedBuffers.size, material->ownedBuffers.capacity);
		simple_vector_init(material->materialDsets.data, material->materialDsets.size, material->materialDsets.capacity);

		if (shader) {
			for (uint32_t i = 0; i < shader->ubo_info_count; ++i) {
				auto& ubo_info = shader->p_ubo_infos[i];
				pulse_material_ubo_column_t col = {};
				col.set = ubo_info.set;
				col.binding = ubo_info.binding;
				col.size = ubo_info.ubo_size;
				if (ubo_info.material_managed) {
					col.cpu_data = (uint8_t*)calloc(1, ubo_info.ubo_size);
				}
				col.dirty = ubo_info.material_managed;
				col.gpu_buffer = nullptr;
				simple_vector_push_back<pulse_material_ubo_column_t>(
					material->uboColumns.data,
					material->uboColumns.size,
					material->uboColumns.capacity,
					col);
			}

			for (uint32_t i = 0; i < shader->set_info_count; ++i) {
				auto& set_info = shader->p_set_infos[i];
				if (set_info.renderer_managed) continue;

				CGPUDescriptorSetDescriptor dset_desc = {};
				dset_desc.root_signature = shader->root_sig;
				dset_desc.set_index = set_info.set_index;
				auto dset_handle = cgpu_device_create_descriptor_set(device, &dset_desc);
			if (dset_handle) {
				pulse_material_descriptor_set_t mdset = {};
				mdset.set_index = set_info.set_index;
				mdset.handle = dset_handle;
				mdset.data_hash = 0;
				mdset.binding_dirty = true;
				simple_vector_push_back<pulse_material_descriptor_set_t>(
					material->materialDsets.data,
					material->materialDsets.size,
					material->materialDsets.capacity,
					mdset);
			}
			}
		}
	}

	void free_material(pulse_material_data_t* material)
	{
		simple_vector_free(material->buffers.data, material->buffers.size, material->buffers.capacity);
		simple_vector_free(material->textures.data, material->textures.size, material->textures.capacity);
		simple_vector_free(material->samplers.data, material->samplers.size, material->samplers.capacity);
		for (int i = 0; i < material->uboColumns.size; ++i)
		{
			auto& col = material->uboColumns.data[i];
			if (col.cpu_data)
				free(col.cpu_data);
			if (col.gpu_buffer)
				free_buffer(col.gpu_buffer);
		}
		simple_vector_free(material->uboColumns.data, material->uboColumns.size, material->uboColumns.capacity);
		for (int i = 0; i < material->ownedBuffers.size; ++i)
		{
			free_buffer(material->ownedBuffers.data[i]);
		}
		simple_vector_free(material->ownedBuffers.data, material->ownedBuffers.size, material->ownedBuffers.capacity);
		for (int i = 0; i < material->materialDsets.size; ++i)
		{
			if (material->materialDsets.data[i].handle)
				cgpu_device_free_descriptor_set(material->device, material->materialDsets.data[i].handle);
		}
		simple_vector_free(material->materialDsets.data, material->materialDsets.size, material->materialDsets.capacity);
		material->shader = nullptr;
		material->device = nullptr;
	}

	void material_mark_dset_binding_dirty(pulse_material_data_t* material, uint32_t set_index)
	{
		for (int i = 0; i < material->materialDsets.size; ++i)
		{
			if (material->materialDsets.data[i].set_index == set_index)
			{
				material->materialDsets.data[i].binding_dirty = true;
				break;
			}
		}
	}

	void material_bindTexture(pulse_material_data_t* material, int set, int bind, pulse_texture_data_t* texture)
	{
		for (int i = 0; i < material->textures.size; ++i)
		{
			if (material->textures.data[i].set == set && material->textures.data[i].bind == bind)
			{
				material->textures.data[i].texture = texture;
				material_mark_dset_binding_dirty(material, (uint32_t)set);
				return;
			}
		}
		pulse_material_bind_texture_t entry = pulse_material_bind_texture_t(set, bind, texture);
		simple_vector_push_back<pulse_material_bind_texture_t>(material->textures.data, material->textures.size, material->textures.capacity, entry);
		material_mark_dset_binding_dirty(material, (uint32_t)set);
	}

	void material_bindSampler(pulse_material_data_t* material, int set, int bind, pulse_sampler_data_t* sampler)
	{
		material_bindSampler(material, set, bind, sampler->handle);
	}

	void material_bindSampler(pulse_material_data_t* material, int set, int bind, CGPUSamplerId sampler)
	{
		for (int i = 0; i < material->samplers.size; ++i)
		{
			if (material->samplers.data[i].set == set && material->samplers.data[i].bind == bind)
			{
				material->samplers.data[i].sampler = sampler;
				material_mark_dset_binding_dirty(material, (uint32_t)set);
				return;
			}
		}
		pulse_material_bind_sampler_t entry = pulse_material_bind_sampler_t(set, bind, sampler);
		simple_vector_push_back<pulse_material_bind_sampler_t>(material->samplers.data, material->samplers.size, material->samplers.capacity, entry);
		material_mark_dset_binding_dirty(material, (uint32_t)set);
	}

	void material_bindBuffer(pulse_material_data_t* material, int set, int bind, pulse_buffer_data_t* buffer)
	{
		for (int i = 0; i < material->buffers.size; ++i)
		{
			if (material->buffers.data[i].set == set && material->buffers.data[i].bind == bind)
			{
				material->buffers.data[i].buffer = buffer;
				material_mark_dset_binding_dirty(material, (uint32_t)set);
				return;
			}
		}
		pulse_material_bind_buffer_t entry = pulse_material_bind_buffer_t(set, bind, buffer);
		simple_vector_push_back<pulse_material_bind_buffer_t>(material->buffers.data, material->buffers.size, material->buffers.capacity, entry);
		material_mark_dset_binding_dirty(material, (uint32_t)set);
	}

	pulse_material_ubo_column_t* material_find_ubo_column(pulse_material_data_t* material, uint32_t set, uint32_t binding)
	{
		for (int i = 0; i < material->uboColumns.size; ++i)
		{
			auto& col = material->uboColumns.data[i];
			if (col.set == set && col.binding == binding)
				return &col;
		}
		return nullptr;
	}

	void material_ubo_sync_to_gpu(pulse_material_data_t* material)
	{
		for (int i = 0; i < material->uboColumns.size; ++i)
		{
			auto& col = material->uboColumns.data[i];
			if (!col.dirty) continue;
			if (!col.gpu_buffer)
			{
				auto desc = CGPUBufferDescriptor{
					.size = col.size,
					.name = "Material UBO",
					.descriptors = CGPU_RESOURCE_TYPE_UNIFORM_BUFFER,
					.memory_usage = CGPU_MEMORY_USAGE_CPU_TO_GPU,
				};
				col.gpu_buffer = create_buffer(material->device, desc);
			}
			cgpu_buffer_map(col.gpu_buffer->handle, nullptr);
			memcpy(col.gpu_buffer->handle->info->cpu_mapped_address, col.cpu_data, col.size);
			cgpu_buffer_unmap(col.gpu_buffer->handle);
			col.dirty = false;
		}
	}

	const uint8_t* material_get_property_data(pulse_material_data_t* material, uint32_t set, uint32_t binding, uint32_t offset, uint32_t size)
	{
		for (int i = 0; i < material->uboColumns.size; ++i)
		{
			auto& col = material->uboColumns.data[i];
			if (col.set == set && col.binding == binding && col.cpu_data && col.size >= offset + size)
				return col.cpu_data + offset;
		}
		return nullptr;
	}

	void material_sync_descriptor_sets(RenderPassEncoder* encoder, pulse_material_data_t* material)
	{
		if (!material->shader) return;
		auto root_sig = material->shader->root_sig;

		for (int m = 0; m < material->materialDsets.size; ++m)
		{
			auto& mdset = material->materialDsets.data[m];
			if (!mdset.binding_dirty) continue;

			uint32_t set_idx = mdset.set_index;

			// Find the table in root signature for this set
			uint32_t table_idx = 0;
			for (; table_idx < root_sig->table_count; ++table_idx)
			{
				if (root_sig->p_tables[table_idx].set_index == set_idx)
					break;
			}
			if (table_idx >= root_sig->table_count)
			{
				mdset.binding_dirty = false;
				continue;
			}

			auto& table = root_sig->p_tables[table_idx];
			const uint32_t data_size = 64;
			CGPUDescriptorData datas[data_size] = {};
			uint32_t data_count = 0;
			uint32_t tex_view_count = 0;
			uint32_t sampler_count = 0;
			uint32_t buffer_count = 0;

			CGPUTextureViewId tex_views[data_size];
			CGPUSamplerId samplers[data_size];
			CGPUBufferId buffers[data_size];

			for (uint32_t j = 0; j < std::min(data_size, table.resources_count); ++j)
			{
				auto& res = table.p_resources[j];
				CGPUDescriptorData data = {};
				data.binding = res.binding;
				data.binding_type = res.type;
				data.count = 1;

				if (res.type == CGPU_RESOURCE_TYPE_TEXTURE)
				{
					CGPUTextureViewId tex_view = CGPU_NULLPTR;
					for (int b = 0; b < material->textures.size; ++b)
					{
						auto& bind = material->textures.data[b];
						if ((uint32_t)bind.set == set_idx && (uint32_t)bind.bind == res.binding)
						{
							if (bind.texture && bind.texture->view)
								tex_view = bind.texture->view;
							break;
						}
					}
					if (!tex_view && encoder->context->default_texture)
						tex_view = encoder->context->default_texture;
					tex_views[tex_view_count] = tex_view;
					data.resources.textures = tex_views + tex_view_count;
					++tex_view_count;
				}
				else if (res.type == CGPU_RESOURCE_TYPE_SAMPLER)
				{
					CGPUSamplerId sampler = CGPU_NULLPTR;
					for (int b = 0; b < material->samplers.size; ++b)
					{
						auto& bind = material->samplers.data[b];
						if ((uint32_t)bind.set == set_idx && (uint32_t)bind.bind == res.binding)
						{
							sampler = bind.sampler;
							break;
						}
					}
					samplers[sampler_count] = sampler;
					data.resources.samplers = samplers + sampler_count;
					++sampler_count;
				}
				else if (res.type == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER || res.type == CGPU_RESOURCE_TYPE_RW_BUFFER)
				{
					CGPUBufferId buffer = CGPU_NULLPTR;
					// Search material buffers first
					for (int b = 0; b < material->buffers.size; ++b)
					{
						auto& bind = material->buffers.data[b];
						if ((uint32_t)bind.set == set_idx && (uint32_t)bind.bind == res.binding)
						{
							if (bind.buffer)
								buffer = bind.buffer->handle;
							break;
						}
					}
					// Search material UBO columns as fallback
					if (!buffer)
					{
						for (int b = 0; b < material->uboColumns.size; ++b)
						{
							auto& col = material->uboColumns.data[b];
							if (col.set == set_idx && col.binding == res.binding && col.gpu_buffer)
							{
								buffer = col.gpu_buffer->handle;
								break;
							}
						}
					}
					buffers[buffer_count] = buffer;
					data.resources.buffers = buffers + buffer_count;
					++buffer_count;
				}

				if (data.resources.ptrs != nullptr)
					datas[data_count++] = data;
			}

			if (data_count > 0)
				cgpu_descriptor_set_update(mdset.handle, data_count, datas);

			mdset.binding_dirty = false;
		}
	}

	void init_backbuffer(pulse_backbuffer_data_t* backbuffer, CGPUSwapChainId swapchain, int index)
	{
		backbuffer->texture.handle = swapchain->p_back_buffers[index];
		backbuffer->texture.view = CGPU_NULLPTR;
		backbuffer->texture.cur_state_count = 1;
		backbuffer->texture.p_cur_states = new ECGPUResourceStateFlags[backbuffer->texture.cur_state_count];
		backbuffer->texture.p_cur_states[0] = CGPU_RESOURCE_STATE_UNDEFINED;
		backbuffer->texture.states_consistent = true;
		backbuffer->texture.dynamic_handle = {};
	}

	void free_backbuffer(pulse_backbuffer_data_t* backbuffer)
	{
		backbuffer->texture.handle = CGPU_NULLPTR;
		backbuffer->texture.view = CGPU_NULLPTR;
		backbuffer->texture.cur_state_count = 0;
		delete[] backbuffer->texture.p_cur_states;
		backbuffer->texture.p_cur_states = nullptr;
		backbuffer->texture.states_consistent = false;
	}

	void set_viewport(RenderPassEncoder* encoder, float x, float y, float width, float height, float min_depth, float max_depth)
	{
		cgpu_render_pass_encoder_set_viewport(encoder->encoder, x, y, width, height, min_depth, max_depth);
	}

	void set_scissor(RenderPassEncoder* encoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		cgpu_render_pass_encoder_set_scissor(encoder->encoder, x, y, width, height);
	}

	void push_constants(RenderPassEncoder* encoder, pulse_shader_data_t* shader, const char* name, const void* data)
	{
		cgpu_render_pass_encoder_push_constants(encoder->encoder, shader->root_sig, name, data);
	}

	void update_render_pipeline(RenderPassEncoder* encoder, pulse_shader_data_t* shader, ECGPUPrimitiveTopology mesh_topology, const CGPUVertexLayout& vertex_layout)
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

			if (shader != encoder->last_shader)
			{
				memset(encoder->last_bind_resources, 0, sizeof(encoder->last_bind_resources));
				memset(encoder->last_textureviews, 0, sizeof(encoder->last_textureviews));
				memset(encoder->last_samplers, 0, sizeof(encoder->last_samplers));
				memset(encoder->last_buffers, 0, sizeof(encoder->last_buffers));
				memset(encoder->last_buffer_offset_sizes, 0, sizeof(encoder->last_buffer_offset_sizes));
				for (uint32_t i = 0; i < std::min(shader->set_info_count, 4u); ++i)
					encoder->last_set_layout_hashes[i] = shader->p_set_infos[i].layout_hash;
				encoder->last_shader = shader;
			}
		}
	}

	void update_descriptor_set(RenderPassEncoder* encoder, CGPURootSignatureId root_sig, bool is_graphics)
	{
		pulse_material_data_t* material = encoder->last_material;
		for (uint32_t i = 0; i < std::min(4u, root_sig->table_count); ++i)
		{
			auto& table = root_sig->p_tables[i];

			// Check for pre-built descriptor set (from material)
			CGPUDescriptorSetId prebuilt_dset = CGPU_NULLPTR;
			if (material)
			{
				for (int m = 0; m < material->materialDsets.size; ++m)
				{
					auto& mdset = material->materialDsets.data[m];
					if (mdset.set_index == (int)table.set_index)
					{
						prebuilt_dset = mdset.handle;
						break;
					}
				}
			}
			if (prebuilt_dset)
			{
				if (is_graphics)
					cgpu_render_pass_encoder_bind_descriptor_set(encoder->encoder, prebuilt_dset);
				else
					cgpu_compute_pass_encoder_bind_descriptor_set(encoder->compute_encoder, prebuilt_dset);
				memset(encoder->last_bind_resources[i], 0, sizeof(encoder->last_bind_resources[i]));
				continue;
			}

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
					if (material)
					{
						for (int b = 0; b < material->textures.size; ++b)
						{
							auto& bind = material->textures.data[b];
							if ((uint32_t)bind.set == table.set_index && (uint32_t)bind.bind == res.binding)
							{
								if (bind.texture && bind.texture->view)
									textureview = bind.texture->view;
								break;
							}
						}
					}
					if (!textureview)
					{
						for (auto iter = encoder->global_texture_table.rbegin(); iter != encoder->global_texture_table.rend(); ++iter)
						{
							auto& binder = *iter;
							if (binder.set == table.set_index && binder.bind == res.binding)
							{
								if (pulse_rgtexture_handle_is_valid(binder.texture_handle))
									textureview = pulse_render_pass_encoder_resolve_texture_view((PulseRenderPassEncoder*)encoder, binder.texture_handle);
								else if (binder.texture && binder.texture->prepared)
									textureview = binder.texture->view;
								break;
							}
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
					if (material)
					{
						for (int b = 0; b < material->samplers.size; ++b)
						{
							auto& bind = material->samplers.data[b];
							if ((uint32_t)bind.set == table.set_index && (uint32_t)bind.bind == res.binding)
							{
								sampler = bind.sampler;
								break;
							}
						}
					}
					if (!sampler)
					{
						for (auto iter = encoder->global_sampler_table.rbegin(); iter != encoder->global_sampler_table.rend(); ++iter)
						{
							auto& binder = *iter;
							if (binder.set == table.set_index && binder.bind == res.binding)
							{
								sampler = binder.sampler;
								break;
							}
						}
					}
					if (!sampler)
						sampler = encoder->context->default_sampler;
					encoder->samplers[sampler_count] = sampler;
					data.resources.samplers = encoder->samplers + sampler_count;
					++sampler_count;
				}
				else if (res.type == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER || res.type == CGPU_RESOURCE_TYPE_RW_BUFFER)
				{
					CGPUBufferId buffer = CGPU_NULLPTR;
					if (material)
					{
						for (int b = 0; b < material->buffers.size; ++b)
						{
							auto& bind = material->buffers.data[b];
							if ((uint32_t)bind.set == table.set_index && (uint32_t)bind.bind == res.binding)
							{
								if (bind.buffer)
									buffer = bind.buffer->handle;
								break;
							}
						}
						if (!buffer)
						{
							for (int b = 0; b < material->uboColumns.size; ++b)
							{
								auto& col = material->uboColumns.data[b];
								if (col.set == table.set_index && col.binding == res.binding && col.gpu_buffer)
								{
									buffer = col.gpu_buffer->handle;
									break;
								}
							}
						}
					}
					if (!buffer)
					{
						for (auto iter = encoder->global_buffer_table.rbegin(); iter != encoder->global_buffer_table.rend(); ++iter)
						{
							auto& binder = *iter;
							if (binder.set == table.set_index && binder.bind == res.binding)
							{
								if (pulse_rgbuffer_handle_is_valid(binder.buffer_handle))
									buffer = pulse_render_pass_encoder_resolve_buffer((PulseRenderPassEncoder*)encoder, binder.buffer_handle);
								else
									buffer = binder.buffer->handle;
								if (binder.offset != 0 || binder.size != 0)
								{
									encoder->buffer_offset_sizes[offset_size_count] = binder.offset;
									data.params.buffers_params.offsets = encoder->buffer_offset_sizes + (offset_size_count++);
									encoder->buffer_offset_sizes[offset_size_count] = binder.size;
									data.params.buffers_params.sizes = encoder->buffer_offset_sizes + (offset_size_count++);
								}
								break;
							}
						}
					}
					if (buffer)
					{
						encoder->buffers[buffer_count] = buffer;
						data.resources.buffers = encoder->buffers + buffer_count;
						++buffer_count;
					}
				}
				if (data.resources.ptrs != nullptr)
					datas[data_count++] = data;
			}

			if (data_count > 0)
			{
				bool dset_dirty = !compare(datas, encoder->last_bind_resources[i], data_count);
				bool buffer_dirty = memcmp(encoder->buffers, encoder->last_buffers[i], sizeof(CGPUBufferId) * buffer_count);
				bool textureview_dirty = memcmp(encoder->textureviews, encoder->last_textureviews[i], sizeof(CGPUTextureViewId) * texture_view_count);
				bool sampler_dirty = memcmp(encoder->samplers, encoder->last_samplers[i], sizeof(CGPUSamplerId) * sampler_count);
				bool offset_size_dirty = memcmp(encoder->buffer_offset_sizes, encoder->last_buffer_offset_sizes[i], sizeof(uint64_t) * offset_size_count);
				if (dset_dirty || buffer_dirty || textureview_dirty || sampler_dirty || offset_size_dirty)
				{
					cgpu_descriptor_set_update(dset->handle, data_count, datas);
					if (is_graphics)
						cgpu_render_pass_encoder_bind_descriptor_set(encoder->encoder, dset->handle);
					else
						cgpu_compute_pass_encoder_bind_descriptor_set(encoder->compute_encoder, dset->handle);
					memcpy(encoder->last_bind_resources[i], datas, sizeof(CGPUDescriptorData) * data_count);
					memcpy(encoder->last_buffers[i], encoder->buffers, sizeof(CGPUBufferId) * buffer_count);
					memcpy(encoder->last_textureviews[i], encoder->textureviews, sizeof(CGPUTextureViewId) * texture_view_count);
					memcpy(encoder->last_samplers[i], encoder->samplers, sizeof(CGPUSamplerId) * sampler_count);
					memcpy(encoder->last_buffer_offset_sizes[i], encoder->buffer_offset_sizes, sizeof(uint64_t) * offset_size_count);
				}
			}
		}
	}

	void update_material(RenderPassEncoder* encoder, pulse_material_data_t* material)
	{
		material_ubo_sync_to_gpu(material);
		material_sync_descriptor_sets(encoder, material);
		encoder->last_material = material;
	}

	void update_mesh(RenderPassEncoder* encoder, pulse_mesh_data_t* mesh)
	{
		CGPUBufferId vertex_buffer = CGPU_NULLPTR;
		if (mesh->vertex_buffer)
		{
			if (pulse_rgbuffer_handle_is_valid(mesh->vertex_buffer->dynamic_handle))
			{
				auto vertex_buffer_handle = mesh->vertex_buffer->dynamic_handle;
				vertex_buffer = pulse_render_pass_encoder_resolve_buffer((PulseRenderPassEncoder*)encoder, vertex_buffer_handle);
			}
			else
			{
				vertex_buffer = mesh->vertex_buffer->handle;
			}
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
			if (pulse_rgbuffer_handle_is_valid(mesh->index_buffer->dynamic_handle))
			{
				auto index_buffer_handle = mesh->index_buffer->dynamic_handle;
				index_buffer = pulse_render_pass_encoder_resolve_buffer((PulseRenderPassEncoder*)encoder, index_buffer_handle);
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

	void draw(RenderPassEncoder* encoder, pulse_shader_data_t* shader, pulse_mesh_data_t* mesh)
	{
		if (!mesh->prepared)
			return;
		encoder->last_material = nullptr;
		update_render_pipeline(encoder, shader, mesh->prim_topology, mesh->vertex_layout);
		update_descriptor_set(encoder, shader->root_sig, true);
		update_mesh(encoder, mesh);
		if (encoder->last_index_buffer)
			cgpu_render_pass_encoder_draw_indexed(encoder->encoder, mesh->index_count, 0, 0);
		else
			cgpu_render_pass_encoder_draw(encoder->encoder, mesh->vertices_count, 0);
	}

	void draw_submesh(RenderPassEncoder* encoder, pulse_shader_data_t* shader, pulse_mesh_data_t* mesh, uint32_t index_count, uint32_t first_index, uint32_t vertex_count, uint32_t first_vertex)
	{
		if (!mesh->prepared)
			return;
		encoder->last_material = nullptr;
		update_render_pipeline(encoder, shader, mesh->prim_topology, mesh->vertex_layout);
		update_descriptor_set(encoder, shader->root_sig, true);
		update_mesh(encoder, mesh);
		if (encoder->last_index_buffer)
			cgpu_render_pass_encoder_draw_indexed(encoder->encoder, index_count, first_index, first_vertex);
		else
			cgpu_render_pass_encoder_draw(encoder->encoder, vertex_count, first_vertex);
	}

	static CGPUVertexLayout procedure_vertex_layout = { .attribute_count = 0 };
	void draw_procedure(RenderPassEncoder* encoder, pulse_shader_data_t* shader, ECGPUPrimitiveTopology mesh_topology, uint32_t vertex_count)
	{
		encoder->last_material = nullptr;
		update_render_pipeline(encoder, shader, mesh_topology, procedure_vertex_layout);
		update_descriptor_set(encoder, shader->root_sig, true);
		cgpu_render_pass_encoder_draw(encoder->encoder, vertex_count, 0);
	}

	void draw(RenderPassEncoder* encoder, pulse_material_data_t* material, pulse_mesh_data_t* mesh)
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

	void draw_submesh(RenderPassEncoder* encoder, pulse_material_data_t* material, pulse_mesh_data_t* mesh, uint32_t index_count, uint32_t first_index, uint32_t vertex_count, uint32_t first_vertex)
	{
		if (!mesh->prepared || !material)
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

	void draw_procedure(RenderPassEncoder* encoder, pulse_material_data_t* material, ECGPUPrimitiveTopology mesh_topology, uint32_t vertex_count)
	{
		if (!material)
			return;
		update_material(encoder, material);
		auto shader = material->shader;
		update_render_pipeline(encoder, shader, mesh_topology, procedure_vertex_layout);
		update_descriptor_set(encoder, shader->root_sig, true);
		cgpu_render_pass_encoder_draw(encoder->encoder, vertex_count, 0);
	}

	void update_compute_pipeline(RenderPassEncoder* encoder, pulse_compute_shader_data_t* shader)
	{
		auto pipeline = encoder->context->computePipelinePool.getComputePipeline(shader);
		if (pipeline && pipeline->handle != encoder->last_compute_pipeline)
		{
			cgpu_compute_pass_encoder_bind_compute_pipeline(encoder->compute_encoder, pipeline->handle);
			encoder->last_compute_pipeline = pipeline->handle;
			memset(encoder->last_bind_resources, 0, sizeof(encoder->last_bind_resources));
			memset(encoder->last_textureviews, 0, sizeof(encoder->last_textureviews));
			memset(encoder->last_samplers, 0, sizeof(encoder->last_samplers));
			memset(encoder->last_buffers, 0, sizeof(encoder->last_buffers));
			memset(encoder->last_buffer_offset_sizes, 0, sizeof(encoder->last_buffer_offset_sizes));
		}
	}

	void dispatch(RenderPassEncoder* encoder, pulse_compute_shader_data_t* shader, uint32_t thread_x, uint32_t thread_y, uint32_t thread_z)
	{
		encoder->last_material = nullptr;
		update_compute_pipeline(encoder, shader);
		update_descriptor_set(encoder, shader->root_sig, false);
		cgpu_compute_pass_encoder_dispatch(encoder->compute_encoder, thread_x, thread_y, thread_z);
	}

	void set_global_texture(RenderPassEncoder* encoder, pulse_texture_data_t* texture, int set, int slot)
	{
		encoder->global_texture_table.push_back({ texture, {}, set, slot });
	}

	void set_global_texture_handle(RenderPassEncoder* encoder, PulseRGTextureHandle texture, int set, int slot)
	{
		encoder->global_texture_table.push_back({ nullptr, texture, set, slot });
	}

	void set_global_sampler(RenderPassEncoder* encoder, CGPUSamplerId sampler, int set, int slot)
	{
		encoder->global_sampler_table.push_back({ sampler, set, slot });
	}

	void set_global_buffer(RenderPassEncoder* encoder, pulse_buffer_data_t* buffer, int set, int slot)
	{
		encoder->global_buffer_table.push_back({ buffer, {}, set, slot, 0, 0 });
	}

	void set_global_dynamic_buffer(RenderPassEncoder* encoder, PulseRGBufferHandle buffer, int set, int slot)
	{
		encoder->global_buffer_table.push_back({ nullptr, buffer, set, slot, 0, 0 });
	}

	void set_global_buffer_with_offset_size(RenderPassEncoder* encoder, PulseRGBufferHandle buffer, int set, int slot, uint64_t offset, uint64_t size)
	{
		encoder->global_buffer_table.push_back({ nullptr, buffer, set, slot, offset, size });
	}

	void upload(UploadEncoder* encoder, uint64_t offset, uint64_t length, void* data)
	{
		char* address = (char*)encoder->address + offset;
		memcpy(address, data, length);
	}

	ExecutorContext::ExecutorContext(CGPUDeviceId device, CGPUQueueId gfx_queue, bool profile, std::pmr::memory_resource* memory_resource)
		: device(device), memory_resource(memory_resource), renderPassPool(device, memory_resource), framebufferPool(device, memory_resource), texturePool(device, gfx_queue, nullptr, memory_resource), pipelinePool(device, nullptr, memory_resource), computePipelinePool(device, nullptr, memory_resource), textureViewPool(nullptr, memory_resource), bufferPool(device, nullptr, memory_resource), descriptorSetPool(device, memory_resource), allocated_dsets(memory_resource)
		, cmds(memory_resource), allocated_cmds(memory_resource)
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