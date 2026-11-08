#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>


uint16_t instruction;
size_t bytes_read; 
uint8_t opcode;
uint8_t registers[16]; 
uint8_t memory[4096];
uint8_t vram[32*64];
bool keys[16];
uint8_t delay_timer;
uint8_t sound_timer;
uint16_t PC; 
uint16_t stack[16];
uint8_t sp;
uint16_t I; 
bool COSMAC_MODE = true; 

uint32_t loadROM(char *filename);
void execute(); 
void initChip8();
void tickTimers();

static const uint8_t chip8_font[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

int main(int argc, char *argv[])
{
    int running = 1; 

     if (argc != 2)
    {
        printf("usage: decoder <filename>");
        return 1;
    }

    // init program
    initChip8(); 
    SDL_Renderer *renderer = initSDL();
    if (renderer == NULL)
    {
        return 1;
    }
    loadROM(argv[1]); 

    //main loop
    while (running)
    {
        if(!handleSDLevents())
        {
            running = 0; 
        }

        execute();
        drawDisplay(renderer);
    }

    return 0; 
}

void drawDisplay(renderer)
{

}

int handleSDLevents()
{
    SDL_Event event; 

    while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN &&
                 event.key.keysym.sym == SDLK_ESCAPE)) {
                return 0;
            }
        }
}

SDL_Renderer *initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_Window *window = SDL_CreateWindow(
        "SDL Test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 320,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED
    );

    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    return renderer;
}

void initChip8()
{
    srand((unsigned int)time(NULL));
    PC = 0x200; 
    sp = 0;
    instruction = 0;
    memset(registers, 0, 16);
    memset(memory, 0, 4096);
    memset(vram, 0, 64*32);
    memset(keys, 0, sizeof keys);
    memset(stack, 0, sizeof stack);
    delay_timer = 0;
    sound_timer = 0;
    memcpy(&memory[0x50], chip8_font, sizeof chip8_font);
}

