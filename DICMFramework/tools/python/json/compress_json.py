#!/usr/bin/env python3
"""
JSON Compression Script

This script reads a JSON file, removes all instances of "comment" keys,
and writes the compressed JSON back to a file.

Usage:
    python compress_json.py input.json [output.json]
    
If no output file is specified, it will overwrite the input file.
"""

import json
import sys
import os
from typing import Any, Dict, List, Union

def remove_comments(obj: Any) -> Any:
    """
    Recursively remove all instances of "comment" keys from a JSON object.
    
    Args:
        obj: The JSON object (dict, list, or primitive value)
        
    Returns:
        The JSON object with all "comment" keys removed
    """
    if isinstance(obj, dict):
        # Create a new dictionary without "comment" keys
        cleaned = {}
        for key, value in obj.items():
            if key != "comment":
                cleaned[key] = remove_comments(value)
        return cleaned
    elif isinstance(obj, list):
        # Recursively clean each item in the list
        return [remove_comments(item) for item in obj]
    else:
        # Return primitive values as-is
        return obj

def compress_json_file(input_file: str, output_file: str = None) -> None:
    """
    Compress a JSON file by removing comments and minimizing whitespace.
    
    Args:
        input_file: Path to the input JSON file
        output_file: Path to the output JSON file (optional)
    """
    if output_file is None:
        output_file = input_file
    
    try:
        # Read the input JSON file
        with open(input_file, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        print(f"Loaded JSON from: {input_file}")
        
        # Remove all "comment" keys
        cleaned_data = remove_comments(data)
        
        # Write the compressed JSON (no indentation, no extra spaces)
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(cleaned_data, f, separators=(',', ':'), ensure_ascii=False)
        
        print(f"Compressed JSON saved to: {output_file}")
        
        # Show file size comparison
        input_size = os.path.getsize(input_file)
        output_size = os.path.getsize(output_file)
        compression_ratio = (1 - output_size / input_size) * 100
        
        print(f"Original size: {input_size} bytes")
        print(f"Compressed size: {output_size} bytes")
        print(f"Compression ratio: {compression_ratio:.1f}%")
        
    except FileNotFoundError:
        print(f"Error: File '{input_file}' not found.")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in '{input_file}': {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

def main():
    """Main function to handle command line arguments."""
    if len(sys.argv) < 2:
        print("Usage: python compress_json.py input.json [output.json]")
        print("If no output file is specified, the input file will be overwritten.")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    compress_json_file(input_file, output_file)

if __name__ == "__main__":
    main()
