cmake_minimum_required(VERSION 3.1)

project(ddsTest)

find_package(OpenGL REQUIRED)
#${OPENGL_INCLUDE_DIRS}
#${OPENGL_LIBRARIES}
find_package(GLUT REQUIRED)
#${GLUT_INCLUDE_DIRS}
#${GLUT_LIBRARIES}


add_executable(ddsTest
#main.cc
				debug_opengl.cpp
				function_opengl.cpp
               	packet_control_interface.cc
               	uart_socket.cc
              )

target_link_libraries(ddsTest
						${OPENGL_LIBRARIES}
						${GLUT_LIBRARIES}
					)

set_property(TARGET ddsTest PROPERTY CXX_STANDARD 11)


