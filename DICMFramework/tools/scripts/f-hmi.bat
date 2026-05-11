@call b.bat
@python %IDF_PATH%/tools/idf.py -p com%1 -b 921600 flash
@python %IDF_PATH%/tools/idf.py -p com%1 -b 921600 hmi_data-flash
@python %IDF_PATH%/tools/idf.py -p com%1 -b 921600 hmi_data_1-flash
@python %IDF_PATH%/tools/idf.py -p com%1 -b 115200 monitor