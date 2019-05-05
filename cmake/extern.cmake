cmake_minimum_required(VERSION 3.0)

function(register_extern_include)
    get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    string(REPLACE " " "_" LIB_NAME ${LIB_NAME})

    set(${INCLUDE}_LOCAL "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set_property(GLOBAL PROPERTY ${LIB_NAME}_INCLUDE ${${INCLUDE}_LOCAL})
    message("Registered lib include: " ${LIB_NAME})
endfunction()

function(register_extern_lib LIB_PATH)
    get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    string(REPLACE " " "_" LIB_NAME ${LIB_NAME})

    set(${LIB}_LOCAL ${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIB_PATH})
    set_property(GLOBAL PROPERTY ${LIB_NAME}_LIB ${${LIB}_LOCAL})
    message("Registered lib binary: " ${LIB_NAME})
endfunction()

function(register_extern_runtime RUNTIME_PATH)
    get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    string(REPLACE " " "_" LIB_NAME ${LIB_NAME})

    set(${RUNTIME}_LOCAL ${CMAKE_CURRENT_SOURCE_DIR}/lib/${RUNTIME_PATH})
    set_property(GLOBAL PROPERTY ${LIB_NAME}_RUNTIME ${${RUNTIME}_LOCAL})
    message("Registered runtime binary: " ${LIB_NAME})
endfunction()


function(include_extern libname)
    get_property(EXTERN_INCLUDE GLOBAL PROPERTY ${libname}_INCLUDE)
    message("Include ${PROJECT_NAME} <- ${libname}=" ${EXTERN_INCLUDE})
    include_directories(${EXTERN_INCLUDE})
endfunction()

function(link_extern libname)
    get_property(EXTERN_LIB GLOBAL PROPERTY ${libname}_LIB)
    message("Linking ${PROJECT_NAME} <- ${libname}=" ${EXTERN_LIB})
    target_link_libraries(${PROJECT_NAME} ${EXTERN_LIB})

	get_property(EXTERN_RUNTIME GLOBAL PROPERTY ${libname}_RUNTIME)
	if(EXISTS ${EXTERN_RUNTIME})
		message("Runtime ${PROJECT_NAME} <- ${libname}=" ${EXTERN_RUNTIME})
		
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${EXTERN_RUNTIME}
			$<TARGET_FILE_DIR:${PROJECT_NAME}>)
	endif()
endfunction()
