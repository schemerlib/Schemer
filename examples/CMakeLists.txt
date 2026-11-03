cmake_minimum_required(VERSION 3.21)

foreach (example simple_usage advanced_usage output_manipulation)
    add_executable(${example} ${example}.cpp)
    target_link_libraries(${example} PRIVATE Schemer::Schemer)
endforeach ()
