/// Includes
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>

u32 KEY_MOD = KEY_R | KEY_L; 
u32 key_map[] = {KEY_DDOWN, KEY_DUP, KEY_DRIGHT, KEY_X, KEY_LEFT, KEY_DUP, KEY_DRIGHT, KEY_X, KEY_DLEFT, KEY_Y, KEY_A, KEY_B, KEY_A, KEY_B};
uint16_t mod_keys = 0x81F3; // 1000000111110011 

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

uint16_t I, PC = 0x200;
uint8_t DT = 0, ST = 0, C = 0xFF, SP;


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

u32 WaitKey(u32 key_mask)
{
    u32 dkeys;

    do
    {
        hidScanInput();
        dkeys = hidKeysDown();
    } while (!(dkeys & key_mask));

    return dkeys;
}

u8 MapKey(u32 key_mask)
{
    uint16_t shifted_mask = key_mask;
    uint8_t key_masked;
    bool modded;

    for (uint8_t i = 0; i < 16; i++)
    {
        key_masked = (key_map[15 - i] & key_mask);
        modded = key_masked % 2;

        if (((key_masked == 0) && !modded) || ((key_masked & mod_keys) && modded))
            return i;

        shifted_mask >>= 1;
    }

    return 0;
}


int main(int argc, char **argv)
{
    // Init gfx
    gfxInitDefault();

    // Console on bottom screen
    consoleInit(GFX_BOTTOM, NULL);

    // Disable buffer swaping and get buffer
    gfxSetDoubleBuffering(GFX_TOP, false);
    u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

    // Read binary from memory
    FILE *file = fopen("chip8-rom.ch8", "rb");
    if (file == NULL) goto quit;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    fread(memory + PC, 1, size, file);
    fclose(file);


    // Main loop
    uint16_t instruction;
    uint16_t nnn;
    uint8_t n, x, y, kk;
    bool running = true;

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

        // Print instruction in bottom screen 
        printf("0x%04hx  %04hx  ", PC, instruction);

        // Read frame input
        hidScanInput();
        u32 kheld = hidKeysHeld();
        
        // Exit if start is holded
        if (kheld & KEY_START)
            running = false;
        

        // Big boi
        switch (instruction & 0xF000)
        {
            case 0x0000:
                switch (instruction & 0x00FF)
                {
                    case 0x0000:            // NULL
                        printf("NULL\n");
                        break;

                    case 0x00E0:            // CLS
                        printf("CLS\n");
                        for (int i = 0; i < 400*240; i++)
                            fb[i] = fb[i + 1] = fb[i + 2] = 0;
                        break;

                    case 0x00EE:            // RET
                        printf("RET\n");
                        PC = memory[SP--];
                        PC <<= 8;
                        PC += memory[SP--];
                        break;

quit:               case 0x00FD:            // EXIT
                        printf("EXIT\n");
                        running = false;
                        break;
                }
                break;

            case 0x1000:                // JP nnn 
                printf("JP %04hx\n", nnn);
                PC = nnn;
                break;

            case 0x2000:                // CALL nnn 
                printf("CALL %04hx\n", nnn);
                SP += 1;
                memory[SP++] = PC & 0x00FF;
                memory[SP]   = PC >> 8;
                PC = nnn;
                break;

            case 0x3000:                // SE Vx, kk 
                printf("SE V%hx, %hd\n", x, kk);
                PC += 2 * (registers[x] == kk);
                break;

            case 0x4000:                // SNE Vx, kk
                printf("SNE V%hx, %hd\n", x, kk);
                PC += 2 * (registers[x] != kk);
                break;

            case 0x5000:                // SE Vx, Vy
                printf("SE V%hx, V%hx\n", x, y);
                PC += 2 * (registers[x] == registers[y]);
                break;

            case 0x6000:                // LD Vx, kk
                printf("LD V%hx, %hd\n", x, kk);
                registers[x] = kk;
                break;

            case 0x7000:                // ADD Vx, kk
                printf("ADD V%hx, %hd\n", x, kk);
                registers[x] += kk;
                break;

            case 0x8000:
                switch (instruction & 0x000F)
                {
                    case 0x0000:        // LD Vx, Vy
                        printf("LD V%hx, V%hx\n", x, y);
                        registers[x] = registers[y];
                        break;

                    case 0x0001:        // OR Vx, Vy
                        printf("OR V%hx, V%hx\n", x, y);
                        registers[x] |= registers[y];
                        break;

                    case 0x0002:        // AND Vx, Vy
                        printf("AND V%hx, V%hx\n", x, y);
                        registers[x] &= registers[y];
                        break;

                    case 0x0003:        // XOR Vx, Vy
                        printf("XOR V%hx, V%hx\n", x, y);
                        registers[x] ^=registers[y];
                        break;

                    case 0x0004:        // ADD Vx, Vy
                        printf("ADD V%hx, V%hx\n", x, y);
                        registers[0xF] = ((registers[x] + registers[y]) < registers[x]);
                        registers[x] += registers[y];
                        break;

                    case 0x0005:        // SUB Vx, Vy
                        printf("SUB V%hx, V%hx\n", x, y);
                        registers[0xF] = registers[x] > registers[y];
                        registers[x] -= registers[y];
                        break;

                    case 0x0006:        // SHR Vx, Vy
                        printf("SHR V%hx {, V%hx}\n", x, y);
                        registers[0xF] = registers[x] & 0x01;
                        registers[x] /= 2;
                        break;

                    case 0x0007:        // SUBN Vx, Vy
                        printf("SUBN V%hx, V%hx\n", x, y);
                        registers[0xF] = registers[x] < registers[y];
                        registers[x] = registers[y] - registers[x];
                        break;

                    case 0x000E:        // SHL Vx, Vy
                        printf("SHL V%hx {, V%hx}\n", x, y);
                        registers[0xF] = (registers[x] >> 7) & 0x01;
                        registers[x] *= 2;
                        break;
                }
                break;

            case 0x9000:                // SNE Vx, Vy
                printf("SNE V%hx, V%hx\n", x, y);
                PC += 2 * (registers[x] != registers[y]);
                break;
                
            case 0xA000:                // LD I, nnn
                printf("LD I, %04hx\n", nnn);
                I = nnn;
                break;

            case 0xB000:                // JP V0, nnn
                printf("JP V0, %04hx\n", nnn);
                PC = nnn + registers[0];
                break;

            case 0xC000:                // RND Vx, kk
                printf("RND V%hx, %d\n", x, kk);
                registers[x] = (rand() % 0x100) & kk;
                break;

            case 0xD000:                // DRW Vx, Vy, n
                printf("DRW V%hx, V%hx, %d\n", x, y, n);
                registers[0xF] = 0;
                for (int _y = 0; _y < n; _y++)
                {
                    uint8_t b = memory[I + _y];
                    for (int _x = 7; _x >= 0; _x--, b>>=1)
                    {
                        if (b % 2 == 0) continue;
                        if (XORpixel(fb, registers[x] + (_x), registers[y] + _y))
                            registers[0xF] = 1;
                    }
                }
                break;

            case 0xE000:
                switch (instruction & 0x00FF)
                {
                    case 0x0095:        // SKP Vx
                        printf("SKP V%hx", x);
                        PC += 2 * (registers[x] == MapKey(kheld));
                        break;

                    case 0x00A1:        // SKNP Vx
                        printf("SKNP V%hx", x);
                        PC += 2 * (registers[x] != MapKey(kheld));
                        break;
                }
                
                break;

            case 0xF000:
                switch (instruction & 0x00FF)
                {
                    case 0x0007:        // LD Vx, DT
                        printf("LD V%hx, DT\n", x);
                        registers[x] = DT;
                        break;

                    case 0x000A:        // LD Vx, K
                        printf("LD V%hx\n, K", x);
                        kheld = WaitKey(0xFFFFFFFF);
                        registers[x] = MapKey(kheld);
                        break;

                    case 0x0015:        // LD DT, Vx
                        printf("LD DT, V%hx\n", x);
                        DT = registers[x];
                        break;

                    case 0x0018:        // LD ST, Vx
                        printf("LD ST, V%hx\n", x);
                        ST = registers[x];
                        break;

                    case 0x001B:        // LD C, Vx
                        printf("LD C, V%hx\n", x);
                        C = registers[x];
                        break;

                    case 0x001E:        // ADD I, Vx
                        printf("ADD I, V%hx\n", x);
                        I += registers[x];
                        break;

                    case 0x0029:        // LD F, Vx
                        printf("LD F, V%hx\n", x);
                        I = registers[x] * 5;
                        break;

                    case 0x0033:        // LD B, Vx
                        printf("LD B, V%hx\n", x);
                        memory[I] = registers[x] / 100;
                        memory[I+1] = registers[x] / 10 % 10;
                        memory[I+2] = registers[x] % 10;
                        break;
                    
                    case 0x0055:        // LD [I], Vx
                        printf("LD [I], V%hx\n", x);
                        for (uint8_t i = 0; i <= x; memory[I + i] = registers[i], i++);
                        break;

                    case 0x0065:        // LD Vx, [I]
                        printf("LD V%hx, [I]\n", x);
                        for (uint8_t i = 0; i <= x; registers[i] = memory[I + i], i++);
                        break;

                }
                break;

            default:
                printf("Invalid opcode: %hx", instruction);
                goto quit;
        }


        // Flush and Swap 
        gfxFlushBuffers();
        gfxSwapBuffers();

        // Wait for VBlank
        gspWaitForVBlank();
    }

    WaitKey(KEY_SELECT);
    gfxExit();
    return 0;
}
