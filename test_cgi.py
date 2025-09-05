#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/html")
print("")
print("<html><head><title>CGI POST Test</title></head><body>")
print("<h1>CGI POST Test Results</h1>")

# Read POST data
content_length = os.environ.get('CONTENT_LENGTH', '0')
if content_length:
    post_data = sys.stdin.read(int(content_length))
    print(f"<p>Received POST data: <strong>{post_data}</strong></p>")
else:
    print("<p>No POST data received</p>")

print("<p>Environment variables:</p><ul>")
for key, value in os.environ.items():
    if key.startswith('HTTP_') or key in ['REQUEST_METHOD', 'CONTENT_TYPE', 'CONTENT_LENGTH']:
        print(f"<li><strong>{key}:</strong> {value}</li>")
print("</ul>")
print("</body></html>")
