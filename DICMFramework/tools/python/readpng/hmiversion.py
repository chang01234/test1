#! /usr/bin/env python3
import os
import sys

HMI_VERSION = str(os.environ.get("HMI_DATA_VERSION", ""))
if HMI_VERSION == "":
    print("HMI_DATA_VERSION environment variable not set.")
    sys.exit(-1)
