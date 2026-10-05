import secrets
import base64

# Generate 32 random bytes
random_bytes = secrets.token_bytes(32)

# Encode to base64
api_token = base64.b64encode(random_bytes).decode('utf-8')

print(api_token)