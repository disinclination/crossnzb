cmake_minimum_required(VERSION 3.10)
project(CrossNZB VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (MSVC)
    foreach(flag_var
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE)
        if (${flag_var} MATCHES "/MD")
            string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

# Static Dependencies
set(Boost_USE_STATIC_LIBS ON)
set(OPENSSL_USE_STATIC_LIBS TRUE)
set(TinyXML2_USE_STATIC_LIBS ON)


find_package(Boost REQUIRED COMPONENTS system asio)
find_package(nlohmann_json REQUIRED)
find_package(TinyXML2 REQUIRED CONFIG)
find_package(OpenSSL REQUIRED Crypto SSL)
find_package(jsoncpp CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)

if (WIN32)
    set(RESOURCE_FILES
            src/resource.rc
    )
elseif ()

add_executable(CrossNZB
        src/main.cpp
        src/NNTP/NNTPClient.cpp
        src/NNTP/NNTPClient.h
        src/resource.h
)

target_include_directories(CrossNZB PRIVATE ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} ${TinyXML2_INCLUDE_DIR})

target_link_libraries(CrossNZB PRIVATE
        Boost::system
        nlohmann_json::nlohmann_json
        OpenSSL::SSL
        OpenSSL::Crypto
        tinyxml2::tinyxml2
        JsonCpp::JsonCpp
        pugixml::static
        pugixml::pugixml
)

if (WIN32)
    target_sources(CrossNZB PRIVATE src/resource.rc)
    target_link_libraries(CrossNZB PRIVATE ws2_32)
endif()