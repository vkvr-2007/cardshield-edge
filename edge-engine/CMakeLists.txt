add_executable(edge-engine
    main.cpp
    src/network/TcpServer.cpp
)

target_include_directories(edge-engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)