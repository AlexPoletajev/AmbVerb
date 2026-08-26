set(expected_value_count 256)
set(matrix_names Rxz3_1 Rxz3_2 Ryz3_1 Ryz3_2)

foreach(matrix_name IN LISTS matrix_names)
    set(matrix_path "${MATRIX_DIR}/${matrix_name}")

    if (NOT EXISTS "${matrix_path}")
        message(FATAL_ERROR "Missing rotation matrix: ${matrix_path}")
    endif()

    file(READ "${matrix_path}" matrix_contents)
    string(REGEX MATCHALL "[-+]?[0-9]+([.][0-9]+)?([eE][-+]?[0-9]+)?" matrix_values "${matrix_contents}")
    list(LENGTH matrix_values actual_value_count)

    if (NOT actual_value_count EQUAL expected_value_count)
        message(FATAL_ERROR
            "${matrix_name} contains ${actual_value_count} values; "
            "expected ${expected_value_count}")
    endif()
endforeach()
