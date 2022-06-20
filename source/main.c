/// Includes
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <citro2d.h>


u_int8_t keymap[17] = {
    0x1, 0x2, 0x3, 0xC,
    0x4, 0x5, 0x6, 0xD,
    0x7, 0x8, 0x9, 0xE,
    0xA, 0x0, 0xB, 0xF,
};

uint8_t memory[4096] = 
{
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80,
};

uint8_t registers[16];

uint8_t stack[32];

uint16_t I, PC = 0x200;
uint8_t DT = 0, ST = 0, C = 0xFF, SP = 0;


bool XORpixel(u8* fb, int x, int y)
{
    x %= 64;
    y %= 32;
    x = (x * 6) + 6;
    y = (y * 6) + 22;
    int n = (240 * x + (240 - y)) * 3;
    bool xored = (fb[n] + fb[n+1] + fb[n+2]) != 0;

    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            int _n = n + 3 * (i + (j*240)); 
            if (xored)
                fb[_n] = fb[_n + 1] = fb[_n + 2] = 0;
            else
            {
                // C: rrrggbbb
                fb[_n + 2] = (int)(((C >> 5) & 7) / 7.0 * 255);
                fb[_n + 1] = (int)(((C >> 3) & 3) / 3.0 * 255);
                fb[_n] =     (int)(((C >> 0) & 7) / 7.0 * 255);

            }
        }
    }

    return xored;
}

u_int8_t WaitKey()
{
    touchPosition pos;
    uint8_t key;

    do
    {
        hidScanInput();
        hidTouchRead(&pos);
    } while (pos.px == 0 && pos.py == 0);
    key = pos.px / 80 + (pos.py / 60) * 4;

    return keymap[key];
}

bool IsDown(u_int8_t k)
{
    touchPosition pos;
    uint8_t key;

    hidScanInput();
    hidTouchRead(&pos);

    if (pos.px == 0 || pos.py == 0)
        return false;

    key = pos.px / 80 + (pos.py / 60) * 4;
    return (k == key);
}


