.PHONY: shaders

shaders:
	glslc shader-code/shader.vert -o shaders/vert.spv
	glslc shader-code/shader.frag  -o shaders/frag.spv
	glslc shader-code/shader.comp -o shaders/comp.spv
	glslc shader-code/particleShader.vert -o shaders/partVert.spv
	glslc shader-code/particleShader.frag -o shaders/partFrag.spv
