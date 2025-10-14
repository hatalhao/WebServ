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
            max-width: 1000px;
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
        .stats-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-bottom: 40px;
        }}
        .stat-card {{
            background: rgba(0,0,0,0.2);
            padding: 20px;
            border-radius: 12px;
            border: 1px solid var(--glass-border);
        }}
        .stat-card .label {{
            font-size: 0.9rem;
            color: var(--text-muted-color);
            margin-bottom: 8px;
        }}
        .stat-card .value {{
            font-size: 2rem;
            font-weight: 700;
        }}
        .chart-placeholder {{
            height: 300px;
            background: rgba(0,0,0,0.2);
            border: 1px solid var(--glass-border);
            border-radius: 12px;
            display: flex;
            justify-content: center;
            align-items: center;
            color: var(--text-muted-color);
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
    sys.stdout.buffer.write(body_bytes)

if __name__ == '__main__':
    main()
