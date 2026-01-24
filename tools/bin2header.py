import os
import argparse


def bin2header(input_path, output_path, array_name):
    if not os.path.exists(input_path):
        print(f"Error: {input_path} not found.")
        return

    with open(input_path, 'rb') as f:
        data = f.read()

    size = len(data)
    
    with open(output_path, 'w') as f:
        guard = f"{array_name.upper()}_H"
        f.write(f"#ifndef {guard}\n")
        f.write(f"#define {guard}\n\n")
        
        f.write("#include <array>\n\n")
        
        f.write(f"inline constexpr std::array<unsigned char, {size}> {array_name} = {{\n")
        
        for i, byte in enumerate(data):
            if i % 12 == 0:
                f.write("    ")
            f.write(f"0x{byte:02x}, ")
            if (i + 1) % 12 == 0:
                f.write("\n")
        
        if size % 12 != 0:
            f.write("\n")
            
        f.write("};\n\n")
        f.write(f"#endif // {guard}\n")
    
    print(f"Success: Generated '{output_path}' ({size} bytes)")


if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='Convert a binary file to a C header file with a byte array.')
	parser.add_argument('-i' ,'--input', help='Path to the input binary file')
	parser.add_argument('-o', '--output', help='Path to the output header file')
	parser.add_argument('-n', '--name', help='Name of the byte array in the header file')
	args = parser.parse_args()
	bin2header(args.input, args.output, args.name)