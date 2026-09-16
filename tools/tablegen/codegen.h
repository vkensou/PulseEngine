#pragma once

#include "schema.h"
#include "validate.h"

namespace tablegen {

bool generate(const Library& library, const LayoutSet& layouts, const std::string& header_path, const std::string& source_path, const std::string& header_name, const std::string& das_path, const std::string& das_module, std::vector<std::string>& errors);

} // namespace tablegen
