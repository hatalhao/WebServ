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

def show_login_form():
    """Display the login form"""
    print('Content-Type: text/html; charset=utf-8')
    body = '''<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <title>Login</title>
  <style>
    body { 
      font-family: Arial, sans-serif; 
      margin: 40px; 
      background-color: #f5f5f5; 
    }
    .container { 
      max-width: 400px; 
      margin: 0 auto; 
      background-color: white; 
      padding: 30px; 
      border-radius: 8px; 
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    .form-group { 
      margin-bottom: 15px; 
    }
    label { 
      display: block; 
      margin-bottom: 5px; 
      font-weight: bold; 
    }
    input { 
      width: 100%; 
      padding: 8px; 
      border: 1px solid #ddd; 
      border-radius: 4px; 
      box-sizing: border-box; 
    }
    button { 
      width: 100%; 
      padding: 10px; 
      background-color: #007bff; 
      color: white; 
      border: none; 
      border-radius: 4px; 
      cursor: pointer; 
    }
    button:hover { 
      background-color: #0056b3; 
    }
    .info {
      background-color: #d1ecf1;
      padding: 10px;
      margin-bottom: 20px;
      border-radius: 4px;
      font-size: 14px;
    }
    .session-status {
      background-color: #fff3cd;
      padding: 10px;
      margin-bottom: 20px;
      border-radius: 4px;
      font-size: 14px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Login</h1>
    <div class="session-status">
      <strong>Session Status:</strong> No active session detected. Please log in.
    </div>
    <div class="info">
      <strong>Demo Credentials:</strong><br>
      Username: admin<br>
      Password: admin<br>
      <br>
      <strong>Session Management:</strong> Upon successful login, a session will be created and you'll be redirected to the welcome page. If you already have an active session, you'll be redirected automatically.
    </div>
    <form method="post" action="/cgi/login.py">
      <div class="form-group">
        <label for="username">Username</label>
        <input name="username" id="username" required />
      </div>
      <div class="form-group">
        <label for="password">Password</label>
        <input type="password" name="password" id="password" required />
      </div>
      <button type="submit">Sign in</button>
    </form>
    <br>
    <p><a href="/cgi/welcome.py">Check Session Status</a></p>
  </div>
</body>
</html>'''
    
    print('Content-Length: %d' % len(body))
    print()
    sys.stdout.write(body)

def main():
    session_id = get_session_id()
    
    if session_id:
        # User already has a session - redirect to welcome page
        print('Status: 302 Found')
        print('Location: /cgi/welcome.py')
        print('Content-Length: 0')
        print()
    else:
        # No session - show login form
        show_login_form()

if __name__ == '__main__':
    main()
