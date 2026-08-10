#include "asset_internal.h"

namespace pulse::asset {

bool is_power_of_two_alignment(uint32_t align) {
    return align != 0 && (align & (align - 1)) == 0;
}

AssetLoader::AssetLoader(std::pmr::memory_resource* resource)
    : extensions(resource) {
}

bool AssetLoader::is_builder() const {
    return extensions.empty();
}

AssetType::AssetType(std::pmr::memory_resource* resource)
    : loaders(resource),
      extension_loaders(resource) {
}

bool AssetType::has_loader_for_any(const std::pmr::vector<std::pmr::string>& extension_list) const {
    for (const std::pmr::string& extension : extension_list) {
        if (extension_loaders.find(extension) != extension_loaders.end()) {
            return true;
        }
    }
    return false;
}

AssetLoader* AssetType::find_builder_loader() {
    std::pmr::string builder_key;
    auto loader_it = extension_loaders.find(builder_key);
    return loader_it != extension_loaders.end() ? loader_it->second : nullptr;
}

AssetLoader* AssetType::find_extension_loader(const std::pmr::string& extension) {
    auto loader_it = extension_loaders.find(extension);
    return loader_it != extension_loaders.end() ? loader_it->second : nullptr;
}

EPulseResult AssetType::add_loader(
    const PulseAssetLoaderDesc& loader_desc,
    std::pmr::vector<std::pmr::string>&& extension_list,
    std::pmr::memory_resource* resource
) {
    if (extension_list.empty()) {
        if (find_builder_loader()) {
            return PULSE_RESULT_ERROR_INVALID_STATE;
        }
    } else if (has_loader_for_any(extension_list)) {
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    extension_loaders.reserve(extension_loaders.size() + (extension_list.empty() ? 1u : extension_list.size()));
    AssetLoader loader(resource);
    loader.desc = loader_desc;
    loader.extensions = std::move(extension_list);
    loaders.push_back(std::move(loader));

    AssetLoader* registered_loader = &loaders.back();
    if (registered_loader->is_builder()) {
        extension_loaders.emplace(std::pmr::string(resource), registered_loader);
    } else {
        for (const std::pmr::string& extension : registered_loader->extensions) {
            extension_loaders.emplace(std::pmr::string(extension, resource), registered_loader);
        }
    }
    return PULSE_RESULT_OK;
}

AssetRegistry::AssetRegistry(std::pmr::memory_resource* resource)
    : resource_(resource),
      types_(resource) {
}

EPulseResult AssetRegistry::register_type(const PulseAssetTypeDesc* desc) {
    if (!desc || !desc->type_id || desc->struct_size != sizeof(PulseAssetTypeDesc) ||
        desc->version != PULSE_ASSET_TYPE_DESC_VERSION ||
        desc->size == 0 || !is_power_of_two_alignment(desc->align)) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto [type_it, inserted] = types_.try_emplace(desc->type_id, resource_);
    if (!inserted) {
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    type_it->second.desc = *desc;
    return PULSE_RESULT_OK;
}

EPulseResult AssetRegistry::register_loader(const PulseAssetLoaderDesc* desc) {
    if (!desc || desc->struct_size != sizeof(PulseAssetLoaderDesc) ||
        desc->version != PULSE_ASSET_LOADER_DESC_VERSION || desc->type_id == 0 ||
        !desc->step ||
        (desc->loader_size > 0 && !is_power_of_two_alignment(desc->loader_align)) ||
        (desc->settings_size > 0 && !is_power_of_two_alignment(desc->settings_align)) ||
        (desc->settings_size == 0 && (desc->settings_size_fn || desc->settings_copy_fn)) ||
        ((desc->settings_size_fn == nullptr) != (desc->settings_copy_fn == nullptr))) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    AssetType* type = find_type(desc->type_id);
    if (!type) {
        return PULSE_RESULT_ERROR_NOT_FOUND;
    }

    std::pmr::vector<std::pmr::string> extension_list = AssetIo::parse_extensions(desc->extensions, resource_);
    return type->add_loader(*desc, std::move(extension_list), resource_);
}

AssetType* AssetRegistry::find_type(uint64_t type_id) {
    auto type_it = types_.find(type_id);
    return type_it != types_.end() ? &type_it->second : nullptr;
}

AssetLoader* AssetRegistry::find_loader(uint64_t type_id, const std::pmr::string& path) {
    std::pmr::string extension = AssetIo::extension_from_path(path, resource_);
    if (extension.empty()) {
        return nullptr;
    }

    AssetType* type = find_type(type_id);
    return type ? type->find_extension_loader(extension) : nullptr;
}

AssetLoader* AssetRegistry::find_builder_loader(uint64_t type_id) {
    AssetType* type = find_type(type_id);
    return type ? type->find_builder_loader() : nullptr;
}

} // namespace pulse::asset