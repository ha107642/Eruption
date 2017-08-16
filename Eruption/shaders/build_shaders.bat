for %%f in (C:\Users\Henrik\Dropbox\Projekt\Eruption\Eruption\shaders\*.frag) do (
	START /WAIT C:\VulkanSDK\1.0.39.1\Bin32\glslangValidator.exe -V %%f 
	copy C:\Users\Henrik\Dropbox\Projekt\Eruption\Eruption\shaders\frag.spv C:\Users\Henrik\Dropbox\Projekt\Eruption\Debug\
)

for %%f in (C:\Users\Henrik\Dropbox\Projekt\Eruption\Eruption\shaders\*.vert) do (
	START /WAIT C:\VulkanSDK\1.0.39.1\Bin32\glslangValidator.exe -V %%f 
	copy C:\Users\Henrik\Dropbox\Projekt\Eruption\Eruption\shaders\vert.spv C:\Users\Henrik\Dropbox\Projekt\Eruption\Debug\
)