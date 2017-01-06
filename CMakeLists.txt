cmake_minimum_required(VERSION 2.8.9)
project(iotmanager)

add_subdirectory(iotServer)
add_subdirectory(iotScheduler)
#set(CMAKE_INSTALL_PREFIX "/etc/init.d/")
install(
	FILES files/iotgateway files/iotscheduler  
	PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE WORLD_EXECUTE
	DESTINATION "/etc/init.d" 
)
install(FILES files/iotgateway.conf DESTINATION "/etc/")
MESSAGE("PROJECT_SOURRCE_DIR is ${PROJECT_SOURCE_DIR}")
