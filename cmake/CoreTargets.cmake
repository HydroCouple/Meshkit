add_library(meshkit_predicates STATIC vendor/predicates/predicates.c)
set_target_properties(meshkit_predicates PROPERTIES
    EXPORT_NAME predicates POSITION_INDEPENDENT_CODE ON
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_include_directories(meshkit_predicates PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/predicates>)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU|AppleClang")
    # The predicates are exact only if every product and sum is rounded on its
    # own: a fused multiply-add skips a rounding and breaks the error terms.
    # GCC fuses by default (-ffp-contract=fast) wherever the CPU has FMA, which
    # every arm64 CPU does; Clang fuses within an expression. Either corrupts
    # orient2d/incircle, and the constrained Delaunay kernel then fails.
    target_compile_options(meshkit_predicates PRIVATE
        -ffp-contract=off
        -Wno-deprecated-non-prototype -Wno-strict-prototypes
        -Wno-unused-function -Wno-unused-variable
        -Wno-implicit-function-declaration -Wno-old-style-definition)
endif()
if(MSVC)
    # The default model, restated so a consumer's global /fp:fast cannot reach the predicates.
    target_compile_options(meshkit_predicates PRIVATE /fp:precise /wd4996 /wd4244 /wd4267 /wd4101)
endif()


add_library(meshkit_core STATIC src/native/cdt.cpp src/native/mesh.cpp src/native/job.cpp src/native/patch.cpp src/native/regions.cpp src/native/planning.cpp src/native/sizefield.cpp src/native/blocks.cpp src/native/constraints.cpp src/native/constraintshared.cpp src/native/constraintaudit.cpp src/native/raster.cpp)
add_library(meshkit::core ALIAS meshkit_core)
set_target_properties(meshkit_core PROPERTIES EXPORT_NAME core POSITION_INDEPENDENT_CODE ON AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_compile_features(meshkit_core PUBLIC cxx_std_17)
target_include_directories(meshkit_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(meshkit_core PUBLIC meshkit::runtime PRIVATE meshkit_predicates)
if(MSVC)
    target_compile_definitions(meshkit_core PRIVATE _USE_MATH_DEFINES)
endif()
install(TARGETS meshkit_core meshkit_predicates EXPORT MeshKitTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES vendor/predicates/LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/MeshKit RENAME predicates.txt)

add_library(meshkit_io STATIC src/native/binaryio.cpp src/native/smsio.cpp)
add_library(meshkit::io ALIAS meshkit_io)
set_target_properties(meshkit_io PROPERTIES EXPORT_NAME io POSITION_INDEPENDENT_CODE ON AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_compile_features(meshkit_io PUBLIC cxx_std_17)
target_link_libraries(meshkit_io PUBLIC meshkit::core)
install(TARGETS meshkit_io EXPORT MeshKitTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
