/// Includes
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char byte;
typedef unsigned short word;

byte memory[4096] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0x10, 0xF0, 0xF0, 0x10, 0xF0,
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

byte registers[16];

word I, PC = 0x200;
byte DT = 0, ST = 0, SP;


int main(int argc, char **argv)
{
    // Init gfx
	gfxInitDefault();

    // Console on bottom screen
	consoleInit(GFX_BOTTOM, NULL);

    // Disable buffer swaping and get buffer
    gfxSetDoubleBuffering(GFX_TOP, false);
    u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);


	// Main loop
    word instruction;
    memory[PC] = 0x00;
    memory[PC+1] = 0xe0;
	while (aptMainLoop())
	{
        // Fetch
        instruction = memory[PC];
        instruction <<= 8;
        instruction += memory[PC + 1];

        // Increment PC
        PC+= 2;

        // Print instruction in bottom screen 
        printf("0x%04hx  0x%04hx\n", PC, instruction);

        // Read frame input
        hidScanInput();
        u32 kheld = hidKeysHeld();
        
        // Exit if start is holded
        if (hidKeysDown() & KEY_START)
            break;


        // Big boi
        // There should be a big switch with every instruction here
        // hmm


        // Flush and Swap 
        gfxFlushBuffers();
        gfxSwapBuffers();

        // Wait for VBlank
        gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
