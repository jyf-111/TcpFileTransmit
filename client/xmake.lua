add_rules("mode.debug", "mode.release")
set_languages("c99", "c++20")
add_cxxflags("-fpermissive")

add_requires("asio")
add_requires("spdlog")
add_requires("jsoncpp")
add_requires("imgui v1.89-docking", { configs = { glfw_opengl3 = true } })
add_requires("glew", { configs = { shared = false } })
add_requires("openssl")

target("client")
do
	set_kind("binary")
	add_includedirs("include")
	add_includedirs("3rdparty/ImGuiFileDialog")
	add_files("src/*.cpp")
	add_files("3rdparty/ImGuiFileDialog/ImGuiFileDialog.cpp")

	add_packages("asio")
	add_packages("spdlog")
	add_packages("jsoncpp")
	add_packages("imgui")
	add_packages("glew")
	add_packages("openssl")
end
