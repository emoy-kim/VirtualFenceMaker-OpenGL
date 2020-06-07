target_link_libraries(VirtualFenceMakerGL glad glfw3dll)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
   target_link_libraries(VirtualFenceMakerGL FreeImaged)
else()
   target_link_libraries(VirtualFenceMakerGL FreeImage)
endif()