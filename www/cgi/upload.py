#!/usr/bin/python3
import os
import sys
import cgi
import html

# Define the upload directory relative to the script location
UPLOAD_DIR = './uploads'

TEMPLATE_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'html', 'upload.html')

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

def handle_get_request(session_id):
    """Renders the initial file upload form."""
    title = "Upload Files"
    content = '''
        <h2>Secure File Upload</h2>
        <form action="/cgi/upload.py" method="post" enctype="multipart/form-data">
            <div class="upload-area" id="uploadArea">
                <div class="icon">
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M12 16.5V9.75m0 0l-3.75 3.75M12 9.75l3.75 3.75M3.75 12h16.5" /></svg>
                </div>
                <div class="text" id="uploadText">Click to browse or drag & drop files</div>
                <div class="subtext">Max file size: 50MB</div>
            </div>
            <input type="file" id="fileInput" name="uploadedFile">
            <button type="submit" class="btn">Upload File</button>
        </form>
    '''
    return render_page(title, content, session_id)

def handle_post_request(session_id):
    """Handles the file upload logic."""
    title = "Upload Status"
    
    # Create the uploads directory if it doesn't exist
    os.makedirs(UPLOAD_DIR, exist_ok=True)
    
    form = cgi.FieldStorage()
    
    message = ""
    if 'uploadedFile' in form:
        fileitem = form['uploadedFile']
        if fileitem.filename:
            # Sanitize filename to prevent directory traversal attacks
            filename = os.path.basename(fileitem.filename)
            filepath = os.path.join(UPLOAD_DIR, filename)
            
            with open(filepath, 'wb') as f:
                f.write(fileitem.file.read())
            
            safe_filename = html.escape(filename)
            message = f'''
                <div class="message-icon">
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M9 12.75L11.25 15 15 9.75M21 12a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
                </div>
                <h2>Upload Successful</h2>
                <p class="message-text">Your file '<b>{safe_filename}</b>' has been saved.</p>
            '''
        else:
            message = "<h2>Upload Failed</h2><p class='message-text'>No file was selected.</p>"
    else:
        message = "<h2>Upload Failed</h2><p class='message-text'>Invalid form data received.</p>"

    content = f'''
        {message}
        <a href="/cgi/upload.py" class="btn">Upload Another File</a>
    '''
    return render_page(title, content, session_id)

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

    request_method = os.environ.get('REQUEST_METHOD', 'GET')
    
    if request_method == 'POST':
        body = handle_post_request(session_id)
    else:
        body = handle_get_request(session_id)
    
    body_bytes = body.encode('utf-8')
    print('Content-Type: text/html; charset=utf-8')
    print(f'Content-Length: {len(body_bytes)}')
    print()
    sys.stdout.flush()
    sys.stdout.buffer.write(body_bytes)

if __name__ == '__main__':
    main()

