set_project("summit")
set_version("1.0.0")

set_languages("c++17")
set_warnings("none")

add_rules("mode.release")

if is_plat("linux") then
    target("summit")
        set_kind("binary")
        add_files("src/**.cpp")
        add_includedirs("src", "include", "include/codegen", "include/ast", "include/utils", "include/lexer", "include/parser")
        
        on_load(function (target)
            local llvm_cxxflags = os.iorun("llvm-config --cxxflags")
            local llvm_ldflags = os.iorun("llvm-config --ldflags --system-libs --libs all")
            
            if llvm_cxxflags and llvm_ldflags then
                for flag in llvm_cxxflags:gmatch("%S+") do
                    if flag:startswith("-I") then
                        target:add("includedirs", flag:sub(3))
                    else
                        target:add("cxxflags", flag)
                    end
                end
                
                for flag in llvm_ldflags:gmatch("%S+") do
                    if flag:startswith("-L") then
                        target:add("linkdirs", flag:sub(3))
                    elseif flag:startswith("-l") then
                        target:add("links", flag:sub(3))
                    else
                        target:add("ldflags", flag)
                    end
                end
            else
                target:add("links", "LLVM-18")
            end
        end)
        
        add_cxflags(
            "-fdata-sections",
            "-ffunction-sections", 
            "-DNDEBUG",
            "-fmerge-all-constants",
            "-fno-stack-protector",
            "-fno-math-errno",
            "-fno-ident",
            "-fexceptions",
            "-flto"
        )
        
        add_ldflags(
            "-Wl,--gc-sections",
            "-Wl,--as-needed", 
            "-Wl,--strip-all",
            "-s",
            "-flto"
        )
        add_syslinks("tommath")

else if is_plat("windows") then
    set_toolchains("mingw")
    
    target("summit")
        set_kind("binary")
        add_files("src/**.cpp")
        add_includedirs("src", "include", "include/codegen", "include/ast", "include/utils", "include/lexer", "include/parser")
        
        on_load(function (target)
            local llvm_cxxflags = os.iorun("llvm-config --cxxflags")
            local llvm_ldflags = os.iorun("llvm-config --ldflags --system-libs --libs all")
            
            if llvm_cxxflags and llvm_ldflags then
                for flag in llvm_cxxflags:gmatch("%S+") do
                    if flag:startswith("-I") then
                        target:add("includedirs", flag:sub(3))
                    else
                        target:add("cxxflags", flag)
                    end
                end

                for flag in llvm_ldflags:gmatch("%S+") do
                    if flag:startswith("-L") then
                        target:add("linkdirs", flag:sub(3))
                    elseif flag:startswith("-l") then
                        target:add("links", flag:sub(3))
                    else
                        target:add("ldflags", flag)
                    end
                end
            else
                target:add("includedirs", "/ucrt64/include")
                target:add("linkdirs", "/ucrt64/lib")
                target:add("links", "LLVM")
            end
        end)
        
        add_cxflags(
            "-fdata-sections",
            "-ffunction-sections", 
            "-DNDEBUG",
            "-fmerge-all-constants",
            "-fno-stack-protector",
            "-fno-math-errno",
            "-fno-ident",
            "-fexceptions",
            "-D__USE_MINGW_ANSI_STDIO=1"
        )
        
        add_ldflags(
            "-Wl,--gc-sections",
            "-Wl,--as-needed", 
            "-Wl,--strip-all",
            "-s"
        )
        
        add_syslinks("tommath")

else if is_plat("windows") and is_arch("arm64") then
    set_toolchains("msvc")
    
    target("summit")
        set_kind("binary")
        add_files("src/**.cpp")
        add_includedirs("src", "include", "include/codegen", "include/ast", "include/utils", "include/lexer", "include/parser")

        on_load(function (target)
            local llvm_paths = {
                os.getenv("LLVM_INSTALL_DIR"),
                "C:/Program Files/LLVM",
                "C:/LLVM"
            }
            
            for _, path in ipairs(llvm_paths) do
                if path and os.isdir(path) then
                    local include_dir = path .. "/include"
                    local lib_dir = path .. "/lib/ARM64"
                    
                    if os.isdir(include_dir) and os.isdir(lib_dir) then
                        target:add("includedirs", include_dir)
                        target:add("linkdirs", lib_dir)
                        target:add("links", "LLVM")
                        break
                    end
                end
            end
            
            target:add("syslinks", "tommath")
        end)
        
        add_cxflags(
            "-DNDEBUG",
            "-DWIN32",
            "-D_WINDOWS"
        )
end
end
end