#!/usr/bin/python3
import os
import sys

TEMPLATE_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'html', 'analytics.html')

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
        
    title = "View Analytics"
    content = '''
        <h2>Server Analytics</h2>
        <div class="stats-grid">
            <div class="stat-card">
                <div class="label">Site Visits (24h)</div>
                <div class="value">1,428</div>
            </div>
            <div class="stat-card">
                <div class="label">User Engagement</div>
                <div class="value">78%</div>
            </div>
            <div class="stat-card">
                <div class="label">Server Load</div>
                <div class="value">15%</div>
            </div>
        </div>
        <div class="chart-placeholder">
            <span>Traffic Data Chart (Coming Soon)</span>
        </div>
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
