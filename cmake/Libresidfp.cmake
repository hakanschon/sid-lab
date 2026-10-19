# Local adapter for the exact 1.2.3 Autotools source list. No upstream edits.
set(RESID_UPSTREAM "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libresidfp")
set(RESID_ROOT "${CMAKE_CURRENT_BINARY_DIR}/resid-source")
file(COPY "${RESID_UPSTREAM}/src" DESTINATION "${RESID_ROOT}")
# Upstream 1.2.3 shares a mutable dither index between render threads. Use a
# lock-free relaxed atomic counter (no thread-local lazy allocation on first render).
set(dither_header "${RESID_ROOT}/src/FilterModelConfig.h")
file(READ "${dither_header}" dither_source)
string(REPLACE "#include <random>" "#include <random>\n#include <atomic>" dither_source "${dither_source}")
string(REPLACE "mutable int index = 0;" "mutable std::atomic<unsigned> index {0};\n        static_assert(std::atomic<unsigned>::is_always_lock_free);" dither_source "${dither_source}")
string(REPLACE "index = (index + 1) & 0x3ff; return buffer[index];" "return buffer[(index.fetch_add(1, std::memory_order_relaxed) + 1) & 0x3ff];" dither_source "${dither_source}")
file(WRITE "${dither_header}" "${dither_source}")
set(RESID_GEN "${CMAKE_CURRENT_BINARY_DIR}/resid-generated")
file(MAKE_DIRECTORY "${RESID_GEN}/residfp")
set(RESIDFP_BRANCH_HINTS 1)
set(HAVE_BUILTIN_EXPECT 1)
set(RESIDFP_INLINING 1)
set(RESIDFP_INLINE inline)
set(PACKAGE_VERSION 1.2.3)
set(LIB_MAJOR 1)
set(LIB_MINOR 2)
set(LIB_LEVEL 3)
configure_file("${RESID_ROOT}/src/siddefs-fp.h.in" "${RESID_GEN}/siddefs-fp.h" @ONLY)
configure_file("${RESID_ROOT}/src/residfp/sidversion.h.in" "${RESID_GEN}/residfp/sidversion.h" @ONLY)
set(RESID_SOURCES Dac EnvelopeGenerator ExternalFilter Filter Filter6581 Filter8580
    FilterModelConfig FilterModelConfig6581 FilterModelConfig8580 Integrator6581
    Integrator8580 OpAmp SID Spline State WaveformCalculator WaveformGenerator)
list(TRANSFORM RESID_SOURCES PREPEND "${RESID_ROOT}/src/")
list(TRANSFORM RESID_SOURCES APPEND ".cpp")
add_library(residfp STATIC ${RESID_SOURCES} "${RESID_ROOT}/src/resample/SincResampler.cpp"
    "${RESID_ROOT}/src/version.cc" "${RESID_ROOT}/src/residfp/residfp.cpp")
target_include_directories(residfp PUBLIC "${RESID_ROOT}/src" "${RESID_GEN}")
target_compile_features(residfp PUBLIC cxx_std_20)
set_target_properties(residfp PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_VISIBILITY_PRESET hidden)
# Keep finite-value checks valid; do not use -ffast-math.
find_package(Threads REQUIRED)
target_link_libraries(residfp PRIVATE Threads::Threads)
