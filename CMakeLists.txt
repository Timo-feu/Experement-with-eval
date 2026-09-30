cmake_minimum_required(VERSION 3.15)
project(SoulEngine VERSION 2.0.0 LANGUAGES CXX C)

# ==============================================================================
# 1. ФИКСАЦИЯ СТАНДАРТОВ ЯЗЫКА
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ==============================================================================
# 2. ЖЕСТКАЯ ОПТИМИЗАЦИЯ БЕЗ LTO
# ==============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # УБРАЛИ -flto, чтобы w64devkit не ругался.
    # Оставили -O3 (скорость), -s (стриппинг символов для размера), 
    # -fno-exceptions и -fno-rtti (вырезание оверхеда для кроссплатформенности).
    set(SOUL_FLAGS "-O3 -s -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections")
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SOUL_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SOUL_FLAGS}")
    
    if(NOT ARDUINO AND NOT EMBEDDED)
        # Оставляем агрессивную чистку мертвого кода линкером на десктопе
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -static")
    endif()
endif()

# ==============================================================================
# 3. СБОРКА ЯДРА SOULENGINE (Пути изменены под твою структуру)
# ==============================================================================
file(GLOB_RECURSE CORE_SOURCES 
    "main/*.cpp"
    "main/*.c"
)

add_library(soul_core STATIC ${CORE_SOURCES})

# Путь к твоим хедерам
target_include_directories(soul_core PUBLIC include)

# ==============================================================================
# 4. РАЗДЕЛЕНИЕ ТАРГЕТОВ
# ==============================================================================
if(NOT ARDUINO AND NOT EMBEDDED)
    # Сборка песочницы для ПК
    add_executable(soul_sandbox main/main.cpp)
    target_link_libraries(soul_sandbox PRIVATE soul_core)
    
    message(STATUS "[SoulEngine] Конфигурация для ПК (Sandbox) успешно создана")
else()
    # Сборка для контроллеров
    target_compile_definitions(soul_core PUBLIC SOUL_ENGINE_EMBEDDED)
    
    message(STATUS "[SoulEngine] Конфигурация для Embedded / Arduino успешно создана")
endif()
