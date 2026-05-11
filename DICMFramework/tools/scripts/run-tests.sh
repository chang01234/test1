#!/bin/bash

echo "::notice::Current working directory: $(pwd)"

echo "Cleaning up previous build artifacts..."
rm -rf build/ bin/ sdkconfig*

echo "Installing python module gcovr@8.4 ..."
pip install gcovr==8.4

git config --global --add safe.directory $(pwd)
git config --global --add safe.directory $(pwd)/DICMFramework
git config --global --add safe.directory $(pwd)/build/googletest-src

idf.py --preview set-target linux
ret=$?
if [ $ret -ne 0 ]; then
    echo "::error::Failed to set idf target to linux!"
    exit $ret
fi

idf.py -D CMAKE_BUILD_TYPE=UnitTest build partition-table
ret=$?
if [ $ret -ne 0 ]; then
    echo "::error::Failed to generate unit test build"
    exit $ret
fi
ctest_report_dir="ctest-reports"
mkdir -p "build/$ctest_report_dir"

# Run ctest to print labels and capture the output
labels_output=$(ctest --test-dir build --print-labels)
# Extract the labels from the output
labels=$(echo "$labels_output" | grep "  " | awk '{$1=$1;print}')
# Check if any labels were found
if [ -z "$labels" ]; then
    echo "No labels found."
    exit 1
fi
# Loop through each label and run ctest for that label
status=0
for label in $labels; do
    echo "Running ctest for label: $label"
    ctest --test-dir build --schedule-random -L "$label" --output-on-failure --repeat until-pass:5 --output-junit "$ctest_report_dir"/test"$label"summary.xml
    if [ $? -ne 0 ]; then
        echo "::error::Tests failed for label: $label"
        status=$?
    fi
done
# Fixing ctest testsuite name -> label
python $(pwd)/DICMFramework/tools/python/update_testsuite_names.py build/"$ctest_report_dir"

# Fixing gtest times
# python $(pwd)/DICMFramework/tools/python/update_gtest_times.py

covr_report_dir="build/coverage-report"
mkdir -p "$covr_report_dir"
gcovr -r $(pwd) --xml -o "$covr_report_dir"/coverage.cobertura.xml -e ".*/test/*" -e ".*/unittest/*" -e ".*/mocks/*" -e ".*/googletest-src/*/*"

exit $status
