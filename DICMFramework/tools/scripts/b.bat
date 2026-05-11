@echo off
@rem build: If adding arguement to b.bat the first one needs to be the com port number. 
@rem The rest shall follow the pattern; -D<CMAKE_DEFINE>=<value>
@rem Examples: 
@rem b.bat 2 -DBUILD_PRODUCT_ID=AB1
@rem b.bat 2 -DBUILD_PRODUCT_ID=AB1 -DMIC_BUILD_PRODUCTION_ENVIRONMENT=1

@echo on
python %IDF_PATH%/tools/idf.py -p com%* -b 921600 all size
