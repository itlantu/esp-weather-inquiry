# 从.env文件中读取环境变量到cmake
function(function_env_read env_file_path)
    # 将路径转为绝对路径
    get_filename_component(abs_env_file_path "${env_file_path}" ABSOLUTE)

    if(NOT EXISTS "${abs_env_file_path}")
        message(WARNING ".env文件不存在, 目标路径:\"${abs_env_file_path}\"")
        return()
    endif()

    file(STRINGS "${abs_env_file_path}" env_lines)

    foreach(line ${env_lines})
        # 跳过空行和注释
        string(STRIP ${line} line)

        if(line STREQUAL "" OR line MATCHES "^#")
            continue()
        endif()

        # 分割 KEY=VALUE（只按第一个'='分割）
        string(REGEX MATCH "^([^=]+)=(.*)$" match ${line})

        if(NOT match)
            continue()
        endif()

        set(key ${CMAKE_MATCH_1})
        if(NOT DEFINED ENV{${key}})
            set(value ${CMAKE_MATCH_2})
            set(ENV{${key}} "${value}")
            message(STATUS " [+]${key}=${value}")
        endif()
    endforeach()
    message(STATUS "-----------")
endfunction()

# 将环境变量的值加载到宏中
function(fucntion_target_env2marco target env_name)
    if(NOT DEFINED ENV{${env_name}})
        message(WARNING " ${env_name}未配置")
        return()
    endif()

    target_compile_definitions(${target} PRIVATE ${env_name}=$ENV{${env_name}})
    message(STATUS " [*]${env_name}=$ENV{${env_name}}")
endfunction()
