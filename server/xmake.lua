add_rules("mode.debug", "mode.release")
set_languages("c99", "c++20")

add_requires("asio")
add_requires("spdlog")
add_requires("jsoncpp")

target("server")
    set_kind("binary")
	add_includedirs("include")
    add_files("src/*.cpp")
	add_packages("asio")
	add_packages("spdlog")
	add_packages("jsoncpp")
