workspace "ClangMethodAnalyzer"
    configurations { "Debug", "Release" }
    location "build"
    toolset "clang"
    
    project "method-analyzer"
        kind "ConsoleApp"
        language "C++"
        
        files { "source/**.cpp" }
        targetdir "bin/%{cfg.buildcfg}"
        
        cppdialect "C++17"
        
        buildoptions { "`llvm-config --cxxflags`", "-std=c++17"}
        linkoptions { "`llvm-config --ldflags`" }

        includedirs {
            "include"
        }
        
        links { 
            "clangTooling",
            "clangFrontend",
            "clangSerialization",
            "clangDriver",
            "clangParse",
            "clangSema",
            "clangAnalysis",
            "clangEdit",
            "clangAST",
            "clangLex",
            "clangBasic",
            "LLVMCore",
            "LLVMSupport"
        }
        
        linkoptions { "`llvm-config --libs --system-libs`" }
        
        filter "configurations:Debug"
            defines { "DEBUG" }
            symbols "On"
            
        filter "configurations:Release"
            defines { "NDEBUG" }
            optimize "On"
