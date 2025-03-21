from Crypto.PublicKey import ECC
from Crypto.Signature import DSS
from Crypto.Hash import SHA256
import binascii

def validate_p256_public_key(pubkey_bytes):
    """Validate and construct a P-256 public key from raw bytes"""
    # Split into X and Y components (32 bytes each)
    x_bytes = bytes(pubkey_bytes[:32])
    y_bytes = bytes(pubkey_bytes[32:])
    
    print(f"X coordinate: {x_bytes.hex()}")
    print(f"Y coordinate: {y_bytes.hex()}")
    
    # Convert to integers
    x = int.from_bytes(x_bytes, byteorder='big')
    y = int.from_bytes(y_bytes, byteorder='big')
    
    try:
        # Create a point on the P-256 curve
        curve_name = 'P-256'
        
        # Create a public key from the point coordinates
        key = ECC.construct(curve=curve_name, point_x=x, point_y=y)
        
        # Get the key in a standard format to verify it worked
        key_der = key.export_key(format='DER')
        print(f"Successfully created key object (DER length: {len(key_der)} bytes)")
        
        return True, key
        
    except Exception as e:
        return False, f"Invalid key: {str(e)}"

def verify_ecdsa_signature(public_key, signature_bytes, message_bytes):
    """Verify an ECDSA signature against a message using the provided public key"""
    try:
        # Create a verifier object
        verifier = DSS.new(public_key, 'fips-186-3')
        
        # Create hash of the message
        h = SHA256.new(message_bytes)
        
        # Verify the signature
        verifier.verify(h, signature_bytes)
        return True, "Signature is valid"
        
    except Exception as e:
        return False, f"Signature verification failed: {str(e)}"

def parse_hex_bytes(hex_string):
    """Parse a string of comma-separated hex values (0xXX format) into a list of bytes"""
    # Clean up the input string
    hex_string = hex_string.strip()
    
    # Handle if there's trailing comma
    if hex_string.endswith(','):
        hex_string = hex_string[:-1]
    
    # Split by comma and convert each hex value to an integer
    bytes_list = []
    for byte in hex_string.split(','):
        byte = byte.strip()
        if byte:  # Skip empty entries
            bytes_list.append(int(byte, 0))  # The 0 base allows for '0x' prefix
    
    return bytes_list

def parse_hex_string(hex_string):
    """Convert a hex string (with or without 0x prefix) to bytes"""
    # Remove 0x prefix if present
    if hex_string.startswith('0x'):
        hex_string = hex_string[2:]
    
    # Remove spaces and convert to bytes
    hex_string = hex_string.replace(' ', '')
    return binascii.unhexlify(hex_string)

if __name__ == "__main__":
    print("ECDSA P-256 Signature Verification")
    print("==================================")
    
    # Ask for public key input
    print("\nEnter your ECC P-256 public key as comma-separated hex values (0xXX, 0xYY, ...) or a hex string:")
    pubkey_input = input()
    
    try:
        # Determine the input format and parse accordingly
        if ',' in pubkey_input:
            # Comma-separated format
            pubkey_bytes = parse_hex_bytes(pubkey_input)
        else:
            # Hex string format
            pubkey_bytes = parse_hex_string(pubkey_input)
        
        # Verify we have the right number of bytes
        if len(pubkey_bytes) != 64:
            print(f"Error: Public key must be exactly 64 bytes (got {len(pubkey_bytes)} bytes)")
            exit(1)
            
        # Validate the public key
        valid, key_or_error = validate_p256_public_key(pubkey_bytes)
        
        if not valid:
            print(f"Error: {key_or_error}")
            exit(1)
            
        public_key = key_or_error  # If valid, this contains the key object
        
        # Ask for signature input
        print("\nEnter the signature (in hex format, with or without 0x prefix):")
        signature_input = input()
        
        # Parse the signature
        try:
            if ',' in signature_input:
                signature_bytes = bytes(parse_hex_bytes(signature_input))
            else:
                signature_bytes = parse_hex_string(signature_input)
                
            print(f"Signature parsed: {len(signature_bytes)} bytes")
        except Exception as e:
            print(f"Error parsing signature: {str(e)}")
            exit(1)
            
        # Ask for the message to verify
        print("\nEnter the message to verify ():")
        message = input()
        message_bytes = message.encode('utf-8')
        
        # Verify the signature
        valid, result = verify_ecdsa_signature(public_key, signature_bytes, message_bytes)
        
        if valid:
            print("\n✅ SUCCESS: Signature verification passed!")
        else:
            print(f"\n❌ FAILED: {result}")
            
    except Exception as e:
        print(f"Error: {str(e)}")