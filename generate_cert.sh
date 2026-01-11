#!/bin/bash
# Script to generate SSL certificate with Subject Alternative Names (SANs)
# Required for Chrome compatibility

echo "Generating SSL certificate with SANs for localhost..."

# Create OpenSSL config file
cat > openssl.cnf << 'EOF'
[req]
default_bits = 2048
prompt = no
default_md = sha256
distinguished_name = dn
x509_extensions = v3_req

[dn]
C = PL
ST = Poznan
L = Poznan
O = UAM
OU = CS
CN = localhost

[v3_req]
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
DNS.2 = *.localhost
IP.1 = 127.0.0.1
IP.2 = ::1
EOF

# Generate certificate
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout server.key -out server.crt \
  -config openssl.cnf

# Verify SANs
echo ""
echo "Certificate generated successfully!"
echo ""
echo "Subject Alternative Names (required for Chrome):"
openssl x509 -in server.crt -text -noout | grep -A 1 "Subject Alternative Name"

# Clean up config file
rm openssl.cnf

echo ""
echo "Files created:"
echo "  - server.key (private key)"
echo "  - server.crt (certificate)"
echo ""
echo "You can now run the server with: ./server"
