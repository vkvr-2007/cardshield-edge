add_executable(cardshield-simulator
    main.cpp
)

target_link_libraries(cardshield-simulator
    PRIVATE
        nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(cardshield-simulator
        PRIVATE
            ws2_32
    )
endif()
