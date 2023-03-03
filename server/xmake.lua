add_rules("mode.debug", "mode.release")
set_languages("c99", "c++20")
add_cxxflags("-fpermissive")

add_requires("asio")
add_requires("spdlog")
add_requires("jsoncpp")
add_requires("openssl")

target("server")
do
	set_kind("binary")
	add_includedirs("include")
	add_files("src/*.cpp")

	add_packages("asio")
	add_packages("spdlog")
	add_packages("jsoncpp")
	add_packages("openssl")
end
