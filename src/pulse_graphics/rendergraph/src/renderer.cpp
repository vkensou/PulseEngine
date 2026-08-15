#include "renderer.h"

#include <vector>
#include "hash.h"
#include "rendergraph_compiler_internal.h"
#include <bit>
#include "drawer.h"
#include "compare.h"

namespace HGEGraphics
{
	std::unique_ptr<PulseShaderData> create_shader(CGPUDeviceId device, const uint8_t* vert_data, uint32_t vert_length, const uint8_t* frag_data, uint32_t frag_length, const CGPUBlendStateDescriptor& blend_desc, const CGPUDepthStateDescriptor& depth_desc, const CGPURasterizerStateDescriptor& rasterizer_state)
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

	std::unique_ptr<PulseShaderData> create_shader_from_libraries(
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
			.dynamic_buffers = true,
		};
		auto root_sig = cgpu_device_create_root_signature(device, &rs_desc);

		auto shader = new PulseShaderData();
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
		return std::unique_ptr<PulseShaderData>(shader);
	}

	void free_shader(PulseShaderData* shader)
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

	std::unique_ptr<PulseComputeShaderData> create_compute_shader(CGPUDeviceId device, const uint8_t* comp_data, uint32_t comp_length)
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

	std::unique_ptr<PulseComputeShaderData> create_compute_shader_from_library(
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
			.dynamic_buffers = true,
		};
		auto root_sig = cgpu_device_create_root_signature(device, &rs_desc);

		auto shader = new PulseComputeShaderData();
		shader->root_sig = root_sig;
		shader->cs = ppl_shaders[0];
		return std::unique_ptr<PulseComputeShaderData>(shader);
	}

	void free_compute_shader(PulseComputeShaderData* compute_shader)
	{
		if (compute_shader->root_sig)
			cgpu_device_free_root_signature(compute_shader->root_sig->device, compute_shader->root_sig);
		if (compute_shader->cs.library)
			cgpu_device_free_shader_library(compute_shader->cs.library->device, compute_shader->cs.library);
	}

	PulseGraphicsBufferData* create_empty_buffer()
	{
		auto buffer = new PulseGraphicsBufferData();
		buffer->handle = CGPU_NULLPTR;
		buffer->type = CGPU_RESOURCE_TYPE_NONE;
		buffer->cur_state = CGPU_RESOURCE_STATE_UNDEFINED;
		buffer->dynamic_handle = {};
		return buffer;
	}

	PulseGraphicsBufferData* create_buffer(CGPUDeviceId device, const CGPUBufferDescriptor& desc)
	{
		auto buffer = create_empty_buffer();
		buffer->handle = cgpu_device_create_buffer(device, &desc);
		buffer->type = (ECGPUResourceTypeFlags)desc.descriptors;
		return buffer;
	}

	void free_buffer(PulseGraphicsBufferData* buffer)
	{
		if (buffer->handle)
			cgpu_device_free_buffer(buffer->handle->device, buffer->handle);
		delete buffer;
	}

	std::unique_ptr<PulseMeshData> create_empty_mesh()
	{
		auto mesh = new PulseMeshData();
		mesh->vertex_layout = {};
		mesh->p_vertex_attributes = nullptr;
		mesh->prim_topology = CGPU_PRIMITIVE_TOPOLOGY_POINT_LIST;
		mesh->vertices_count = 0;
		mesh->index_count = 0;
		mesh->index_stride = 0;
		mesh->vertex_buffer = {};
		mesh->index_buffer = {};
		mesh->prepared = false;
		return std::unique_ptr<PulseMeshData>(mesh);
	}

	void init_mesh(PulseMeshData* mesh, CGPUDeviceId device, uint32_t vertex_count, uint32_t index_count, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride, bool update_vertex_data_from_compute_shader, bool update_index_data_from_compute_shader)
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
		mesh->vertex_buffer.ptr = create_buffer(device, vertex_buffer_desc);

		if (index_count > 0)
		{
			CGPUBufferDescriptor index_buffer_desc = {};
			index_buffer_desc.name = "index buffer";
			index_buffer_desc.flags = CGPU_BUFFER_CREATION_USAGE_PERSISTENT_MAP;
			index_buffer_desc.descriptors = update_index_data_from_compute_shader ? CGPU_RESOURCE_TYPE_INDEX_BUFFER | CGPU_RESOURCE_TYPE_RW_BUFFER : CGPU_RESOURCE_TYPE_INDEX_BUFFER;
			index_buffer_desc.memory_usage = CGPU_MEMORY_USAGE_GPU_ONLY;
			index_buffer_desc.size = index_count * mesh->index_stride;
			mesh->index_buffer.ptr = create_buffer(device, index_buffer_desc);
		}
		mesh->prepared = false;
	}

	std::unique_ptr<PulseMeshData> create_mesh(CGPUDeviceId device, uint32_t vertex_count, uint32_t index_count, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride, bool update_vertex_data_from_compute_shader, bool update_index_data_from_compute_shader)
	{
		auto mesh = create_empty_mesh();
		init_mesh(mesh.get(), device, vertex_count, index_count, prim_topology, vertex_layout, index_stride, update_vertex_data_from_compute_shader, update_index_data_from_compute_shader);
		return mesh;
	}

	void create_dynamic_mesh(PulseMeshData* mesh, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride)
	{
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
        mesh->index_count = 0;
		mesh->index_stride = index_stride;
        mesh->vertex_buffer.handle = {};
		mesh->vertex_buffer.ptr = create_empty_buffer();
        mesh->index_buffer.handle = {};
        mesh->index_buffer.ptr = create_empty_buffer();
		mesh->prepared = true;
	}

	PulseRGBufferHandle declare_dynamic_vertex_buffer(PulseMeshData* mesh, PulseRenderGraphId rg, uint32_t count)
	{
		auto dynamic_vertex_buffer = pulse_render_graph_import_dynamic_buffer(rg, mesh->vertex_buffer.ptr);
		pulse_render_graph_buffer_set_size(rg, dynamic_vertex_buffer, count * mesh->vertex_stride);
		pulse_render_graph_buffer_set_type(rg, dynamic_vertex_buffer, CGPU_RESOURCE_TYPE_VERTEX_BUFFER);
		pulse_render_graph_buffer_set_usage(rg, dynamic_vertex_buffer, CGPU_MEMORY_USAGE_GPU_ONLY);
		mesh->vertex_buffer.ptr->dynamic_handle = dynamic_vertex_buffer;
		mesh->vertices_count = count;
		return mesh->vertex_buffer.ptr->dynamic_handle;
	}

	PulseRGBufferHandle declare_dynamic_index_buffer(PulseMeshData* mesh, PulseRenderGraphId rg, uint32_t count)
	{
		auto dynamic_index_buffer = pulse_render_graph_import_dynamic_buffer(rg, mesh->index_buffer.ptr);
		pulse_render_graph_buffer_set_size(rg, dynamic_index_buffer, count * mesh->index_stride);
		pulse_render_graph_buffer_set_type(rg, dynamic_index_buffer, CGPU_RESOURCE_TYPE_INDEX_BUFFER);
		pulse_render_graph_buffer_set_usage(rg, dynamic_index_buffer, CGPU_MEMORY_USAGE_GPU_ONLY);
		mesh->index_buffer.ptr->dynamic_handle = dynamic_index_buffer;
		mesh->index_count = count;
		return mesh->index_buffer.ptr->dynamic_handle;
	}

	void dynamic_mesh_reset(PulseMeshData* mesh)
	{
		mesh->vertices_count = 0;
		mesh->index_count = 0;
		mesh->vertex_buffer.ptr->dynamic_handle = {};
		mesh->index_buffer.ptr->dynamic_handle = {};
	}

	void free_mesh(PulseMeshData* mesh)
	{
		if (mesh->p_vertex_attributes)
		{
			delete[] mesh->p_vertex_attributes;
			mesh->p_vertex_attributes = nullptr;
		}
		if (mesh->vertex_buffer.ptr)
		{
			free_buffer(mesh->vertex_buffer.ptr);
			mesh->vertex_buffer = {};
		}
		if (mesh->index_buffer.ptr)
		{
			free_buffer(mesh->index_buffer.ptr);
			mesh->index_buffer = {};
		}
	}

	PulseTextureData* create_empty_texture()
	{
		auto texture = new PulseTextureData();
		texture->handle = CGPU_NULLPTR;
		texture->view = CGPU_NULLPTR;
		texture->cur_state_count = 0;
		texture->p_cur_states = nullptr;
		texture->states_consistent = false;
		texture->prepared = false;
		texture->dynamic_handle = {};
		return texture;
	}

	void init_texture(PulseTextureData* texture, CGPUDeviceId device, const CGPUTextureDescriptor& desc)
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

	PulseTextureData* create_texture(CGPUDeviceId device, const CGPUTextureDescriptor& desc)
	{
		auto texture = create_empty_texture();
		init_texture(texture, device, desc);
		return texture;
	}

	void free_texture(PulseTextureData* texture)
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

	void init_material(PulseMaterialData* material, CGPUDeviceId device, PulseShader shader)
	{
		material->device = device;
		material->shader = shader;
		simple_vector_init(material->buffers.data, material->buffers.size, material->buffers.capacity);
		simple_vector_init(material->textures.data, material->textures.size, material->textures.capacity);
		simple_vector_init(material->samplers.data, material->samplers.size, material->samplers.capacity);
		simple_vector_init(material->uboColumns.data, material->uboColumns.size, material->uboColumns.capacity);
		simple_vector_init(material->ownedBuffers.data, material->ownedBuffers.size, material->ownedBuffers.capacity);
		simple_vector_init(material->materialDsets.data, material->materialDsets.size, material->materialDsets.capacity);

		if (shader.ptr) {
			for (uint32_t i = 0; i < shader.ptr->ubo_info_count; ++i) {
				auto& ubo_info = shader.ptr->p_ubo_infos[i];
				pulse_material_ubo_column_t col = {};
				col.set = ubo_info.set;
				col.binding = ubo_info.binding;
				col.size = ubo_info.size;
				if (ubo_info.material_managed) {
					col.cpu_data = (uint8_t*)calloc(1, ubo_info.size);
				}
				col.dirty = ubo_info.material_managed;
				col.gpu_buffer = nullptr;
				col.material_only = ubo_info.material_managed && !ubo_info.renderer_managed;
				simple_vector_push_back<pulse_material_ubo_column_t>(
					material->uboColumns.data,
					material->uboColumns.size,
					material->uboColumns.capacity,
					col);
			}

			for (uint32_t i = 0; i < shader.ptr->set_info_count; ++i) {
				auto& set_info = shader.ptr->p_set_infos[i];
				if (set_info.renderer_managed) continue;

				CGPUDescriptorSetDescriptor dset_desc = {};
				dset_desc.root_signature = shader.ptr->root_sig;
				dset_desc.set_index = set_info.set_index;
				auto dset_handle = cgpu_device_create_descriptor_set(device, &dset_desc);
				if (dset_handle) {
					pulse_material_descriptor_set_t mdset = {};
					mdset.set_index = set_info.set_index;
					mdset.handle = dset_handle;
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

	void free_material(PulseMaterialData* material)
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
		material->shader.ptr = nullptr;
		material->device = nullptr;
	}

	void material_mark_dset_binding_dirty(PulseMaterialData* material, uint32_t set_index)
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

	void material_bindTexture(PulseMaterialData* material, int set, int bind, PulseTextureData* texture)
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

	void material_bindSampler(PulseMaterialData* material, int set, int bind, PulseSamplerData* sampler)
	{
		material_bindSampler(material, set, bind, sampler->handle);
	}

	void material_bindSampler(PulseMaterialData* material, int set, int bind, CGPUSamplerId sampler)
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

	void material_bindBuffer(PulseMaterialData* material, int set, int bind, PulseGraphicsBufferData* buffer)
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

	pulse_material_ubo_column_t* material_find_ubo_column(PulseMaterialData* material, uint32_t set, uint32_t binding)
	{
		for (int i = 0; i < material->uboColumns.size; ++i)
		{
			auto& col = material->uboColumns.data[i];
			if (col.set == set && col.binding == binding)
				return &col;
		}
		return nullptr;
	}

	void material_ubo_sync_to_gpu(PulseMaterialData* material)
	{
		for (int i = 0; i < material->uboColumns.size; ++i)
		{
			auto& col = material->uboColumns.data[i];
			if (!col.dirty || !col.material_only) continue;
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

	const uint8_t* material_get_property_data(PulseMaterialData* material, uint32_t set, uint32_t binding, uint32_t offset, uint32_t size)
	{
		for (int i = 0; i < material->uboColumns.size; ++i)
		{
			auto& col = material->uboColumns.data[i];
			if (col.set == set && col.binding == binding && col.cpu_data && col.size >= offset + size)
				return col.cpu_data + offset;
		}
		return nullptr;
	}

	void material_sync_descriptor_sets(RenderPassEncoder* encoder, PulseMaterialData* material)
	{
		if (!material->shader.ptr) return;
		auto root_sig = material->shader.ptr->root_sig;

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
					if (!tex_view)
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
					if (!sampler)
						sampler = encoder->context->default_sampler;
					samplers[sampler_count] = sampler;
					data.resources.samplers = samplers + sampler_count;
					++sampler_count;
				}
				else if (res.type == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER || res.type == CGPU_RESOURCE_TYPE_RW_BUFFER)
				{
					static const uint64_t kMaterialZeroBase = 0; // material 路径 buffer offset 恒 0
					CGPUBufferId buffer = CGPU_NULLPTR;
					uint64_t range = 0; // buffer现在都是dynamic绑定，需要显式指定range
					// Search material buffers first
					for (int b = 0; b < material->buffers.size; ++b)
					{
						auto& bind = material->buffers.data[b];
						if ((uint32_t)bind.set == set_idx && (uint32_t)bind.bind == res.binding)
						{
							if (bind.buffer)
							{
								buffer = bind.buffer->handle;
								range = buffer->info->size;
							}
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
								range = col.size;
								break;
							}
						}
					}
					if (!buffer)
						buffer = encoder->context->default_buffer;
					else if (range > 0)
					{
						data.params.buffers_params.offsets = &kMaterialZeroBase;
						data.params.buffers_params.sizes = &range;
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

	void push_constants(RenderPassEncoder* encoder, PulseShaderData* shader, const char* name, const void* data)
	{
		cgpu_render_pass_encoder_push_constants(encoder->encoder, shader->root_sig, name, data);
	}

	void update_render_pipeline(RenderPassEncoder* encoder, PulseShaderData* shader, ECGPUPrimitiveTopology mesh_topology, const CGPUVertexLayout& vertex_layout)
	{
		auto pipeline = shader ? encoder->context->pipelinePool.getGraphicsPipeline(encoder, shader, mesh_topology, vertex_layout) : CGPU_NULLPTR;
        auto render_pipeline = pipeline ? pipeline->handle : CGPU_NULLPTR;
		if (render_pipeline && render_pipeline != encoder->last_render_pipeline)
		{
			cgpu_render_pass_encoder_bind_render_pipeline(encoder->encoder, render_pipeline);
			if (encoder->context->pipelinePool.dynamicStateT1Enabled())
			{
				cgpu_raster_state_encoder_set_cull_mode(encoder->raster_state_encoder, shader->rasterizer_state.cull_mode);
				cgpu_raster_state_encoder_set_front_face(encoder->raster_state_encoder, shader->rasterizer_state.front_face);
				cgpu_raster_state_encoder_set_depth_test_enabled(encoder->raster_state_encoder, shader->depth_desc.depth_test);
				cgpu_raster_state_encoder_set_depth_write_enabled(encoder->raster_state_encoder, shader->depth_desc.depth_write);
				cgpu_raster_state_encoder_set_depth_compare_op(encoder->raster_state_encoder, shader->depth_desc.depth_op);
			}
		}
		encoder->last_render_pipeline = render_pipeline;
		encoder->last_shader = shader;
		if (encoder->context->pipelinePool.dynamicStateT1Enabled() && mesh_topology != encoder->last_prim_topology)
		{
			cgpu_raster_state_encoder_set_primitive_topology(encoder->raster_state_encoder, mesh_topology);
			encoder->last_prim_topology = mesh_topology;
		}
	}

	void update_descriptor_set(RenderPassEncoder* encoder, CGPURootSignatureId root_sig, bool is_graphics)
	{
		PulseShaderData* shader = encoder->last_shader;
		for (uint32_t i = 0; i < std::min(4u, root_sig->table_count); ++i)
		{
			const auto& table = root_sig->p_tables[i];
			const uint32_t set_idx = table.set_index;

			if (shader)
			{
				const auto& set_info = shader->p_set_infos[i];
				assert(set_info.set_index == set_idx);
				if (!set_info.renderer_managed)
					continue;
			}

			ResourceSet& rs = encoder->resource_sets[set_idx];
			rs.set_index = set_idx;

			const uint32_t data_size = 64;
			const uint32_t res_count = std::min(data_size, table.resources_count);
			if (res_count == 0)
				continue;

			uintptr_t values[data_size] = {};
			uint32_t offsets[data_size] = {};
			uint32_t offset_count = 0;
			for (uint32_t j = 0; j < res_count; ++j)
			{
				const auto& res = table.p_resources[j];
				const ResourceSlot& slot = rs.slots[res.binding];
				values[j] = slot.value;
				if (res.type == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER || res.type == CGPU_RESOURCE_TYPE_RW_BUFFER)
					offsets[offset_count++] = (uint32_t)slot.offset;
			}

			uint64_t hash = murmur3((const uint32_t*)values, res_count * (sizeof(uintptr_t) / sizeof(uint32_t)), 0x9e3779b9u ^ set_idx);
			hash_combine(hash, root_sig);
			auto it = encoder->dset_cache.find(hash);
			CGPUDescriptorSetId dset = CGPU_NULLPTR;
			if (it != encoder->dset_cache.end() && it->second.count == res_count && memcmp(it->second.values, values, sizeof(uintptr_t) * res_count) == 0)
			{
				dset = it->second.handle;
			}
			else
			{
				CGPUDescriptorData datas[data_size] = {};
				uint32_t data_count = 0;
				uint32_t texture_view_count = 0;
				uint32_t sampler_count = 0;
				uint32_t buffer_count = 0;
				uint64_t buf_base[data_size] = {};  // descriptor base offset（恒 0，真实偏移走 bind dynamic offset）
				uint64_t buf_range[data_size] = {}; // descriptor range（DYNAMIC uniform 必须显式，不能 VK_WHOLE_SIZE）
				CGPUTextureViewId tex_views[data_size];
				CGPUSamplerId samplers[data_size];
				CGPUBufferId buffers[data_size];

				for (uint32_t j = 0; j < res_count; ++j)
				{
					auto& res = table.p_resources[j];
					CGPUDescriptorData data = {};
					data.binding = res.binding;
					data.binding_type = res.type;
					data.count = 1;

					const ResourceSlot& slot = rs.slots[res.binding];

					if (res.type == CGPU_RESOURCE_TYPE_TEXTURE)
					{
						CGPUTextureViewId textureview = (slot.kind == ResourceSlot::Kind::Texture && slot.value) ? (CGPUTextureViewId)slot.value : encoder->context->default_texture;
						tex_views[texture_view_count] = textureview;
						data.resources.textures = tex_views + texture_view_count;
						++texture_view_count;
					}
					else if (res.type == CGPU_RESOURCE_TYPE_SAMPLER)
					{
						CGPUSamplerId sampler = (slot.kind == ResourceSlot::Kind::Sampler && slot.value) ? (CGPUSamplerId)slot.value : encoder->context->default_sampler;
						samplers[sampler_count] = sampler;
						data.resources.samplers = samplers + sampler_count;
						++sampler_count;
					}
					else if (res.type == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER || res.type == CGPU_RESOURCE_TYPE_RW_BUFFER)
					{
						CGPUBufferId buffer = (slot.kind == ResourceSlot::Kind::Buffer && slot.value) ? (CGPUBufferId)slot.value : encoder->context->default_buffer;
						buf_base[buffer_count] = 0;
						buf_range[buffer_count] = (slot.size > 0) ? slot.size : (buffer ? buffer->info->size : 0);
						data.params.buffers_params.offsets = buf_base + buffer_count;
						data.params.buffers_params.sizes = buf_range + buffer_count;
						buffers[buffer_count] = buffer;
						data.resources.buffers = buffers + buffer_count;
						++buffer_count;
					}
					datas[data_count] = data;
					++data_count;
				}

				CGPUDescriptorSetDescriptor dset_desc =
				{
					.root_signature = root_sig,
					.set_index = set_idx,
				};
				auto pdset = encoder->context->descriptorSetPool.getDescriptorSet(dset_desc);
				encoder->context->allocated_dsets.push_back(pdset);
				cgpu_descriptor_set_update(pdset->handle, data_count, datas);
				dset = pdset->handle;

				SetCacheEntry entry;
				entry.count = res_count;
				memcpy(entry.values, values, sizeof(uintptr_t) * res_count);
				entry.handle = dset;
				encoder->dset_cache[hash] = entry;
			}

			bool bind_changed = (dset != encoder->last_bound_dset[set_idx]) ||
				(offset_count != encoder->last_bound_offset_count[set_idx]) ||
				(offset_count > 0 && memcmp(encoder->last_bound_offsets[set_idx], offsets, sizeof(uint32_t) * offset_count) != 0);
			if (bind_changed)
			{
				if (is_graphics)
					cgpu_render_pass_encoder_bind_descriptor_set(encoder->encoder, dset, offset_count, offset_count > 0 ? offsets : nullptr);
				else
					cgpu_compute_pass_encoder_bind_descriptor_set(encoder->compute_encoder, dset, offset_count, offset_count > 0 ? offsets : nullptr);
				encoder->last_bound_dset[set_idx] = dset;
				encoder->last_bound_offset_count[set_idx] = offset_count;
				if (offset_count > 0)
					memcpy(encoder->last_bound_offsets[set_idx], offsets, sizeof(uint32_t) * offset_count);
			}
		}
	}

	static void clear_material_slots(RenderPassEncoder* encoder)
	{
		for (uint32_t s = 0; s < 4; ++s)
		{
			auto& rs = encoder->resource_sets[s];
			for (uint32_t b = 0; b < 64; ++b)
			{
				if (rs.slots[b].from_material)
					rs.slots[b] = ResourceSlot{};
			}
		}
	}

	static void material_load_into_slots(RenderPassEncoder* encoder, PulseMaterialData* material)
	{
		for (int i = 0; i < material->textures.size; ++i)
		{
			auto& bind = material->textures.data[i];
			if (bind.set < 0 || bind.set >= 4 || bind.bind < 0 || bind.bind >= 64) continue;
			auto& rslot = encoder->resource_sets[bind.set].slots[bind.bind];
			rslot.kind = ResourceSlot::Kind::Texture;
			rslot.value = (bind.texture && bind.texture->view) ? (uintptr_t)bind.texture->view : 0;
			rslot.offset = 0;
			rslot.size = 0;
			rslot.from_material = true;
		}
		for (int i = 0; i < material->samplers.size; ++i)
		{
			auto& bind = material->samplers.data[i];
			if (bind.set < 0 || bind.set >= 4 || bind.bind < 0 || bind.bind >= 64) continue;
			auto& rslot = encoder->resource_sets[bind.set].slots[bind.bind];
			rslot.kind = ResourceSlot::Kind::Sampler;
			rslot.value = (uintptr_t)bind.sampler;
			rslot.offset = 0;
			rslot.size = 0;
			rslot.from_material = true;
		}
		for (int i = 0; i < material->buffers.size; ++i)
		{
			auto& bind = material->buffers.data[i];
			if (bind.set < 0 || bind.set >= 4 || bind.bind < 0 || bind.bind >= 64) continue;
			auto& rslot = encoder->resource_sets[bind.set].slots[bind.bind];
			rslot.kind = ResourceSlot::Kind::Buffer;
			rslot.value = bind.buffer ? (uintptr_t)bind.buffer->handle : 0;
			rslot.offset = 0;
			rslot.size = 0;
			rslot.from_material = true;
		}
		for (int i = 0; i < material->uboColumns.size; ++i)
		{
			auto& col = material->uboColumns.data[i];
			if (col.set >= 4 || col.binding >= 64 || !col.gpu_buffer) continue;
			auto& rslot = encoder->resource_sets[col.set].slots[col.binding];
			rslot.kind = ResourceSlot::Kind::Buffer;
			rslot.value = (uintptr_t)col.gpu_buffer->handle;
			rslot.offset = 0;
			rslot.size = 0;
			rslot.from_material = true;
		}
	}

	void update_material(RenderPassEncoder* encoder, PulseMaterialData* material)
	{
        if (material)
        {
            material_ubo_sync_to_gpu(material);
            material_sync_descriptor_sets(encoder, material);
        }

		if (encoder->last_material != material)
		{
			clear_material_slots(encoder);

            if (material)
            {
			    PulseShaderData* shader = material->shader.ptr;
			    if (shader)
			    {
				    material_load_into_slots(encoder, material);

				    for (size_t m = 0; m < material->materialDsets.size; ++m)
				    {
					    auto& mdset = material->materialDsets.data[m];
					    uint32_t dyn = 0;
					    for (uint32_t t = 0; t < shader->root_sig->table_count; ++t)
					    {
						    if (shader->root_sig->p_tables[t].set_index != mdset.set_index) continue;
						    for (uint32_t r = 0; r < shader->root_sig->p_tables[t].resources_count; ++r)
						    {
							    auto type = shader->root_sig->p_tables[t].p_resources[r].type;
							    if (type == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER || type == CGPU_RESOURCE_TYPE_RW_BUFFER)
								    dyn += shader->root_sig->p_tables[t].p_resources[r].count;
						    }
					    }
					    uint32_t zoffsets[64] = {};
					    cgpu_render_pass_encoder_bind_descriptor_set(encoder->encoder, mdset.handle, dyn, dyn > 0 ? zoffsets : nullptr);
				    }
			    }
            }

			encoder->last_material = material;
		}
	}

	void update_mesh(RenderPassEncoder* encoder, PulseMeshData* mesh)
	{
		CGPUBufferId vertex_buffer = CGPU_NULLPTR;
		if (mesh->vertex_buffer.ptr)
		{
			if (pulse_rgbuffer_handle_is_valid(mesh->vertex_buffer.ptr->dynamic_handle))
			{
				auto vertex_buffer_handle = mesh->vertex_buffer.ptr->dynamic_handle;
				vertex_buffer = pulse_render_pass_encoder_resolve_buffer((PulseRenderPassEncoder*)encoder, vertex_buffer_handle);
			}
			else
			{
				vertex_buffer = mesh->vertex_buffer.ptr->handle;
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
		if (mesh->index_buffer.ptr)
		{
			if (pulse_rgbuffer_handle_is_valid(mesh->index_buffer.ptr->dynamic_handle))
			{
				auto index_buffer_handle = mesh->index_buffer.ptr->dynamic_handle;
				index_buffer = pulse_render_pass_encoder_resolve_buffer((PulseRenderPassEncoder*)encoder, index_buffer_handle);
			}
			else
			{
				index_buffer = mesh->index_buffer.ptr->handle;
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

	void draw(RenderPassEncoder* encoder, PulseShaderData* shader, PulseMeshData* mesh)
	{
		if (!mesh->prepared)
			return;
		update_render_pipeline(encoder, shader, mesh->prim_topology, mesh->vertex_layout);
		update_material(encoder, nullptr);
		update_descriptor_set(encoder, shader->root_sig, true);
		update_mesh(encoder, mesh);
		if (encoder->last_index_buffer)
			cgpu_render_pass_encoder_draw_indexed(encoder->encoder, mesh->index_count, 0, 0);
		else
			cgpu_render_pass_encoder_draw(encoder->encoder, mesh->vertices_count, 0);
	}

	void draw_submesh(RenderPassEncoder* encoder, PulseShaderData* shader, PulseMeshData* mesh, uint32_t index_count, uint32_t first_index, uint32_t vertex_count, uint32_t first_vertex)
	{
		if (!mesh->prepared)
			return;
        update_render_pipeline(encoder, shader, mesh->prim_topology, mesh->vertex_layout);
        update_material(encoder, nullptr);
		update_descriptor_set(encoder, shader->root_sig, true);
		update_mesh(encoder, mesh);
		if (encoder->last_index_buffer)
			cgpu_render_pass_encoder_draw_indexed(encoder->encoder, index_count, first_index, first_vertex);
		else
			cgpu_render_pass_encoder_draw(encoder->encoder, vertex_count, first_vertex);
	}

	static CGPUVertexLayout procedure_vertex_layout = { .attribute_count = 0 };
	void draw_procedure(RenderPassEncoder* encoder, PulseShaderData* shader, ECGPUPrimitiveTopology mesh_topology, uint32_t vertex_count)
	{
        update_render_pipeline(encoder, shader, mesh_topology, procedure_vertex_layout);
        update_material(encoder, nullptr);
		update_descriptor_set(encoder, shader->root_sig, true);
		cgpu_render_pass_encoder_draw(encoder->encoder, vertex_count, 0);
	}

	void draw(RenderPassEncoder* encoder, PulseMaterialData* material, PulseMeshData* mesh)
	{
		if (!mesh->prepared || !material)
			return;
		auto shader = material->shader.ptr;
		update_render_pipeline(encoder, shader, mesh->prim_topology, mesh->vertex_layout);
		update_material(encoder, material);
		update_descriptor_set(encoder, shader->root_sig, true);
		update_mesh(encoder, mesh);
		if (encoder->last_index_buffer)
			cgpu_render_pass_encoder_draw_indexed(encoder->encoder, mesh->index_count, 0, 0);
		else
			cgpu_render_pass_encoder_draw(encoder->encoder, mesh->vertices_count, 0);
	}

	void draw_submesh(RenderPassEncoder* encoder, PulseMaterialData* material, PulseMeshData* mesh, uint32_t index_count, uint32_t first_index, uint32_t vertex_count, uint32_t first_vertex)
	{
		if (!mesh->prepared || !material)
			return;
		auto shader = material->shader.ptr;
		update_render_pipeline(encoder, shader, mesh->prim_topology, mesh->vertex_layout);
		update_material(encoder, material);
		update_descriptor_set(encoder, shader->root_sig, true);
		update_mesh(encoder, mesh);
		if (encoder->last_index_buffer)
			cgpu_render_pass_encoder_draw_indexed(encoder->encoder, index_count, first_index, first_vertex);
		else
			cgpu_render_pass_encoder_draw(encoder->encoder, vertex_count, first_vertex);
	}

	void draw_procedure(RenderPassEncoder* encoder, PulseMaterialData* material, ECGPUPrimitiveTopology mesh_topology, uint32_t vertex_count)
	{
		if (!material)
			return;
		auto shader = material->shader.ptr;
		update_render_pipeline(encoder, shader, mesh_topology, procedure_vertex_layout);
		update_material(encoder, material);
		update_descriptor_set(encoder, shader->root_sig, true);
		cgpu_render_pass_encoder_draw(encoder->encoder, vertex_count, 0);
	}

	void update_compute_pipeline(RenderPassEncoder* encoder, PulseComputeShaderData* shader)
	{
		auto pipeline = encoder->context->computePipelinePool.getComputePipeline(shader);
		if (pipeline && pipeline->handle != encoder->last_compute_pipeline)
		{
			cgpu_compute_pass_encoder_bind_compute_pipeline(encoder->compute_encoder, pipeline->handle);
			encoder->last_compute_pipeline = pipeline->handle;
		}
	}

	// render pass和compute pass不会混用draw和dispatch
	void dispatch(RenderPassEncoder* encoder, PulseComputeShaderData* shader, uint32_t thread_x, uint32_t thread_y, uint32_t thread_z)
	{
		encoder->last_material = nullptr;
		encoder->last_shader = nullptr;
        clear_material_slots(encoder);
		update_compute_pipeline(encoder, shader);
		update_descriptor_set(encoder, shader->root_sig, false);
		cgpu_compute_pass_encoder_dispatch(encoder->compute_encoder, thread_x, thread_y, thread_z);
	}

	static inline bool set_global_slot_bounds(RenderPassEncoder* encoder, int set, int slot, ResourceSlot*& out_slot)
	{
		if (set < 0 || set >= 4 || slot < 0 || slot >= 64)
			return false;
		auto out_rs = &encoder->resource_sets[set];
		out_slot = &out_rs->slots[slot];
		return true;
	}

	void set_global_texture(RenderPassEncoder* encoder, PulseTextureData* texture, int set, int slot)
	{
		ResourceSlot* rslot;
		if (!set_global_slot_bounds(encoder, set, slot, rslot)) return;
		if (rslot->from_material) return;
		rslot->kind = ResourceSlot::Kind::Texture;
		rslot->value = (texture && texture->view) ? (uintptr_t)texture->view : 0;
		rslot->offset = 0;
		rslot->size = 0;
		rslot->from_material = false;
	}

	void set_global_texture_handle(RenderPassEncoder* encoder, PulseRGTextureHandle texture, int set, int slot)
	{
		ResourceSlot* rslot;
		if (!set_global_slot_bounds(encoder, set, slot, rslot)) return;
		if (rslot->from_material) return;
		if (!pulse_rgtexture_handle_is_valid(texture)) return;
		rslot->kind = ResourceSlot::Kind::Texture;
		rslot->value = (uintptr_t)pulse_render_pass_encoder_resolve_texture_view((PulseRenderPassEncoder*)encoder, texture);
		rslot->offset = 0;
		rslot->size = 0;
		rslot->from_material = false;
	}

	void set_global_sampler(RenderPassEncoder* encoder, CGPUSamplerId sampler, int set, int slot)
	{
		ResourceSlot* rslot;
		if (!set_global_slot_bounds(encoder, set, slot, rslot)) return;
		if (rslot->from_material) return;
		rslot->kind = ResourceSlot::Kind::Sampler;
		rslot->value = (uintptr_t)sampler;
		rslot->offset = 0;
		rslot->size = 0;
		rslot->from_material = false;
	}

	void set_global_buffer(RenderPassEncoder* encoder, PulseGraphicsBufferData* buffer, int set, int slot)
	{
		ResourceSlot* rslot;
		if (!set_global_slot_bounds(encoder, set, slot, rslot)) return;
		if (rslot->from_material) return;
		rslot->kind = ResourceSlot::Kind::Buffer;
		rslot->value = buffer ? (uintptr_t)buffer->handle : 0;
		rslot->offset = 0;
		rslot->size = 0;
		rslot->from_material = false;
	}

	void set_global_dynamic_buffer(RenderPassEncoder* encoder, PulseRGBufferHandle buffer, int set, int slot)
	{
		ResourceSlot* rslot;
		if (!set_global_slot_bounds(encoder, set, slot, rslot)) return;
		if (rslot->from_material) return;
		if (!pulse_rgbuffer_handle_is_valid(buffer)) return;
		rslot->kind = ResourceSlot::Kind::Buffer;
		rslot->value = (uintptr_t)pulse_render_pass_encoder_resolve_buffer((PulseRenderPassEncoder*)encoder, buffer);
		rslot->offset = 0;
		rslot->size = 0;
		rslot->from_material = false;
	}

	void set_global_buffer_with_offset_size(RenderPassEncoder* encoder, PulseRGBufferHandle buffer, int set, int slot, uint64_t offset, uint64_t size)
	{
		ResourceSlot* rslot;
		if (!set_global_slot_bounds(encoder, set, slot, rslot)) return;
		if (rslot->from_material) return;
		if (!pulse_rgbuffer_handle_is_valid(buffer)) return;
		rslot->kind = ResourceSlot::Kind::Buffer;
		rslot->value = (uintptr_t)pulse_render_pass_encoder_resolve_buffer((PulseRenderPassEncoder*)encoder, buffer);
		rslot->offset = offset;
		rslot->size = size;
		rslot->from_material = false;
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
