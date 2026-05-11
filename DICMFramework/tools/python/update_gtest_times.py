import xml.etree.ElementTree as ET
import os

def update_gtest_times(ctest_file, gtest_file):
    print("Running on ", gtest_file)
    # Parse the CTest output file
    ctest_tree = ET.parse(ctest_file)
    ctest_root = ctest_tree.getroot()

    # Create a mapping of "classname" to "time" from the CTest file
    ctest_times = {}
    for testcase in ctest_root.findall("testcase"):
        classname = testcase.attrib.get("classname")
        time = testcase.attrib.get("time")
        if classname and time:
            ctest_times[classname] = str(float(int(float(time)*1000.0)))

    # Parse the GTest output file
    gtest_tree = ET.parse(gtest_file)
    gtest_root = gtest_tree.getroot()

    # Update the <testsuites> "time" attribute
    #if "time" in gtest_root.attrib:
    #    gtest_root.attrib["time"] = ctest_root.attrib.get("time", gtest_root.attrib["time"])

    # Update the <testcase> "time" attributes
    for testsuite in gtest_root.findall("testsuite"):
        for testcase in testsuite.findall("testcase"):
            classname = testcase.attrib.get("classname")
            testcasename = testcase.attrib.get("name")
            clsname = classname + "." + testcasename
            print(clsname)
            if clsname in ctest_times:
                testcase.attrib["time"] = ctest_times[clsname]
                if "time" in testsuite.attrib:
                    testsuite.attrib["time"] = ctest_times[clsname] #ctest_root.attrib.get("time", gtest_root.attrib["time"])
                if "time" in gtest_root.attrib:
                    gtest_root.attrib["time"] = ctest_times[clsname] #ctest_root.attrib.get("time", gtest_root.attrib["time"])

    # Save the updated GTest XML file
    gtest_tree.write(gtest_file, encoding="UTF-8", xml_declaration=True)
    # print(f"Updated times in {gtest_file}")

# Example usage
path = './build/test-results'

ctest_file = "./build/testsummary.xml"  # Path to the CTest output file
gtest_file = "./build/test-results/DICMCommonLibs_UnitTest_dcp_encapsulation_1.xml"  # Path to the GTest output file

for filename in os.listdir(path):
    if not filename.endswith('.xml'): continue
    fullname = os.path.join(path, filename)
    update_gtest_times(ctest_file, fullname)