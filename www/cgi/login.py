#!/usr/bin/python3
import os, sys, urllib.parse

def parse_body():
    length = int(os.environ.get('CONTENT_LENGTH','0') or '0')
    data = sys.stdin.read(length) if length > 0 else ''
    return urllib.parse.parse_qs(data)

def serve_login_form():
    """Serve the login HTML form for GET requests"""
    print('Status: 302 Found')
    print('Location: /index.html')
    print('Content-Length: 0')
    print()

def main():
    method = os.environ.get('REQUEST_METHOD','GET')
    if method == 'GET':
        serve_login_form()
        return
    if method != 'POST':
        print('Status: 405 Method Not Allowed')
        print('Content-Type: text/plain')
        print()
        print('Method Not Allowed')
        return

    form = parse_body()
    user = (form.get('username',[''])[0]).strip()
    pwd  = (form.get('password',[''])[0]).strip()

    # demo auth: accept user=admin, pass=admin
    if user == 'admin' and pwd == 'admin':
        # on success, create session and redirect to welcome page
        print('Status: 302 Found')
        print('Location: /cgi/welcome.py')
        print('X-WebServ-Create-Session: true')
        print('Content-Length: 0')
        print()
        return

    # failure
    print('Status: 401 Unauthorized')
    print('Content-Type: text/html; charset=utf-8')
    body = '<h1>Login failed</h1><p>Invalid username or password.</p><a href="/index.html">Try again</a>'
    print('Content-Length: %d' % len(body))
    print()
    sys.stdout.write(body)

if __name__ == '__main__':
    main()
