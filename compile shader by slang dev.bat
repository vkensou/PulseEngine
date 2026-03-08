mkdir "examples\assets\shaderbin"

.\cgpu\tools\slang\slangc examples\rendersystem\obj2.slang -profile sm_5_0 -capability SPIRV_1_3 -entry vert -o examples\assets\shaderbin\obj2.vert.spv -O0 -g3 -line-directive-mode none -emit-spirv-directly -matrix-layout-row-major
.\cgpu\tools\slang\slangc examples\rendersystem\obj2.slang -profile sm_5_0 -capability SPIRV_1_3 -entry frag -o examples\assets\shaderbin\obj2.frag.spv -O0 -g3 -line-directive-mode none -emit-spirv-directly -matrix-layout-row-major
