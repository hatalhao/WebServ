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
    # Let server handle all session validation via SESSION_VALID env var
    if is_session_valid():
        session_id = get_session_id() or 'unknown'
        body_template = load_template(TEMPLATE_PATH)
        body = body_template.replace('{{session_id}}', session_id)

        print('Content-Type: text/html; charset=utf-8')
        print(f'Content-Length: {len(body.encode("utf-8"))}')
        print()
        sys.stdout.flush()
        sys.stdout.buffer.write(body.encode('utf-8'))
    else:
        # Server already handles session cleanup, just redirect
        print('Status: 302 Found')
        print('Location: /cgi/login.py')
        print('Content-Length: 0')
        print()


if __name__ == '__main__':
    main()