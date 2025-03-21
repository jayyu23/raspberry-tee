from Crypto.PublicKey import ECC

def validate_p256_public_key(pubkey_bytes):
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
        # PyCryptodome uses the name 'P-256' or 'NIST P-256' for the SECP256R1 curve
        curve_name = 'P-256'
        
        # Create a public key from the point coordinates
        key = ECC.construct(curve=curve_name, point_x=x, point_y=y)
        
        # Get the key in a standard format to verify it worked
        key_der = key.export_key(format='DER')
        print(f"Successfully created key object (DER length: {len(key_der)} bytes)")
        
        # If we got here without exceptions, the key is valid
        return True, "Valid P-256 public key"
        
    except Exception as e:
        return False, f"Invalid key: {str(e)}"

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

if __name__ == "__main__":
    print("Validating ECC P-256 public key with PyCryptodome...")
    
    # Ask for public key input
    print("\nEnter your ECC P-256 public key as comma-separated hex values (0xXX, 0xYY, ...):")
    pubkey_input = input()
    
    # Parse the input into a list of bytes
    try:
        pubkey_bytes = parse_hex_bytes(pubkey_input)
        
        # Verify we have the right number of bytes
        if len(pubkey_bytes) != 64:
            print(f"Error: Public key must be exactly 64 bytes (got {len(pubkey_bytes)} bytes)")
        else:
            valid, message = validate_p256_public_key(pubkey_bytes)
            
            if valid:
                print("Success! The public key is valid.")
            else:
                print(f"Error: {message}")
    except Exception as e:
        print(f"Error parsing input: {str(e)}")