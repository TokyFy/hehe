#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/plain")
print()  # blank line required between headers and body

print("Hello World")
print("Method:", os.environ.get("REQUEST_METHOD", "unknown"))
print("Query:", os.environ.get("QUERY_STRING", ""))

if os.environ.get("REQUEST_METHOD") == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    if content_length > 0:
        body = sys.stdin.read(content_length)
        print("Body:", body)

