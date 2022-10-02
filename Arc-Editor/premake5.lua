project "Arc-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp",

		"%{IncludeDir.EASTL}/EASTL.natvis",
		"%{IncludeDir.glm}/util/glm.natvis",
		"%{IncludeDir.yaml_cpp}/../src/contrib/yaml-cpp.natvis",
		"%{IncludeDir.ImGui}/misc/debuggers/imgui.natvis",

		"%{IncludeDir.entt}/../natvis/config.natvis",
		"%{IncludeDir.entt}/../natvis/container.natvis",
		"%{IncludeDir.entt}/../natvis/core.natvis",
		"%{IncludeDir.entt}/../natvis/entity.natvis",
		"%{IncludeDir.entt}/../natvis/graph.natvis",
		"%{IncludeDir.entt}/../natvis/locator.natvis",
		"%{IncludeDir.entt}/../natvis/meta.natvis",
		"%{IncludeDir.entt}/../natvis/platform.natvis",
		"%{IncludeDir.entt}/../natvis/poly.natvis",
		"%{IncludeDir.entt}/../natvis/process.natvis",
		"%{IncludeDir.entt}/../natvis/resource.natvis",
		"%{IncludeDir.entt}/../natvis/signal.natvis",
	}

	includedirs
	{
		"%{wks.location}/Arc/vendor/spdlog/include",
		"%{wks.location}/Arc/src",
		"%{wks.location}/Arc/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.assimp_config}",
		"%{IncludeDir.assimp_config_assimp}",
		"%{IncludeDir.optick}",
		"%{IncludeDir.EABase}",
		"%{IncludeDir.EASTL}",
		"%{IncludeDir.miniaudio}",
		"%{IncludeDir.icons}",
	}

	links
	{
		"Arc",
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "ARC_DEBUG"
		runtime "Debug"
		symbols "on"
		postbuildcommands
		{
			'{COPY} "../Arc/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}

	filter "configurations:Release"
		defines "ARC_RELEASE"
		runtime "Release"
		optimize "on"
		postbuildcommands
		{
			'{COPY} "../Arc/vendor/mono/bin/Release/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}

	filter "configurations:Dist"
		defines "ARC_DIST"
		runtime "Release"
        optimize "on"
		postbuildcommands
		{
			'{COPY} "../Arc/vendor/mono/bin/Release/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}
