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

def render_page(title, content, session_id):
    """Renders the common page structure with specific content."""
    return f'''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title}</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap" rel="stylesheet">
    <style>
        :root {{
            --text-color: #f0f0f0;
            --text-muted-color: #a0a0a0;
            --glass-bg: rgba(22, 22, 22, 0.4);
            --glass-border: rgba(255, 255, 255, 0.1);
            --primary-color: #007bff;
        }}
        body {{
            font-family: 'Inter', sans-serif;
            margin: 0;
            background-color: #050505;
            color: var(--text-color);
            background-image: radial-gradient(circle at 10% 10%, #007bff 0%, transparent 40%),
                              radial-gradient(circle at 85% 90%, #8a2be2 0%, transparent 40%);
            background-attachment: fixed;
            overflow-y: auto;
        }}
        .page-wrapper {{
            opacity: 0;
            animation: fadeIn 1s ease-out forwards;
        }}
        .header {{
            position: sticky;
            top: 20px;
            margin: 20px auto;
            max-width: 1200px;
            padding: 15px 30px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            background: var(--glass-bg);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            border: 1px solid var(--glass-border);
            border-radius: 16px;
            z-index: 10;
        }}
        .header a {{
            color: var(--text-color);
            text-decoration: none;
            font-weight: 600;
            display: flex;
            align-items: center;
        }}
        .header svg {{
            width: 20px;
            height: 20px;
            margin-right: 8px;
        }}
        .main-container {{
            padding: 0 40px 40px 40px;
            max-width: 800px;
            margin: 0 auto;
        }}
        .content-card {{
            background: var(--glass-bg);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            border-radius: 20px;
            padding: 40px;
            border: 1px solid var(--glass-border);
        }}
        h2 {{
            font-size: 2.2rem;
            font-weight: 700;
            margin-top: 0;
            margin-bottom: 30px;
        }}
        .input-group {{
            margin-bottom: 25px;
            text-align: left;
        }}
        .input-group label {{
            display: block;
            margin-bottom: 10px;
            font-weight: 600;
            color: var(--text-muted-color);
        }}
        .input-group input {{
            width: 100%;
            padding: 14px;
            background-color: rgba(0,0,0,0.3);
            border: 1px solid var(--glass-border);
            border-radius: 12px;
            color: var(--text-color);
            font-size: 1rem;
            box-sizing: border-box;
            transition: border-color 0.3s ease, box-shadow 0.3s ease;
        }}
        .input-group input:focus {{
            outline: none;
            border-color: var(--primary-color);
            box-shadow: 0 0 0 4px rgba(0, 123, 255, 0.25);
        }}
        .input-group input[readonly] {{
            background-color: rgba(0,0,0,0.2);
            cursor: not-allowed;
        }}
        .btn {{
            width: 100%;
            padding: 14px;
            background-color: var(--primary-color);
            border: none;
            border-radius: 12px;
            color: #ffffff;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: background-color 0.3s ease, transform 0.2s ease;
        }}
        .btn:hover {{
            background-color: #0056b3;
            transform: translateY(-2px);
        }}
        @keyframes fadeIn {{ to {{ opacity: 1; }} }}
    </style>
</head>
<body>
<div class="page-wrapper">
    <header class="header">
        <a href="/cgi/welcome.py">
            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M15.75 19.5L8.25 12l7.5-7.5" /></svg>
            Back to Dashboard
        </a>
        <span>{title}</span>
    </header>
    <main class="main-container">
        <div class="content-card">
            {content}
        </div>
    </main>
</div>
</body>
</html>'''

def main():
    session_id = get_session_id()
    if not session_id:
        print('Status: 302 Found')
        print('Location: /cgi/login.py')
        print()
        return

    title = "Manage Profile"
    content = '''
        <h2>User Profile</h2>
        <form>
            <div class="input-group">
                <label for="username">Username</label>
                <input type="text" id="username" name="username" value="admin" readonly>
            </div>
            <div class="input-group">
                <label for="fullname">Full Name</label>
                <input type="text" id="fullname" name="fullname" value="Administrator">
            </div>
            <div class="input-group">
                <label for="email">Email Address</label>
                <input type="email" id="email" name="email" value="admin@webserv.com">
            </div>
            <button type="submit" class="btn">Save Changes</button>
        </form>
    '''
    
    body = render_page(title, content, session_id)
    body_bytes = body.encode('utf-8')
    print('Content-Type: text/html; charset=utf-8')
    print(f'Content-Length: {len(body_bytes)}')
    print()
    sys.stdout.buffer.write(body_bytes)

if __name__ == '__main__':
    main()
