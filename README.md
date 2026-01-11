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

**IMPORTANT**: You must specify port 8443 in the URL!

**If testing on the same machine where the server runs:**
- Open `https://localhost:8443/` in Chrome or Firefox

**If testing from a different computer:**
- Replace `localhost` with the server's IP address: `https://SERVER_IP_ADDRESS:8443/`
- Example: `https://192.168.1.100:8443/`
- Make sure port 8443 is not blocked by firewall

**Expected behavior:**
1. **Firefox**: Will show "Warning: Potential Security Risk Ahead" with error code `MOZILLA_PKIX_ERROR_SELF_SIGNED_CERT`. Click "Advanced..." → "Accept the Risk and Continue"
2. **Chrome**: Will show "Your connection is not private" with `NET::ERR_CERT_AUTHORITY_INVALID`. Click "Advanced" → "Proceed to localhost (unsafe)"
3. After accepting the certificate warning, the HTML page will load correctly

**Common issues:**
- `ERR_CONNECTION_REFUSED` or `ERR_TIMED_OUT`: Server is not running, wrong IP address, or forgot to specify `:8443` port
- Certificate warning is EXPECTED for self-signed certificates - you must click through it

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

## Troubleshooting

### Chrome shows ERR_TIMED_OUT or "This site can't be reached"
- **Cause**: Not specifying port 8443 in URL, or firewall blocking the port
- **Solution**: Make sure to include `:8443` in the URL: `https://localhost:8443/` or `https://YOUR_SERVER_IP:8443/`
- **Check firewall**: If testing from another computer, ensure port 8443 is allowed through the firewall

### Firefox shows "uses an invalid security certificate" 
- **This is EXPECTED** - self-signed certificates trigger this warning
- **Solution**: Click "Advanced..." then "Accept the Risk and Continue"

### Chrome shows "Your connection is not private"
- **This is EXPECTED** - self-signed certificates trigger this warning  
- **Solution**: Click "Advanced" then "Proceed to localhost (unsafe)"

### Connection works with curl but not browsers
- **Make sure you're using the correct URL with port**: `https://localhost:8443/`
- **Browsers don't auto-detect port 8443** - you MUST specify it in the URL  

## Technical Details

See `FIXES_EXPLANATION.txt` for a comprehensive explanation of each problem and fix.
