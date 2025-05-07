workspace "ClangMethodAnalyzer"
    configurations { "Debug", "Release" }
    location "build"
    toolset "clang"
    
    project "method-analyzer"
        kind "ConsoleApp"
        language "C++"
        
        files { "main.cpp"}
        targetdir "bin/%{cfg.buildcfg}"
        
        cppdialect "C++20"
        
        buildoptions { "`llvm-config --cxxflags`" }
        linkoptions { "`llvm-config --ldflags`" }
        
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
