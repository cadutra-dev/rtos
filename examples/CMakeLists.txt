# Exemplos

add_executable(blink_led blink_led.c)
target_link_libraries(blink_led rtos)

add_executable(multi_task multi_task.c)
target_link_libraries(multi_task rtos)
