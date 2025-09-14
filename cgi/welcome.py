#!/usr/bin/python3
import os
import sys

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

def main():
    session_id = get_session_id()
    
    if session_id:
        # User is logged in - show welcome page
        print('Content-Type: text/html; charset=utf-8')
        body = '''<!DOCTYPE html>
<html>
<head>
    <title>Welcome</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .container { max-width: 600px; margin: 0 auto; }
        .logout-btn { 
            background-color: #dc3545; 
            color: white; 
            padding: 10px 20px; 
            text-decoration: none; 
            border-radius: 4px; 
            margin-left: 10px;
        }
        .logout-btn:hover { background-color: #c82333; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Welcome, admin!</h1>
        <p>You are successfully logged in.</p>
        <p>Session ID: %s</p>
        <p>
            <a href="/cgi/login_page.py">Login Page</a>
            <a href="/cgi/logout.py" class="logout-btn">Logout</a>
        </p>
    </div>
</body>
</html>''' % session_id
        
        print('Content-Length: %d' % len(body))
        print()
        sys.stdout.write(body)
    else:
        # User is not logged in - redirect to login
        print('Status: 302 Found')
        print('Location: /cgi/login_page.py')
        print('Content-Length: 0')
        print()

if __name__ == '__main__':
    main()