#!/usr/bin/python3
import os
import sys

TEMPLATE_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'html', 'welcome.html')

def get_session_id():
    """Extract session ID from cookies"""
    cookies = os.environ.get('HTTP_COOKIE', '')
    if not cookies:
        return None

    for cookie in cookies.split(';'):
        cookie = cookie.strip()
        if cookie.startswith('session_id='):
            return cookie.split('=', 1)[1]

    return None

def is_session_valid():
    """Check if the session is valid on the server"""
    return os.environ.get('SESSION_VALID', '0') == '1'

def load_template(path):
    try:
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    except Exception as e:
        return '<html><body><h1>Template load error</h1><p>{}</p></body></html>'.format(str(e))

def main():
    session_id = get_session_id()

    if session_id and is_session_valid():
        # User is logged in with valid session - read template from disk and substitute session id
        body_template = load_template(TEMPLATE_PATH)
        body = body_template.replace('{{session_id}}', session_id)

        # Print headers then body
        print('Content-Type: text/html; charset=utf-8')
        print(f'Content-Length: {len(body.encode("utf-8"))}')
        print()
        sys.stdout.flush()
        sys.stdout.buffer.write(body.encode('utf-8'))
    else:
        # User is not logged in or session is invalid - redirect to login
        if session_id and not is_session_valid():
            # Clear invalid session cookie
            print('Status: 302 Found')
            print('Location: /cgi/login.py')
            print('Set-Cookie: session_id=; HttpOnly; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT')
            print('Content-Length: 0')
        else:
            print('Status: 302 Found')
            print('Location: /cgi/login.py')
            print('Content-Length: 0')
        print()


if __name__ == '__main__':
    main()