# WebServ

WebServ is a lightweight HTTP server designed to provide a fundamental understanding of how web servers operate and how fundamental web concepts can be implemented from scratch. This project focuses on core server-side networking, HTTP protocol handling, and basic web application logic, offering a practical exploration of essential backend technologies.

## Concepts Tackled and Implemented

### 1. Socket Programming & Asynchronous I/O

The server is built from the ground up using raw socket programming techniques, managing TCP connections directly. The implementation handles multiple client connections concurrently using event-driven mechanisms, efficiently managing requests without the need for multithreading.

### 2. HTTP Protocol Parsing

WebServ features a custom HTTP request parser that supports the main HTTP methods (GET, POST, DELETE). The request parsing logic interprets headers, query strings, and request bodies, providing robust error handling for malformed requests and protocol violations.

### 3. Response Generation

The server generates HTTP responses in compliance with the protocol standard, including proper status lines, headers (such as `Content-Type`, `Content-Length`, and `Connection`), and bodies. It supports various response codes and manages keep-alive connections.

### 4. Static & Dynamic Content Serving

WebServ can serve static files from a configurable document root, determining MIME types and handling path resolution securely. Additionally, it supports executing CGI scripts, enabling dynamic content generation and basic server-side scripting.

### 5. Configuration Management

The project includes a flexible configuration system, allowing users to define settings such as listening ports, server names, error pages, allowed methods, root directories, and more. The configuration is parsed at startup and guides the server's runtime behavior.

### 6. Error Handling & Custom Error Pages

The server implements comprehensive error handling for both client and server errors, returning appropriate HTTP error codes and customizable error pages for scenarios like `404 Not Found`, `405 Method Not Allowed`, and `500 Internal Server Error`.

### 7. Logging

WebServ includes access logging, tracking incoming requests and server responses. The logging system provides essential information for debugging and monitoring server activity.

### 8. Security Considerations

Path traversal protection, request size limits, and other basic security measures are implemented to mitigate common web server vulnerabilities and ensure safe file serving.

---

## Core Architectural Components

WebServ is structured around several core components, each implementing foundational architectural and design concepts:

- **Configuration Management**: Utilizes a dedicated module for parsing and storing server configuration options, ensuring that runtime parameters are accessible and validated.
- **Request Lifecycle Management**: Employs a multi-stage parsing and validation approach to handle HTTP requests, including support for state transitions and error detection.
- **Response Handling**: Implements response serialization and formatting based on HTTP protocol, with mechanisms for header management and body composition.
- **Client Connection Handling**: Manages the lifecycle and state of each client connection, including buffer management, file streaming, and integration with dynamic content generation.
- **Session and Cookie Management**: Incorporates session persistence and cookie handling to enable stateful interactions and user tracking.
- **Singleton Design Pattern**: The core server controller adopts the singleton pattern to ensure a single orchestrator instance manages all resources, sockets, and event loops throughout the server’s lifetime.
- **Event-driven Server Loop**: Relies on event-based architecture for scalable and efficient handling of concurrent network events, minimizing resource usage and latency.
- **CGI Integration**: Supports dynamic content generation via the Common Gateway Interface, enabling the execution of scripts and external programs in response to HTTP requests.

---

## Getting Started

1. **Clone the repository**:
   ```bash
   git clone https://github.com/hatalhao/WebServ.git
   cd WebServ
   ```

2. **Build**:
   Follow the instructions in the project's documentation or Makefile to compile the server.

3. **Configure**:
   Edit the configuration file to define server behavior to your needs.

4. **Run**:
   Launch the server and connect using your web browser or tools like `curl`.

---

## License

This project is released under the MIT License.
