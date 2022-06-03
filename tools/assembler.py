#! /usr/bin/python3.10
from sys import argv
import os


## File loading 
assert len(argv) != 1
file = argv[1]

assert os.path.exists(argv[1])
assert file.endswith(".ch8s")

with open(file, "r") as f:
    text = f.read().split("\n")


## output array
output = bytearray()
def outadd(x):
    output.append(x // 256);
    output.append(x % 256);


## Things for reading
bad_programmer = lambda l: exit(f"bad code at line {l+1}")
reg_to_int = lambda reg: int(reg[-1], base=16) * 0x0100
def read_n(n):
    if n.startswith('0x'):
        return int(n[2:], base=16)
    elif n.startswith('0b'):
        return int(n[2:], base=2)
    else:
        return int(n)
labels = {}
lines_to_resolve = []
line_offset = 0


## The assembler itself
for l, line in enumerate(text):
    # Remove contents if any
    if '#' in line:
        line = line[:line.index('#')]

    # Split line
    line = line.lstrip()
    parts = line.split(" ")

    # Empty line
    if len(parts) == 0 or parts[0] == '':
        line_offset += 1
        continue

    # Label 
    if parts[0].endswith(":"):
        labels[parts.pop(0)[:-1]] = 512 + len(output)

    # Empty now?
    if len(parts) == 0:
        line_offset += 1
        continue

    # Check if its raw hex
    if parts[0].startswith("0x"):
        for byte in parts:
            output.append(int(byte, base=16))
        continue

    # Get instruction 
    opcode = parts.pop(0).upper()
    operand = "" 
    if len(parts): operand = "".join(parts)
    op = operand.split(',')

    # Match instruction
    match opcode:
        case "CLS":
            outadd(0x00E0)

        case "RET":
            outadd(0x00EE)

        case "EXIT":
            outadd(0x00FD)

        case "JP":
            if operand.isnumeric():
                outadd(0x1000 + read_n(operand)) 
            else:
                outadd(0x1000)
                lines_to_resolve.append([l - line_offset, operand])

        case "CALL":
            if operand.isnumeric():
                outadd(0x2000 + read_n(operand))
            else:
                outadd(0x2000)
                lines_to_resolve.append([l - line_offset, operand])

        case "SE":
            if op[1].startswith('V'):
                outadd(0x5000 + reg_to_int(op[0]) + (reg_to_int(op[1]) >> 4))
            else:
                outadd(0x3000 + reg_to_int(op[0]) + read_n(op[1]))

        case "SNE":
            if op[1].startswith('V'):
                outadd(0x9000 + reg_to_int(op[0]) + (reg_to_int(op[1]) >> 4))
            else:
                outadd(0x4000 + reg_to_int(op[0]) + read_n(op[1]))

        case "LD":
            # 8xy0 - LD Vx, Vy 
            if op[0].startswith('V') and op[1].startswith('V'):
                outadd(0x8000 + reg_to_int(op[0]) + (reg_to_int(op[1]) >> 4))

            # Annn - LD I, nnn
            elif op[0] == 'I':
                if op[1].isnumeric():
                    outadd(0xA000 + read_n(op[1])) 
                else:
                    outadd(0xA000)
                    lines_to_resolve.append([l - line_offset, op[1]])

            # Fx15 - LD DT, Vx
            elif op[0] == 'DT':
                outadd(0xF015 + reg_to_int(op[1]))
                
            # Fx07 - LD Vx, DT 
            elif op[1] == 'DT':
                outadd(0xF007 + reg_to_int(op[0]))

            # Fx18 - LD ST, Vx
            elif op[0] == 'ST':
                outadd(0xF018 + reg_to_int(op[1]))

            # Fx1B - LD C, Vx
            elif op[0] == 'C':
                outadd(0xF01B + reg_to_int(op[1]))

            # Fx0A - LD Vx, K
            elif op[1] == 'K':
                outadd(0xF00A + reg_to_int(op[0]))

            # Fx29 - LD F, Vx
            elif op[0] == 'F':
                outadd(0xF029 + reg_to_int(op[1]))

            # Fx33 - LD B, Vx
            elif op[0] == 'B':
                outadd(0xF033 + reg_to_int(op[1]))

            # Fx55 - LD [I], Vx
            elif op[0] == '[I]':
                outadd(0xF055 + reg_to_int(op[1]))

            # Fx65 - LD Vx, [I]
            elif op[1] == '[I]':
                outadd(0xF065 + reg_to_int(op[0]))

            # 6xkk - LD Vx, kk
            else:
                outadd(0x6000 + reg_to_int(op[0]) + read_n(op[1]))

        case "ADD":
            if op[0].startswith('V') and op[1].startswith('V'):
                outadd(0x8002 + reg_to_int(op[0]) + reg_to_int(op[1]))
            elif op[0].startswith('V'):
                outadd(0x7000 + reg_to_int(op[0]) + read_n(op[1]))
            else:
                outadd(0xF01E + reg_to_int(op[0]))

        case "OR":
            outadd(0x8001 + reg_to_int(op[0]) + (reg_to_int(op[1]) >> 4))

        case "AND":
            outadd(0x8002 + reg_to_int(op[0]) + (reg_to_int(op[1]) >> 4))

        case "XOR":
            outadd(0x8003 + reg_to_int(op[0]) + (reg_to_int(op[1]) >> 4))

        case "SUB":
            outadd(0x8005 + reg_to_int(op[0]) + (reg_to_int(op[1]) >> 4))

        case "SHR":
            outadd(0x8006 + reg_to_int(op[0]) + (reg_to_int(op[1]) >> 4))

        case "SUBN":
            outadd(0x8007 + reg_to_int(op[0]) + (reg_to_int(op[1]) >> 4))

        case "SHL":
            outadd(0x800E + reg_to_int(op[0]) + (reg_to_int(op[1]) >> 4))

        case "RND": 
            outadd(0xC000 + reg_to_int(op[0]) + read_n(op[1]))
            
        case "DRW":
            outadd(0xD000 + reg_to_int(op[0]) + (reg_to_int(op[1]) >> 2) + read_n(op[2]))

        case "SKP":
            outadd(0xE09E + reg_to_int(op[0]))

        case "SKNP":
            outadd(0xE0A1 + reg_to_int(op[0]))

        case _:
            bad_programmer(l)

# Resolve missing labels
for line, label in lines_to_resolve:
    new_byte = (output[line * 2] << 8) + labels[label]
    output[line*2] = int(new_byte >> 8)
    output[line*2+1] = new_byte & 0x00FF


# Write the file
with open(f"{file[:-1]}", "wb") as f:
    f.write(output)

print(len(output))

