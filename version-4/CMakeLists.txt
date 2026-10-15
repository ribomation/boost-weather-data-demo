SET(STEP v4)

# Weather-Data Core - the library
add_library(weather-data-core-${STEP} STATIC
        config/config.hxx
        config/config.cxx
        options/options.hxx
        options/options.cxx
        http/http-source.cxx
        http/http-source.hxx
        elapsed/elapsed.hxx
        observation/observation.hxx
        observation/observation.cxx
        csv/csv-reader.hxx
        csv/csv-reader.cxx
        tar/tar-reader.hxx
        tar/tar-reader.cxx
        pipeline/reading-pipeline.hxx
        pipeline/pipeline-counters.hxx
        counter/byte-counter.hxx
        aggregate/aggregator.hxx
        aggregate/aggregator.cxx
        report/reporter.cxx
        report/reporter.hxx
)
target_include_directories(weather-data-core-${STEP} PUBLIC
        config options http elapsed observation csv tar
        pipeline counter aggregate report
)
target_link_libraries(weather-data-core-${STEP} PUBLIC
        Boost::headers
        Boost::property_tree
        Boost::program_options
        Boost::beast
        Boost::url
        Boost::iostreams
        Boost::accumulators
        OpenSSL::SSL
        OpenSSL::Crypto
)
target_compile_definitions(weather-data-core-${STEP} PUBLIC APP_STEP="${STEP}")


# Weather-Data App - the program
add_executable(weather-data-${STEP} app.cxx)
target_link_libraries(weather-data-${STEP} PRIVATE
        weather-data-core-${STEP}
)


# Weather-Data Test - the unit-test suite
add_executable(unit-test-${STEP}
        config/config.test.cxx
        options/options.test.cxx
        http/http-source.test.cxx
        observation/observation.test.cxx
        csv/csv-reader.test.cxx
        tar/tar-reader.test.cxx
        elapsed/elapsed.test.cxx
        counter/byte-counter.test.cxx
        aggregate/aggregator.test.cxx
        report/reporter.test.cxx
)
target_link_libraries(unit-test-${STEP} PRIVATE
        weather-data-core-${STEP}
        Catch2::Catch2WithMain
)
catch_discover_tests(unit-test-${STEP})

