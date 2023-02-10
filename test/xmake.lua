add_rules("mode.debug", "mode.release")
set_languages("c99", "c++20")

add_requires("asio")
add_requires("spdlog")
add_requires("jsoncpp")
add_requires("gtest")

target("test")
    set_kind("binary")
    add_files("*.cpp")
	add_includedirs("../server/include")
	add_files("../server/src/File.cpp")

	add_packages("asio")
	add_packages("spdlog")
	add_packages("jsoncpp")
	add_packages("gtest")
