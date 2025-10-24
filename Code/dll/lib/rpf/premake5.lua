project "rpf"  
    kind "SharedLib"  
    defines { "__RPF" }  
  
    pchheader "pch.h"  
    pchsource "pch.cpp"  
  
    -- 添加当前目录作为包含路径  
    includedirs { "." }  
  
    files {  
        "**.h",  
        "**.cpp"  
    }  
  
    -- 添加 Windows 系统库链接  
    links { "crypt32", "ws2_32" }