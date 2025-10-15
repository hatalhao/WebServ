#!/usr/bin/python3
import os
import sys

TEMPLATE_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'html', 'status.html')

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

def render_page(title, content, session_id):
    template = load_template(TEMPLATE_PATH)
    page = template.replace('{{title}}', title).replace('{{content}}', content)
    return page

def main():
    session_id = get_session_id()
    if not session_id or not is_session_valid():
        print('Status: 302 Found')
        print('Location: /cgi/login.py')
        if session_id and not is_session_valid():
            # Clear invalid session cookie
            print('Set-Cookie: session_id=; HttpOnly; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT')
        print()
        return

    title = "System Status"
    content = '''
        <h2>Live System Status</h2>
        <ul class="status-list">
            <li class="status-item">
                <span class="status-name">Web Server (HTTP)</span>
                <span class="status-indicator status-ok"><span></span>Operational</span>
            </li>
            <li class="status-item">
                <span class="status-name">Database Service</span>
                <span class="status-indicator status-ok"><span></span>Operational</span>
            </li>
            <li class="status-item">
                <span class="status-name">API Gateway</span>
                <span class="status-indicator status-ok"><span></span>Operational</span>
            </li>
            <li class="status-item">
                <span class="status-name">Authentication Service</span>
                <span class="status-indicator status-ok"><span></span>Operational</span>
            </li>
            <li class="status-item">
                <span class="status-name">Mail Service</span>
                <span class="status-indicator status-offline"><span></span>Offline</span>
            </li>
        </ul>
    '''
    
    body = render_page(title, content, session_id)
    body_bytes = body.encode('utf-8')
    print('Content-Type: text/html; charset=utf-8')
    print(f'Content-Length: {len(body_bytes)}')
    print()
    sys.stdout.flush()
    sys.stdout.buffer.write(body_bytes)

if __name__ == '__main__':
    main()
