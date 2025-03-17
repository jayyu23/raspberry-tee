from Crypto.PublicKey import ECC
from Crypto.Signature import DSS
from Crypto.Hash import SHA256
import base64

# Helper functions
def base64_to_hex(base64_str):
    """Convert a base64 string to hexadecimal string with 0x prefix"""
    if isinstance(base64_str, str):
        # Decode base64 to bytes
        decoded_bytes = base64.b64decode(base64_str)
        # Convert bytes to hex string with 0x prefix
        return '0x' + decoded_bytes.hex()
    elif isinstance(base64_str, bytes):
        # If already bytes, just convert to hex
        return '0x' + base64_str.hex()
    else:
        raise TypeError("Input must be a base64 string or bytes")
    
def generate_key_pair():
    """Generate a P-256 key pair and return private, public keys"""
    # Generate a new private key on the P-256 curve
    private_key = ECC.generate(curve='P-256')
    
    # Extract the public key
    public_key = private_key.public_key()
    
    # Serialize keys for storage/transmission
    private_bytes = private_key.export_key(format='PEM')
    public_bytes = public_key.export_key(format='PEM')
    
    return private_key, public_key, private_bytes, public_bytes

def sign_message(private_key, message):
    """Sign a message using the private key"""
    if isinstance(message, str):
        message = message.encode('utf-8')
    
    # Create a hash of the message
    hash_obj = SHA256.new(message)
    
    # Create a signature using Deterministic DSS (RFC 6979)
    signer = DSS.new(private_key, 'fips-186-3')
    signature = signer.sign(hash_obj)
    
    # Base64 encode for easy storage/transmission
    signature_b64 = base64.b64encode(signature).decode('utf-8')
    
    return signature, signature_b64

def verify_signature(public_key, message, signature):
    """Verify a signature using the public key"""
    if isinstance(message, str):
        message = message.encode('utf-8')
    
    if isinstance(signature, str):
        # Assume it's base64-encoded
        signature = base64.b64decode(signature)
    
    # Create a hash of the message
    hash_obj = SHA256.new(message)
    
    # Verify the signature
    verifier = DSS.new(public_key, 'fips-186-3')
    try:
        verifier.verify(hash_obj, signature)
        return True
    except ValueError as e:
        print(f"Verification failed: {e}")
        return False

# Example usage
if __name__ == "__main__":
    # Generate keys
    private_key, public_key, private_pem, public_pem = generate_key_pair()
    print(f"Private key: {base64_to_hex(private_pem)}")
    print(f"Public key: {base64_to_hex(public_pem)}")
    print("--------------------------------")
    print("Test sign message")
    # Sign a message
    message = "Hello, blockchain world!"
    signature, signature_b64 = sign_message(private_key, message)
    print(f"Message: {message}")
    print(f"Signed message: {base64_to_hex(signature_b64)}")
    print("--------------------------------")
    print("Test verify message")
    # Verify the signature
    is_valid = verify_signature(public_key, message, signature)
    print(f"Signature valid: {is_valid}")
    print("--------------------------------")
    print("Test tampered message")
    # Try with a tampered message
    tampered_message = "Hello, hacked blockchain world!"
    is_valid = verify_signature(public_key, tampered_message, signature)
    print(f"Tampered message signature valid: {is_valid}")