int main(int argc, char **argv)
{
    // Init gfx
    gfxInitDefault();

    // Botton screen is terminal for printing the touch
    consoleInit(GFX_BOTTOM, NULL);

    // Disable buffer swaping and get buffer
    gfxSetDoubleBuffering(GFX_TOP, false);
    u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

    // Read binary from memory
    FILE *file = fopen("chip8-rom.ch8", "rb");
    if (file == NULL)
    {
        gfxExit();
        return -1;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    fread(memory + PC, 1, size, file);
    fclose(file);

    // Log file
    FILE *log = fopen("chip8.log", "w");
    if (log == NULL)
    {
        gfxExit();
        return -1;
    }

    // Print bottom screen
    printf(
            "|         |         |         |        |\n"
            "|    1    |    2    |    3    |   C    |\n"
            "|         |         |         |        |\n"
            "+---------+---------+---------+--------+\n"
            "|         |         |         |        |\n"
            "|    4    |    5    |    6    |   D    |\n"
            "|         |         |         |        |\n"
            "+---------+---------+---------+--------+\n"
            "|         |         |         |        |\n"
            "|    7    |    8    |    9    |   E    |\n"
            "|         |         |         |        |\n"
            "+---------+---------+---------+--------+\n"
            "|         |         |         |        |\n"
            "|    A    |    0    |    B    |   F    |\n"
            "|         |         |         |        |"
    );
    

    // Main loop
    uint16_t instruction;
    uint16_t nnn;
    uint8_t n, x, y, kk;
    bool running = true;
    bool valid_opcode = true;

    while (aptMainLoop() && running)
    {
        // Fetch
        instruction = memory[PC];
        instruction <<= 8;
        instruction += memory[PC + 1];

        nnn = instruction & 0x0FFF;
        n = instruction & 0x000F;
        x = (instruction & 0x0F00) >> 8;
        y = (instruction & 0x00F0) >> 4;
        kk = instruction & 0x00FF;

        // Increment PC
        PC+= 2;

        // Decrement counters if not 0
        DT = (DT == 0) ? 0 : DT - 1;
        ST = (ST == 0) ? 0 : ST - 1;

        // Print instruction to log 
        fprintf(log, "0x%04hx  %04hx  ", PC, instruction);

        // Read frame input
        hidScanInput();
        u32 kheld = hidKeysHeld();
        
        // Exit if start is holded
        if (kheld & KEY_START)
            running = false;
        
        // Instruction decoder 
        valid_opcode = true;
        switch (instruction & 0xF000)
        {
            case 0x0000:
                switch (instruction & 0x00FF)
                {
                    case 0x0000:            // NULL
                        fprintf(log, "NULL\n");
                        break;

                    case 0x00E0:            // CLS
                        fprintf(log, "CLS\n");
                        for (int i = 0; i < 400*240; i++)
                            fb[i*3] = fb[i*3 + 1] = fb[i*3 + 2] = 0;
                        break;

                    case 0x00EE:            // RET
                        fprintf(log, "RET\n");
                        PC = stack[SP--];
                        PC <<= 8;
                        PC += stack[SP--];
                        break;

                    case 0x00FD:            // EXIT
                        fprintf(log, "EXIT\n");
                        running = false;
                        break;

                    default:
                        valid_opcode = false;
                }
                break;

            case 0x1000:                // JP nnn 
                fprintf(log, "JP %04hx\n", nnn);
                PC = nnn;
                break;

            case 0x2000:                // CALL nnn 
                fprintf(log, "CALL %04hx\n", nnn);
                SP += 1;
                stack[SP++] = PC & 0x00FF;
                stack[SP]   = PC >> 8;
                PC = nnn;
                break;

            case 0x3000:                // SE Vx, kk 
                fprintf(log, "SE V%hx, %hd\n", x, kk);
                PC += 2 * (registers[x] == kk);
                break;

            case 0x4000:                // SNE Vx, kk
                fprintf(log, "SNE V%hx, %hd\n", x, kk);
                PC += 2 * (registers[x] != kk);
                break;

            case 0x5000:                // SE Vx, Vy
                fprintf(log, "SE V%hx, V%hx\n", x, y);
                PC += 2 * (registers[x] == registers[y]);
                break;

            case 0x6000:                // LD Vx, kk
                fprintf(log, "LD V%hx, %hd\n", x, kk);
                registers[x] = kk;
                break;

            case 0x7000:                // ADD Vx, kk
                fprintf(log, "ADD V%hx, %hd\n", x, kk);
                registers[x] += kk;
                break;

            case 0x8000:
                switch (instruction & 0x000F)
                {
                    case 0x0000:        // LD Vx, Vy
                        fprintf(log, "LD V%hx, V%hx\n", x, y);
                        registers[x] = registers[y];
                        break;

                    case 0x0001:        // OR Vx, Vy
                        fprintf(log, "OR V%hx, V%hx\n", x, y);
                        registers[x] |= registers[y];
                        break;

                    case 0x0002:        // AND Vx, Vy
                        fprintf(log, "AND V%hx, V%hx\n", x, y);
                        registers[x] &= registers[y];
                        break;

                    case 0x0003:        // XOR Vx, Vy
                        fprintf(log, "XOR V%hx, V%hx\n", x, y);
                        registers[x] ^=registers[y];
                        break;

                    case 0x0004:        // ADD Vx, Vy
                        fprintf(log, "ADD V%hx, V%hx\n", x, y);
                        registers[0xF] = (((int)registers[x] + registers[y]) > 255); 
                        registers[x] += registers[y];
                        break;

                    case 0x0005:        // SUB Vx, Vy
                        fprintf(log, "SUB V%hx, V%hx\n", x, y);
                        registers[0xF] = registers[x] >= registers[y];
                        registers[x] = registers[x] - registers[y];
                        break;

                    case 0x0006:        // SHR Vx, Vy
                        fprintf(log, "SHR V%hx {, V%hx}\n", x, y);
                        registers[0xF] = registers[x] & 0x01;
                        registers[x] /= 2;
                        break;

                    case 0x0007:        // SUBN Vx, Vy
                        fprintf(log, "SUBN V%hx, V%hx\n", x, y);
                        registers[0xF] = registers[x] <= registers[y];
                        registers[x] = registers[y] - registers[x];
                        break;

                    case 0x000E:        // SHL Vx, Vy
                        fprintf(log, "SHL V%hx {, V%hx}\n", x, y);
                        registers[0xF] = (registers[x] >> 7) & 0x01;
                        registers[x] *= 2;
                        break;

                    default:
                        valid_opcode = false;
                }
                break;

            case 0x9000:                // SNE Vx, Vy
                fprintf(log, "SNE V%hx, V%hx\n", x, y);
                PC += 2 * (registers[x] != registers[y]);
                break;
                
            case 0xA000:                // LD I, nnn
                fprintf(log, "LD I, %04hx\n", nnn);
                I = nnn;
                break;

            case 0xB000:                // JP V0, nnn
                fprintf(log, "JP V0, %04hx\n", nnn);
                PC = nnn + registers[0];
                break;

            case 0xC000:                // RND Vx, kk
                fprintf(log, "RND V%hx, %d\n", x, kk);
                registers[x] = (rand() % 0x100) & kk;
                break;

            case 0xD000:                // DRW Vx, Vy, n
                fprintf(log, "DRW V%hx, V%hx, %d\n", x, y, n);
                registers[0xF] = 0;
                for (int _y = 0; _y < n; _y++)
                {
                    uint8_t b = memory[I + _y];
                    for (int _x = 7; _x >= 0; _x--, b>>=1)
                    {
                        fprintf(log, "%c", b % 2 ? '@' : ' ');
                        if (b % 2 == 0) continue;
                        if (XORpixel(fb, registers[x] + (_x), registers[y] + _y))
                            registers[0xF] = 1;
                    }
                    fprintf(log, "\n");
                }
                break;

            case 0xE000:
                switch (instruction & 0x00FF)
                {
                    case 0x009E:        // SKP Vx
                        fprintf(log, "SKP V%hx\n", x);
                        PC += 2 * (IsDown(registers[x]));
                        break;

                    case 0x00A1:        // SKNP Vx
                        fprintf(log, "SKNP V%hx\n", x);
                        PC += 2 * (!IsDown(registers[x]));
                        break;

                    default:
                        valid_opcode = false;
                }
                
                break;

            case 0xF000:
                switch (instruction & 0x00FF)
                {
                    case 0x0007:        // LD Vx, DT
                        fprintf(log, "LD V%hx, DT\n", x);
                        registers[x] = DT;
                        break;

                    case 0x000A:        // LD Vx, K
                        fprintf(log, "LD V%hx, K", x);
                        registers[x] = WaitKey();
                        break;

                    case 0x0015:        // LD DT, Vx
                        fprintf(log, "LD DT, V%hx\n", x);
                        DT = registers[x];
                        break;

                    case 0x0018:        // LD ST, Vx
                        fprintf(log, "LD ST, V%hx\n", x);
                        ST = registers[x];
                        break;

                    case 0x001B:        // LD C, Vx
                        fprintf(log, "LD C, V%hx\n", x);
                        C = registers[x];
                        break;

                    case 0x001E:        // ADD I, Vx
                        fprintf(log, "ADD I, V%hx\n", x);
                        I += registers[x];
                        break;

                    case 0x0029:        // LD F, Vx
                        fprintf(log, "LD F, V%hx\n", x);
                        I = registers[x] * 5;
                        break;

                    case 0x0033:        // LD B, Vx
                        fprintf(log, "LD B, V%hx\n", x);
                        memory[I] = registers[x] / 100;
                        memory[I+1] = registers[x] / 10 % 10;
                        memory[I+2] = registers[x] % 10;
                        break;
                    
                    case 0x0055:        // LD [I], Vx
                        fprintf(log, "LD [I], V%hx\n", x);
                        for (uint8_t i = 0; i <= x; memory[I + i] = registers[i], i++);
                        break;

                    case 0x0065:        // LD Vx, [I]
                        fprintf(log, "LD V%hx, [I]\n", x);
                        for (uint8_t i = 0; i <= x; registers[i] = memory[I + i], i++);
                        break;

                    default:
                        valid_opcode = false;

                }
                break;

            default:
                valid_opcode = false;
                break;
        }

        if (!valid_opcode)
        {
            fprintf(log, "Invalid opcode: %hx\n", instruction);
            break;
        }

        // Flush and Swap 
        gfxFlushBuffers();
        // gfxSwapBuffers();

        // Wait for VBlank
        gspWaitForVBlank();
    }

    fclose(log);
    gfxExit();
    return 0;
}
