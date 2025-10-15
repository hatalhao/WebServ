#!/usr/bin/python3
import os
import sys
import urllib.parse

def parse_body():
    """Parses the POST request body."""
    try:
        length = int(os.environ.get('CONTENT_LENGTH', '0'))
    except (ValueError, TypeError):
        length = 0
    data = sys.stdin.read(length) if length > 0 else ''
    return urllib.parse.parse_qs(data)

def serve_login_form():
    """For GET requests, redirect to the main login page."""
    print('Status: 302 Found')
    print('Location: /index.html')
    print('Content-Length: 0')
    print()

def serve_error_page():
    """Serves a styled 401 Unauthorized error page from file."""
    # Get the script directory and construct path to error page
    script_dir = os.path.dirname(os.path.abspath(__file__))
    error_page_path = os.path.join(script_dir, '..', 'html', 'errors', '401.html')
    
    try:
        with open(error_page_path, 'rb') as f:
            body_bytes = f.read()
        
        print('Status: 401 Unauthorized')
        print('Content-Type: text/html; charset=utf-8')
        print(f'Content-Length: {len(body_bytes)}')
        print()
        sys.stdout.flush()
        sys.stdout.buffer.write(body_bytes)
    except FileNotFoundError:
        fallback = b'<html><body><h1>401 Unauthorized</h1><p>Login failed.</p></body></html>'
        print('Status: 401 Unauthorized')
        print('Content-Type: text/html; charset=utf-8')
        print(f'Content-Length: {len(fallback)}')
        print()
        sys.stdout.flush()
        sys.stdout.buffer.write(fallback)

def main():
    method = os.environ.get('REQUEST_METHOD', 'GET')
    
    if method == 'GET':
        serve_login_form()
        return

    if method != 'POST':
        print('Status: 405 Method Not Allowed')
        print('Content-Type: text/plain')
        print('Content-Length: 19')
        print()
        print('Method Not Allowed')
        return

    form = parse_body()
    user = (form.get('username', [''])[0]).strip()
    pwd  = (form.get('password', [''])[0]).strip()

    # Demo auth: accept user=admin, pass=admin
    if user == 'admin' and pwd == 'admin':
        # On success, create session and redirect to welcome page
        print('Status: 302 Found')
        print('Location: /cgi/welcome.py')
        print('X-WebServ-Create-Session: true') # Custom header for the server
        print('Content-Length: 0')
        print()
        return

    # On failure, serve the styled error page
    serve_error_page()

if __name__ == '__main__':
    main()
