SET(STEP v2)

add_library(weather-data-core-${STEP} STATIC
        config/config.hxx
        config/config.cxx
        options/options.hxx
        options/options.cxx
        http/http-source.cxx
        http/http-source.hxx
        elapsed/elapsed.hxx
)
target_include_directories(weather-data-core-${STEP} PUBLIC
        config options http elapsed
)
target_link_libraries(weather-data-core-${STEP} PUBLIC
        Boost::headers
        Boost::property_tree
        Boost::program_options
        Boost::beast
        Boost::url
        Boost::iostreams
        OpenSSL::SSL
        OpenSSL::Crypto
)
target_compile_definitions(weather-data-core-${STEP} PUBLIC APP_STEP="${STEP}")


add_executable(weather-data-${STEP} app.cxx)
target_link_libraries(weather-data-${STEP} PRIVATE
        weather-data-core-${STEP}
)


add_executable(unit-test-${STEP}
        config/config.test.cxx
        options/options.test.cxx
)
target_link_libraries(unit-test-${STEP} PRIVATE
        weather-data-core-${STEP}
        Catch2::Catch2WithMain
)
catch_discover_tests(unit-test-${STEP})

