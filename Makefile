CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: Main.cpp
	g++ $(CFLAGS) ate a task.json-o VulkanTest Main.cpp $(LDFLAGS)

fast: Main.cpp
	g++ -std=c++17 -o VulkanTest Main.cpp $(LDFLAGS)

.PHONY: test clean shaders

test: fast
	./VulkanTest

clean:
	rm -f VulkanTest

shaders:
	glslc shader-code/shader.vert -o shaders/vert.spv
	glslc shader-code/shader.frag  -o shaders/frag.spv
	glslc shader-code/shader.comp -o shaders/comp.spv
	glslc shader-code/particleShader.vert -o shaders/partVert.spv
	glslc shader-code/particleShader.frag -o shaders/partFrag.spv
