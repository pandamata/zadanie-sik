# HTTPS Server - Fixed Version

This repository contains a corrected HTTPS server implementation in C using OpenSSL.

## What Was Fixed

The server was upgraded from a "bad" version (that worked with curl but failed in browsers) to a fully compliant version that works perfectly in Chrome and Firefox.

### 8 Major Improvements Applied:

1. **Modern OpenSSL Initialization** - Removed deprecated functions
2. **Secure TLS Configuration** - Using TLS 1.2+ only
3. **Better Socket Options** - Removed problematic SO_REUSEPORT
4. **Proper Error Handling** - Clean up on SSL handshake failures
5. **Reliable Data Transmission** - Implemented send_all() with proper SSL error handling
6. **Complete HTTP Headers** - Content-Type with charset, Content-Length, Connection: close
7. **Efficient File Reading** - Single read operation using stat() + malloc + fread
8. **Graceful Connection Shutdown** - Proper SSL and socket cleanup sequence

## Building and Running

### Prerequisites
```bash
sudo apt update
sudo apt install libssl-dev
```

### Generate SSL Certificates
```bash
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout server.key -out server.crt \
  -subj "/C=PL/ST=Poznan/L=Poznan/O=UAM/OU=CS/CN=localhost"
```

### Compile
```bash
gcc -o server server.c -lssl -lcrypto -Wall -Wextra
```

### Run
```bash
./server
```

The server will listen on port 8443 (HTTPS).

## Testing

### With curl
```bash
curl -k https://localhost:8443/
```

### With OpenSSL client
```bash
openssl s_client -connect localhost:8443
```

### With a browser
Open `https://localhost:8443/` in Chrome or Firefox. You'll see a self-signed certificate warning (which is expected and normal for development). Accept it and the page will load correctly.

## Files

- `server.c` - Fixed HTTPS server implementation
- `index.html` - HTML page served by the server
- `FIXES_EXPLANATION.txt` - Detailed explanation of all fixes
- `.gitignore` - Excludes certificates and compiled binaries

## Expected Behavior

✅ No "Connection reset" errors  
✅ Proper HTTP headers with Content-Length  
✅ Clean TLS shutdown  
✅ Works in Chrome and Firefox  
✅ Only expected warning: self-signed certificate  

## Technical Details

See `FIXES_EXPLANATION.txt` for a comprehensive explanation of each problem and fix.
