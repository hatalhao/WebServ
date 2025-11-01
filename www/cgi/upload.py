#!/usr/bin/python3
import os
import sys
import html

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

def handle_status_response(status_code):
    """Generate content based on upload status code"""
    if status_code == '201':
        title = "Upload Files"
        content = '''
            <div class="message-icon success">
                <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" d="M9 12.75L11.25 15 15 9.75M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
                </svg>
            </div>
            <h2>Upload Successful</h2>
            <p class="message-text">Your file has been uploaded successfully.</p>
            <a href="/cgi/upload.py" class="btn">Upload Another File</a>
        '''
    elif status_code == '413':
        title = "Upload Error"
        content = '''
            <div class="message-icon error">
                <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" d="M12 9v3.75m9-.75a9 9 0 11-18 0 9 9 0 0118 0zm-9 3.75h.008v.008H12v-.008z" />
                </svg>
            </div>
            <h2>File Too Large</h2>
            <p class="message-text">The uploaded file exceeds the maximum allowed size.</p>
            <a href="/cgi/upload.py" class="btn">Try Again</a>
        '''
    elif status_code == '409':
        title = "Upload Error"
        content = '''
            <div class="message-icon error">
                <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" d="M12 9v3.75m9-.75a9 9 0 11-18 0 9 9 0 0118 0zm-9 3.75h.008v.008H12v-.008z" />
                </svg>
            </div>
            <h2>Upload Failed</h2>
            <p class="message-text">Could not save the file. The upload directory might be full or not writable.</p>
            <a href="/cgi/upload.py" class="btn">Try Again</a>
        '''
    else:
        title = "Upload Files"
        content = '''
            <h2>Secure File Upload</h2>
            <form action="/uploads/" method="post" enctype="multipart/form-data" id="uploadForm">
                <div class="upload-area" id="uploadArea">
                    <div class="icon">
                        <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor">
                            <path stroke-linecap="round" stroke-linejoin="round" d="M12 16.5V9.75m0 0l-3.75 3.75M12 9.75l3.75 3.75M3.75 12h16.5" />
                        </svg>
                    </div>
                    <div class="text" id="uploadText">Click to browse or drag & drop files</div>
                    <div class="subtext">Max file size: 50MB</div>
                </div>
                <input type="file" id="fileInput" name="uploadedFile" required>
                <button type="submit" class="btn">Upload File</button>
            </form>
            <script>
                const uploadArea = document.getElementById('uploadArea');
                const fileInput = document.getElementById('fileInput');
                const uploadText = document.getElementById('uploadText');
                const form = document.getElementById('uploadForm');

                // Click to upload
                uploadArea.addEventListener('click', () => fileInput.click());

                // Form submission handler
                form.onsubmit = async (e) => {
                    e.preventDefault();
                    const submitButton = form.querySelector('button[type="submit"]');
                    const originalButtonText = submitButton.textContent;
                    
                    try {
                        // Show loading state
                        submitButton.textContent = 'Uploading...';
                        submitButton.disabled = true;
                        
                        const response = await fetch(form.action, {
                            method: 'POST',
                            body: new FormData(form)
                        });

                        if (response.ok) {
                            window.location.href = '/cgi/upload.py?status=201';
                        } else {
                            const statusMap = {
                                413: '413', // File too large
                                409: '409', // Conflict/Save error
                                500: '500', // Server error
                                400: '400'  // Bad request
                            };
                            window.location.href = `/cgi/upload.py?status=${statusMap[response.status] || '500'}`;
                        }
                    } catch (error) {
                        window.location.href = '/cgi/upload.py?status=500';
                    } finally {
                        submitButton.textContent = originalButtonText;
                        submitButton.disabled = false;
                    }
                };

                // File input change handler
                fileInput.addEventListener('change', () => {
                    if (fileInput.files.length > 0) {
                        const file = fileInput.files[0];
                        const maxSize = 50 * 1024 * 1024; // 50MB

                        if (file.size > maxSize) {
                            alert('File size exceeds 50MB limit. Please select a smaller file.');
                            fileInput.value = '';
                            uploadText.textContent = 'Click to browse or drag & drop files';
                        } else {
                            uploadText.textContent = `File selected: ${file.name}`;
                        }
                    }
                });

                // Drag and drop handlers
                ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {
                    uploadArea.addEventListener(eventName, preventDefaults);
                    document.body.addEventListener(eventName, preventDefaults);
                });

                function preventDefaults(e) {
                    e.preventDefault();
                    e.stopPropagation();
                }

                // Handle drag and drop visual feedback
                ['dragenter', 'dragover'].forEach(eventName => {
                    uploadArea.addEventListener(eventName, () => {
                        uploadArea.classList.add('drag-over');
                    });
                });

                ['dragleave', 'drop'].forEach(eventName => {
                    uploadArea.addEventListener(eventName, () => {
                        uploadArea.classList.remove('drag-over');
                    });
                });

                // Handle dropped files
                uploadArea.addEventListener('drop', (e) => {
                    const dt = e.dataTransfer;
                    const file = dt.files[0];
                    
                    if (file) {
                        const maxSize = 50 * 1024 * 1024; // 50MB
                        if (file.size > maxSize) {
                            alert('File size exceeds 50MB limit. Please select a smaller file.');
                        } else {
                            fileInput.files = dt.files;
                            uploadText.textContent = `File selected: ${file.name}`;
                        }
                    }
                });
            </script>
        '''
    return title, content

