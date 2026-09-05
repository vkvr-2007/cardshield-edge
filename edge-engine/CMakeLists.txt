add_executable(edge-engine
    main.cpp
    src/detection/AnomalyScorer.cpp
    src/network/TcpServer.cpp
    src/processing/EventProcessor.cpp
)

target_include_directories(edge-engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WIN32)
    target_link_libraries(edge-engine
        PRIVATE
            nlohmann_json::nlohmann_json
            ws2_32
    )
else()
    target_link_libraries(edge-engine
        PRIVATE
            nlohmann_json::nlohmann_json
    )
endif()