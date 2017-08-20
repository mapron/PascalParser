#[[
  Copyright (C) 2017 Smirnov Vladimir mapron1@gmail.com
  Source code licensed under the Apache License, Version 2.0 (the "License");
  You may not use this file except in compliance with the License.
  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 or in file COPYING-APACHE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.h
#]]

cmake_minimum_required(VERSION 3.5)
project(PascalParser)

# main paths.


#configure options
set( BOOST_INCLUDEDIR "" CACHE PATH "Boost path to includes")
if (BOOST_INCLUDEDIR)
	include_directories(${BOOST_INCLUDEDIR})
endif()

find_package(Qt5Core REQUIRED)
find_package(Qt5Test)

#platform configuration.
if (MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX /wd4267 /wd4244)
else()
	#add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX /wd4267 /wd4244)
endif()

if (NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z ")
endif()

set(sys_deps)

if (APPLE)
	list(APPEND sys_deps -stdlib=libc++ -lc++)
endif()


# function for target declaration.
function(AddTarget)
	set(options APP)
	set(oneValueArgs NAME ROOT SUBDIR)
	set(multiValueArgs CSRC INCLUDES DEPS EXCLUDE OPTIONS DEFINES)
	cmake_parse_arguments(AddTarget "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
	set(_sources)
	if (NOT AddTarget_ROOT)
		set(AddTarget_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/${AddTarget_SUBDIR}${AddTarget_NAME})
	endif()
	foreach (csrc ${AddTarget_CSRC})
		file(GLOB src ${AddTarget_ROOT}/${csrc})
		list(APPEND _sources ${src})
	endforeach()
	foreach (excl ${AddTarget_EXCLUDE})
		list(REMOVE_ITEM _sources ${AddTarget_ROOT}/${excl})
	endforeach()
	if (AddTarget_APP)
		add_executable(${AddTarget_NAME} ${_sources})
	else()
		add_library(${AddTarget_NAME} STATIC ${_sources})
	endif()
	foreach (inc ${AddTarget_INCLUDES})
		target_include_directories(${AddTarget_NAME} PRIVATE ${inc})
	endforeach()
	target_include_directories(${AddTarget_NAME} PUBLIC ${AddTarget_ROOT})
	foreach (dep ${AddTarget_DEPS})
		target_link_libraries(${AddTarget_NAME} PRIVATE ${dep})
	endforeach()
	foreach (opt ${AddTarget_OPTIONS})
		target_compile_options(${AddTarget_NAME} PRIVATE ${opt})
	endforeach()
	foreach (opt ${AddTarget_DEFINES})
		target_compile_definitions(${AddTarget_NAME} PRIVATE ${opt})
	endforeach()
endfunction()

set(CMAKE_INCLUDE_CURRENT_DIR ON)


AddTarget(NAME TreeVariant ROOT TreeVariant/ CSRC *.cpp *.h
	DEPS
		Qt5::Core
)

AddTarget(NAME ScriptRuntime ROOT ScriptRuntime/ CSRC *.cpp *.h
	DEPS
		TreeVariant
)

AddTarget(NAME ScriptParser ROOT ScriptParser/ CSRC *.cpp *.h}
	DEPS
		ScriptRuntime TreeVariant Qt5::Core
)
AddTarget(APP NAME Pascal2cpp ROOT Pascal2cpp/ CSRC *.cpp
	DEPS
		ScriptParser ScriptRuntime TreeVariant Qt5::Core
)
if (Qt5Test_DIR)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)
	AddTarget(APP NAME ScriptTest ROOT tests/ CSRC *.cpp *.h *.qrc
		DEPS
			ScriptParser ScriptRuntime TreeVariant Qt5::Core Qt5::Test
	)
	set(CMAKE_AUTOMOC OFF)
	set(CMAKE_AUTORCC OFF)
endif()
