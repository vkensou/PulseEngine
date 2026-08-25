@echo off
rem Compile the snake example shader (color.slang -> color.vert.spv / color.frag.spv)
rem Run from the repository root.

mkdir assets 2>nul

..\..\cgpu\tools\slang\slangc assets\color.slang -profile sm_5_0 -capability SPIRV_1_3 -entry vert -o assets\color.vert.spv -O3 -emit-spirv-directly -matrix-layout-row-major
..\..\cgpu\tools\slang\slangc assets\color.slang -profile sm_5_0 -capability SPIRV_1_3 -entry frag -o assets\color.frag.spv -O3 -emit-spirv-directly -matrix-layout-row-major

echo Done.
