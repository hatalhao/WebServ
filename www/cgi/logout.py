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
    
    # Clear the session cookie and redirect to login
    print('Status: 302 Found')
    print('Location: /cgi/login.py')
    print('Set-Cookie: session_id=; HttpOnly; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT')
    
    # Optional: Signal to the server to delete the session
    if session_id:
        print('X-WebServ-Delete-Session: %s' % session_id)
    
    print('Content-Length: 0')
    print()

if __name__ == '__main__':
    main()