def handle_get_request(session_id):
    """Renders the initial file upload form."""
    query_string = os.environ.get('QUERY_STRING', '')
    params = dict(param.split('=') for param in query_string.split('&') if '=' in param)
    status = params.get('status', '')
    title, content = handle_status_response(status)
    return render_page(title, content, session_id)

def handle_post_request(session_id):
    """Handle file upload POST requests."""
    # This function will be implemented when we handle the actual file upload
    return render_page("Upload Error", '''
        <div class="message-icon error">
            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" d="M12 9v3.75m9-.75a9 9 0 11-18 0 9 9 0 0118 0zm-9 3.75h.008v.008H12v-.008z" />
            </svg>
        </div>
        <h2>Not Implemented</h2>
        <p class="message-text">Direct POST handling is not implemented. Please use the form submission.</p>
            <a href="/cgi/upload.py" class="btn">Try Again</a>
    ''', session_id)

if __name__ == "__main__":
    try:
        session_id = get_session_id()
        if not session_id or not is_session_valid():
            sys.stdout.buffer.write(b"Status: 401 Unauthorized\r\n")
            sys.stdout.buffer.write(b"Content-Type: text/html\r\n\r\n")
            sys.stdout.buffer.write(b"<h1>401 Unauthorized</h1>")
            sys.exit(0)

        request_method = os.environ.get('REQUEST_METHOD', 'GET')
        
        if request_method == 'GET':
            response = handle_get_request(session_id)
        elif request_method == 'POST':
            response = handle_post_request(session_id)
        else:
            sys.stdout.buffer.write(b"Status: 405 Method Not Allowed\r\n")
            sys.stdout.buffer.write(b"Content-Type: text/plain\r\n\r\n")
            sys.stdout.buffer.write(b"Method Not Allowed")
            sys.exit(0)

        # Convert response to bytes and get length
        response_bytes = response.encode('utf-8')
        content_length = len(response_bytes)

        # Write headers
        sys.stdout.buffer.write(b"Content-Type: text/html; charset=utf-8\r\n")
        sys.stdout.buffer.write(f"Content-Length: {content_length}\r\n".encode('utf-8'))
        sys.stdout.buffer.write(b"\r\n")
        
        # Write response body
        sys.stdout.buffer.write(response_bytes)
        sys.stdout.flush()

    except Exception as e:
        error_message = f"<h1>500 Internal Server Error</h1><p>{html.escape(str(e))}</p>"
        error_bytes = error_message.encode('utf-8')
        
        sys.stdout.buffer.write(b"Status: 500 Internal Server Error\r\n")
        sys.stdout.buffer.write(b"Content-Type: text/html; charset=utf-8\r\n")
        sys.stdout.buffer.write(f"Content-Length: {len(error_bytes)}\r\n".encode('utf-8'))
        sys.stdout.buffer.write(b"\r\n")
        sys.stdout.buffer.write(error_bytes)
        sys.stdout.flush()