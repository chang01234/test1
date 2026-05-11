import xml.etree.ElementTree as ET
import os
import sys

def update_testsuite_name(file_path):
    # Parse the XML file
    tree = ET.parse(file_path)
    root = tree.getroot()

    # Find the first <property> element with name="cmake_labels"
    for testcase in root.findall("testcase"):
        properties = testcase.find("properties")
        if properties is not None:
            for prop in properties.findall("property"):
                if prop.attrib.get("name") == "cmake_labels":
                    cmake_label = prop.attrib.get("value")
                    if cmake_label:
                        # Update the <testsuite> name attribute
                        root.attrib["name"] = cmake_label
                        # Save the updated XML file
                        tree.write(file_path, encoding="UTF-8", xml_declaration=True)
                        print(f"Updated 'name' attribute in {file_path} to '{cmake_label}'")
                        return
    print(f"No 'cmake_labels' property found in {file_path}")

def main():
    # Check if the directory is provided as an argument
    if len(sys.argv) != 2:
        print("Usage: python update_testsuite_name.py <directory>")
        sys.exit(1)

    directory = sys.argv[1]

    # Check if the directory exists
    if not os.path.isdir(directory):
        print(f"Error: Directory '{directory}' does not exist.")
        sys.exit(1)

    # Process all XML files in the directory
    for filename in os.listdir(directory):
        if filename.endswith(".xml"):
            file_path = os.path.join(directory, filename)
            update_testsuite_name(file_path)

if __name__ == "__main__":
    main()