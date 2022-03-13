CMAKE_MINIMUM_REQUIRED(VERSION 3.4.0)

PROJECT(ctviewer)

CMAKE_POLICY(SET CMP0020 NEW)
CMAKE_POLICY(SET CMP0028 NEW)
CMAKE_POLICY(SET CMP0053 NEW)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

#---设置自动生成moc文件,不可缺少---------
#--加了这句话之后可以不用写QT5_WRAP_CPP(WRAP_SRCS_H ${HEARD_FILES})
#
SET(CMAKE_AUTOMOC ON)

#------------------------------------------
FIND_PACKAGE(Qt5Widgets)
FIND_PACKAGE(Qt5Core)
FIND_PACKAGE(Qt5Gui)
#FIND_PACKAGE(qtmain)

set(VTK_DIR "E:/Pg/VTK/VTK-8.2.0" CACHE FILEPATH "VTK dir")
FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

FIND_PACKAGE(ITK REQUIRED)
INCLUDE(${ITK_USE_FILE})

#--------查找当前文件中的左右源码----------
FILE(GLOB SRC_FILES "./*.cpp")
FILE(GLOB HEAD_FILES "./*.h")
FILE(GLOB UI_FILES "./*.ui")


#--------生成ui文件的头文件----------------
QT5_WRAP_UI(WRAP_FILES ${UI_FILES})
QT5_WRAP_CPP(WRAP_SRCS_H ${HEARD_FILES})
QT5_ADD_RESOURCES(PROJECT_RESOURCE_RCC ${PROJECT_RESOURCE})

#--------添加资源文件----------------------


#--------创建工程文件-----------------------
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR})

ADD_EXECUTABLE(${PROJECT_NAME}
               ${SRC_FILES}
               ${HEAD_FILES}
               ${UI_FILES}
               ${WRAP_FILES}
               ${WRAP_SRCS_H}
               ${PROJECT_RESOURCE_RCC}
               ${PROJECT_RESOURCE} )

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
                      Qt5::Widgets
                      Qt5::Core         
                      Qt5::Gui
	      ${VTK_LIBRARIES}
                      ${ITK_LIBRARIES}) 

#---------添加筛选器----------------				  
source_group("Forms" FILES ${UI_FILES})
source_group("Forms\\WRAP_UI" FILES ${WRAP_FILES})

#---------自动调试----------------  
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/Od")
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/DEBUG")