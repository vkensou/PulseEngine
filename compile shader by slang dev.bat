mkdir "legacy\examples\assets\shaderbin"

.\cgpu\tools\slang\slangc legacy\examples\rendersystem\obj2.slang -profile sm_5_0 -capability SPIRV_1_3 -entry vert -o legacy\examples\assets\shaderbin\obj2.vert.spv -O0 -g3 -line-directive-mode none -emit-spirv-directly -matrix-layout-row-major
.\cgpu\tools\slang\slangc legacy\examples\rendersystem\obj2.slang -profile sm_5_0 -capability SPIRV_1_3 -entry frag -o legacy\examples\assets\shaderbin\obj2.frag.spv -O0 -g3 -line-directive-mode none -emit-spirv-directly -matrix-layout-row-major

.\cgpu\tools\slang\slangc legacy\examples\snake\color.slang -profile sm_5_0 -capability SPIRV_1_3 -entry vert -o legacy\examples\assets\shaderbin\color.vert.spv -O0 -g3 -line-directive-mode none -emit-spirv-directly -matrix-layout-row-major
.\cgpu\tools\slang\slangc legacy\examples\snake\color.slang -profile sm_5_0 -capability SPIRV_1_3 -entry frag -o legacy\examples\assets\shaderbin\color.frag.spv -O0 -g3 -line-directive-mode none -emit-spirv-directly -matrix-layout-row-major
