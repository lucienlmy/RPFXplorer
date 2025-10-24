require('vstudio')  
  
premake.override(premake.vstudio.vc2010.elements, "globals", function(base, prj)  
    local calls = base(prj)  
    table.insertafter(calls, premake.vstudio.vc2010.targetFramework, function(prj)  
        premake.vstudio.vc2010.element("VcpkgEnabled", nil, "true")  
        premake.vstudio.vc2010.element("VcpkgTriplet", nil, "x64-windows")  
        premake.vstudio.vc2010.element("VcpkgEnableManifest", nil, "true")  
        -- 移除或设置为 false,使用动态链接  
        premake.vstudio.vc2010.element("VcpkgUseStatic", nil, "false")  
    end)  
    return calls  
end)