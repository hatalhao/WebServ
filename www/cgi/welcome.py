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
        # User is logged in - show the new glassmorphism-style dashboard
        print('Content-Type: text/html; charset=utf-8')
        
        body = f'''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webserv Dashboard</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap" rel="stylesheet">
    <style>
        :root {{
            --text-color: #f0f0f0;
            --text-muted-color: #a0a0a0;
            --glass-bg: rgba(22, 22, 22, 0.4);
            --glass-border: rgba(255, 255, 255, 0.1);
            --glow-color: rgba(0, 123, 255, 0.5);
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
        .header h1 {{
            font-size: 1.5rem;
            margin: 0;
            font-weight: 700;
        }}
        .logout-btn {{
            padding: 10px 20px;
            background: rgba(220, 53, 69, 0.3);
            border: 1px solid rgba(220, 53, 69, 0.8);
            border-radius: 12px;
            color: #ffffff;
            font-size: 0.9rem;
            font-weight: 600;
            cursor: pointer;
            text-decoration: none;
            transition: background-color 0.3s ease;
        }}
        .logout-btn:hover {{
            background-color: rgba(220, 53, 69, 0.6);
        }}
        .main-container {{
            padding: 0 40px 40px 40px;
            max-width: 1200px;
            margin: 0 auto;
        }}
        .welcome-message h2 {{
            font-size: 2.8rem;
            font-weight: 700;
            margin-top: 20px;
            margin-bottom: 5px;
        }}
        .welcome-message p {{
            font-size: 1.2rem;
            color: var(--text-muted-color);
            margin-bottom: 50px;
        }}
        .dashboard-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 25px;
        }}
        a.card {{
            display: block;
            background: var(--glass-bg);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            border-radius: 20px;
            padding: 30px;
            border: 1px solid var(--glass-border);
            text-decoration: none;
            color: inherit;
            transition: transform 0.3s ease, box-shadow 0.3s ease, background 0.3s ease;
        }}
        a.card:hover {{
            transform: translateY(-8px);
            box-shadow: 0 10px 30px rgba(0,0,0,0.3), 0 0 20px var(--glow-color);
            background: rgba(30, 30, 30, 0.5);
        }}
        .card-icon {{
            width: 40px;
            height: 40px;
            margin-bottom: 20px;
            color: #007bff;
        }}
        .card h3 {{
            margin-top: 0;
            font-size: 1.4rem;
            font-weight: 600;
            color: var(--text-color);
        }}
        .card p {{
            font-size: 0.95rem;
            line-height: 1.6;
            color: var(--text-muted-color);
        }}
        .footer {{
            text-align: center;
            padding: 30px;
            margin-top: 40px;
            color: var(--text-muted-color);
            font-size: 0.8rem;
        }}
        @keyframes fadeIn {{
            to {{
                opacity: 1;
            }}
        }}
    </style>
</head>
<body>
<div class="page-wrapper">
    <header class="header">
        <h1>Webserv</h1>
        <a href="/cgi/logout.py" class="logout-btn">Logout</a>
    </header>

    <main class="main-container">
        <div class="welcome-message">
            <h2>Dashboard</h2>
            <p>Welcome back, Admin. Your portal is active.</p>
        </div>

        <div class="dashboard-grid">
            <a href="/cgi/profile.py" class="card">
                <div class="card-icon">
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M15.75 6a3.75 3.75 0 11-7.5 0 3.75 3.75 0 017.5 0zM4.501 20.118a7.5 7.5 0 0114.998 0A17.933 17.933 0 0112 21.75c-2.676 0-5.216-.584-7.499-1.632z" /></svg>
                </div>
                <h3>Manage Profile</h3>
                <p>Update personal info and manage your account settings.</p>
            </a>
            <a href="/cgi/analytics.py" class="card">
                <div class="card-icon">
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M3.75 3v11.25A2.25 2.25 0 006 16.5h2.25M3.75 3h-1.5m1.5 0h16.5m0 0h1.5m-1.5 0v11.25A2.25 2.25 0 0118 16.5h-2.25m-7.5 0h7.5m-7.5 0l-1 1.5m-5-1.5l1-1.5m-7.5 0h3.75a3 3 0 013 3v9.75a1.5 1.5 0 01-1.5 1.5h-7.5a1.5 1.5 0 01-1.5-1.5V6.75a3 3 0 013-3z" /></svg>
                </div>
                <h3>View Analytics</h3>
                <p>Access traffic reports and server performance data.</p>
            </a>
            <a href="/cgi/status.py" class="card">
                <div class="card-icon">
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M7.5 14.25v2.25m3-4.5v4.5m3-6.75v6.75m3-9v9M6 20.25h12A2.25 2.25 0 0020.25 18V6A2.25 2.25 0 0018 3.75H6A2.25 2.25 0 003.75 6v12A2.25 2.25 0 006 20.25z" /></svg>
                </div>
                <h3>System Status</h3>
                <p>Check the real-time status of services and server load.</p>
            </a>
            <a href="/cgi/upload.py" class="card">
                <div class="card-icon">
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M12 16.5V9.75m0 0l-3.75 3.75M12 9.75l3.75 3.75M3.75 12h16.5" /></svg>
                </div>
                <h3>Upload Files</h3>
                <p>Securely upload documents and files to the server.</p>
            </a>
        </div>
    </main>

    <footer class="footer">
        Session ID: {session_id}
    </footer>
</div>
</body>
</html>'''
        
        print(f'Content-Length: {len(body.encode("utf-8"))}')
        print()
        sys.stdout.flush()
        sys.stdout.buffer.write(body.encode('utf-8'))
    else:
        # User is not logged in - redirect to login
        print('Status: 302 Found')
        print('Location: /cgi/login.py')
        print('Content-Length: 0')
        print()

if __name__ == '__main__':
    main()