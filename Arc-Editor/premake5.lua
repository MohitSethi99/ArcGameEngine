project "Arc-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"
	warnings "extra"
	externalwarnings "off"
	rtti "off"
	buildmessage "================ Copying Resource ================"
	postbuildmessage "================ Post-Build: Copying other dependencies ================"

	flags { "FatalWarnings" }

	binDir = "%{wks.location}/bin/" .. outputdir
	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp",
	}

	includedirs
	{
		"src",
		"%{wks.location}/Arc/src",
		"%{wks.location}/Arc/vendor"
	}

	externalincludedirs
	{
		"%{wks.location}/Arc/vendor/spdlog/include",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.optick}",
		"%{IncludeDir.icons}",
	}
	
	links
	{
		"Arc",
		"GLFW",
		"Glad",
		"ImGui",
		"yaml-cpp",
		"optick",
		"box2d",
		"JoltPhysics",
		"Arc-ScriptCore",
	}

	buildcommands
	{
		'{ECHO} ***** Copying Resource Files *****',
		'{COPYDIR} "../vendor" "%{binDir}"/vendor',
		'{COPYDIR} "assets" "%{cfg.targetdir}"/assets',
		'{COPYDIR} "Resources" "%{cfg.targetdir}"/Resources',
		'{COPYFILE} "imgui.ini" "%{cfg.targetdir}"',
	}
	
	buildoutputs
	{
		"%{cfg.targetdir}/assets",
		"%{cfg.targetdir}/Resources",
	}

	postbuildcommands
	{
		-- Mono
		'{ECHO} ***** Copying Mono *****',
		'{COPYDIR} "mono" "%{cfg.targetdir}"/mono',
	}

	filter "system:windows"
		systemversion "latest"
		links
		{
			"%{LibDir.Mono}/mono-2.0-sgen.lib",
			"opengl.dll",

			-- DirectX
			"dxguid.lib",
			"d3d12.lib",
			"dxgi.lib",
		}

	filter "system:linux"
		pic "On"
		systemversion "latest"
		linkoptions { "`pkg-config --libs gtk+-3.0`" }
		links
		{
			"monosgen-2.0:shared",
			"GL:shared",
			"dl:shared"
		}

	filter "configurations:Debug"
		defines "ARC_DEBUG"
		runtime "Debug"
		symbols "on"
		postbuildcommands
		{
			'{COPYFILE} "%{BinDir.Mono}/mono-2.0-sgen.dll" "%{cfg.targetdir}"',
		}

	filter "configurations:Release"
		defines "ARC_RELEASE"
		runtime "Release"
		optimize "speed"
		postbuildcommands
		{
			'{COPYFILE} "%{BinDir.Mono}/mono-2.0-sgen.dll" "%{cfg.targetdir}"',
		}

	filter "configurations:Dist"
		defines "ARC_DIST"
		runtime "Release"
        optimize "speed"
		symbols "off"
		postbuildcommands
		{
			'{COPYFILE} "%{BinDir.Mono}/mono-2.0-sgen.dll" "%{cfg.targetdir}"',
		}