void tickTimers()
{
    static uint64_t last_tick_ns;
    static uint64_t fractional_ticks;
    struct timespec now;

    if (timespec_get(&now, TIME_UTC) != TIME_UTC)
    {
        return;
    }

    uint64_t now_ns = (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec;
    if (last_tick_ns == 0)
    {
        last_tick_ns = now_ns;
        return;
    }

    fractional_ticks += (now_ns - last_tick_ns) * 60;
    last_tick_ns = now_ns;
    uint64_t elapsed_ticks = fractional_ticks / 1000000000ULL;
    fractional_ticks %= 1000000000ULL;

    if (elapsed_ticks > 0)
    {
        delay_timer = elapsed_ticks >= delay_timer ? 0 : delay_timer - (uint8_t)elapsed_ticks;
        sound_timer = elapsed_ticks >= sound_timer ? 0 : sound_timer - (uint8_t)elapsed_ticks;
    }
}

uint32_t loadROM(char* filename)
{
    FILE *file = fopen(filename, "rb");

    if (!file)
    {
        printf("Unable to open file.");
        return 0; 
    }

    // gets the file size
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // writes file contents into a buffer
    fread(&memory[0x200], 1, size, file);    

    
    return 1;
}
void execute()
{
    int index;
    int index_x, index_y;
    int value;
    int address; 
    int shifted_out; 

        tickTimers();
        
        instruction = (instruction | (memory[PC] << 8)) | memory[PC + 1];
        PC += 2; 
        opcode = instruction >> 12; 

        switch(opcode)
        {
            case 0x1: // JMP NNN 
            {
                address = 0x0FFF & instruction; 
                PC = address; 
                break; 
            }
            case 0x06: // 6XNN sets register X to NN 
            {
                index = 0x000F & (instruction >> 8); 
                registers[index] = 0x00FF & instruction; 
                break;
            }
            case 0x07: // 7XNN adds a value NN to register X 
            {
                value = 0x00FF & instruction;
                index = 0x000F & (instruction >> 8); 
                registers[index] += value; 
                break;
            }
            case 0x02: // Calls the subroutine at memory location NNN
            {
                address = 0x0FFF & instruction;
                stack[sp] = PC;
                PC = address;
                sp++; 
                break;
            }
            case 0x00: // return from a subroutine
            {
                sp--;
                PC = stack[sp];
                break;
            }
            case 0x03: // Skip conditionally if VX == NN 
            {
                index = (0x0F00 & instruction) >> 8; 
                value = 0x00FF & instruction; 
                if (registers[index] == value)
                {
                    PC += 2; 
                }
                break; 
            }
            case 0x04: // Skip Conditionally  if VX != NN
            {
                index = (0x0F00 & instruction) >> 8; 
                value = 0x00FF & instruction; 
                if (registers[index] != value)
                {
                    PC += 2; 
                }
                break; 
            }
            case 0x05: // Skip if VX == VY
            {
                index_x = (0x0F00 & instruction) >> 8;
                index_y = (0x00F0 & instruction) >> 4;
                if (registers[index_x] == registers[index_y])
                {
                    PC += 2;
                }
                break; 
            }
            case 0x09:
            {
                index_x = (0x0F00 & instruction) >> 8;
                index_y = (0x00F0 & instruction) >> 4;
                if (registers[index_x] != registers[index_y])
                {
                    PC += 2;
                }
                break; 
            }
            case 0x08:
            {
                switch( 0x000F & instruction ) // LOGICAL AND ARITHMETIC INSTRUCTIONS
                {
                    case 0x0: // Vx = Vy
                    {
                        index_x = (0x0F00 & instruction) >> 8;
                        index_y = (0x00F0 & instruction) >> 4;
                        registers[index_x] = registers[index_y];
                        break; 
                    }
                    case 0x1: // Vx |= Vy
                    {   
                        index_x = (0x0F00 & instruction) >> 8;
                        index_y = (0x00F0 & instruction) >> 4;
                        registers[index_x] |= registers[index_y];
                        break; 
                    }
                    case 0x2:
                    {
                        index_x = (0x0F00 & instruction) >> 8;
                        index_y = (0x00F0 & instruction) >> 4;
                        registers[index_x] &= registers[index_y];
                        break; 

                    }
                    case 0x3:
                    {
                        index_x = (0x0F00 & instruction) >> 8;
                        index_y = (0x00F0 & instruction) >> 4;
                        registers[index_x] ^= registers[index_y];
                        break; 
                    }
                    case 0x4: //
                    {
                        index_x = (0x0F00 & instruction) >> 8;
                        index_y = (0x00F0 & instruction) >> 4;
                        value = registers[index_x] + registers[index_y];
                        registers[index_x] = value;
                        printf("%d", registers[index_x]);

                        if (value > 255)
                        {
                            registers[0xF] = 1;
                        }
                        else
                        {
                            registers[0xF] = 0;
                        }
                        break; 
                    }
                    case 0x5: // Vx = Vx - Vy
                    {
                        index_x = (0x0F00 & instruction) >> 8;
                        index_y = (0x00F0 & instruction) >> 4;
                        value = registers[index_x] - registers[index_y];
                        registers[index_x] = value;
                        if (registers[index_x] >= registers[index_y])
                        {
                            registers[0xF] = 1;
                        }
                        else
                        {
                            registers[0xF] = 0; 
                        }
                        break; 

                    }
                    case 0x6: // BitShift Right
                    {
                        index_x = (0x0F00 & instruction) >> 8;
                        index_y = (0x00F0 & instruction) >> 4;
                        shifted_out = 0b00000001 & registers[index_x];
                        registers[0xF] = (shifted_out == 1);

                        if (COSMAC_MODE)
                        {
                            registers[index_x] = registers[index_y];        
                        }
                        
                        registers[index_x] >>= 1;

                        break;
                       
                    }
                    case 0x7: // Vx = Vy - Vx
                    {
                        index_x = (0x0F00 & instruction) >> 8;
                        index_y = (0x00F0 & instruction) >> 4;
                        value = registers[index_y] - registers[index_x];
                        registers[index_x] = value;

                        if (registers[index_x] >= registers[index_y])
                        {
                            registers[0xF] = 1;
                        }
                        else
                        {
                            registers[0xF] = 0; 
                        }
                        break;  
                        
                    }
                    case 0xE: // Bitshift left
                    {
                        index_x = (0x0F00 & instruction) >> 8;
                        index_y = (0x00F0 & instruction) >> 4;
                        shifted_out = 0b10000000 & registers[index_x];
                        registers[0xF] = (shifted_out == 1);

                        if (COSMAC_MODE)
                        {
                            registers[index_x] = registers[index_y];
                        }
                        
                        registers[index_x] <<= 1;

                        break;
                        
                    }
                }
                break;
            }
            case 0x0A:
            {
                value = 0x0FFF & instruction; 
                I = value; 
                break; 
            }
            case 0x0B:
            {
                if (COSMAC_MODE)
                {
                    // Original CHIP-8 behavior: jump to NNN + V0.
                    PC = (instruction & 0x0FFF) + registers[0];
                }
                else
                {
                    // CHIP-48 / SUPER-CHIP behavior: jump to XNN + VX.
                    index = (instruction >> 8) & 0x000F;
                    PC = (instruction & 0x0FFF) + registers[index];
                }
                break;
            }
            case 0x0C:
            {
                index = (instruction >> 8) & 0x000F;
                value = rand() & (instruction & 0x00FF);
                registers[index] = (uint8_t)value;
                break;
            }
            case 0x0D:
            {
                int x, y, height;
                int row, column;
                uint8_t sprite_byte;

                index_x = (0x0F00 & instruction) >> 8; 
                index_y = (0x00F0 & instruction) >> 4; 
                height = 0x000F & instruction; 

                x = registers[index_x];
                y = registers[index_y];

                registers[0xF] = 0;

                for (row = 0; row < height; row++)
                {
                    sprite_byte = memory[(I + row) & 0x0FFF];

                    for (column = 0; column < 8; column++)
                    {
                        if ((sprite_byte & (0x80 >> column)) != 0)
                        {
                            int pixel_x = (x + column) % 64;
                            int pixel_y = (y + row) % 32;
                            int pixel_index = pixel_y * 64 + pixel_x;

                            if (vram[pixel_index] != 0)
                            {
                                registers[0xF] = 1;
                            }

                            vram[pixel_index] ^= 1;
                        }
                    }
                }
                break;
            }
            case 0x0E:
            {
                index = (instruction >> 8) & 0x000F;
                switch (instruction & 0x00FF)
                {
                    case 0x9E: // Skip if key in VX is pressed.
                        if (keys[registers[index] & 0x0F])
                        {
                            PC += 2;
                        }
                        break;
                    case 0xA1: // Skip if key in VX is not pressed.
                        if (!keys[registers[index] & 0x0F])
                        {
                            PC += 2;
                        }
                        break;
                }
                break;
            }
            case 0x0F:
            {
                index = (instruction >> 8) & 0x000F;
                switch (instruction & 0x00FF)
                {
                    case 0x07: // VX = delay timer.
                        registers[index] = delay_timer;
                        break;
                    case 0x0A: // Wait for a key press; retry this instruction until one is down.
                    {
                        int key;
                        for (key = 0; key < 16 && !keys[key]; key++)
                        {
                        }

                        if (key < 16)
                        {
                            registers[index] = (uint8_t)key;
                        }
                        else
                        {
                            PC -= 2;
                        }
                        break;
                    }
                    case 0x15: // delay timer = VX.
                        delay_timer = registers[index];
                        break;
                    case 0x18: // sound timer = VX.
                        sound_timer = registers[index];
                        break;
                    case 0x1E: // I += VX; original CHIP-8 does not set VF on overflow.
                        I += registers[index];
                        break;
                    case 0x29: // I = address of the 5-byte font sprite for VX's low nibble.
                        I = 0x50 + (registers[index] & 0x0F) * 5;
                        break;
                    case 0x33: // Store the hundreds, tens, and ones digits of VX.
                        memory[I & 0x0FFF] = registers[index] / 100;
                        memory[(I + 1) & 0x0FFF] = (registers[index] / 10) % 10;
                        memory[(I + 2) & 0x0FFF] = registers[index] % 10;
                        break;
                    case 0x55: // Store V0..VX; keep I unchanged (modern behavior).
                    {
                        int reg;
                        for (reg = 0; reg <= index; reg++)
                        {
                            memory[(I + reg) & 0x0FFF] = registers[reg];
                        }
                        break;
                    }
                    case 0x65: // Load V0..VX; keep I unchanged (modern behavior).
                    {
                        int reg;
                        for (reg = 0; reg <= index; reg++)
                        {
                            registers[reg] = memory[(I + reg) & 0x0FFF];
                        }
                        break;
                    }
                }
                break;
            }

            
        }

        instruction = 0;
        opcode = 0;
}


