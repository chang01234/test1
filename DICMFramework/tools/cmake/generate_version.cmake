# Git executable is extracted from parameters.
execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
                 OUTPUT_VARIABLE GIT_REPO_VERSION
                 OUTPUT_STRIP_TRAILING_WHITESPACE)
# Input and output files are extracted from parameters.
configure_file(${INPUT_FILE} ${OUTPUT_FILE})
