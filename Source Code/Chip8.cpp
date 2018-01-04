#pragma warning(disable:4996)
#include "Chip8.h"
#include<string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <random>
#include "time.h"


unsigned char chip8_fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, //0
	0x20, 0x60, 0x20, 0x20, 0x70, //1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
	0x90, 0x90, 0xF0, 0x10, 0x10, //4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
	0xF0, 0x10, 0x20, 0x40, 0x40, //7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
	0xF0, 0x90, 0xF0, 0x90, 0x90, //A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
	0xF0, 0x80, 0x80, 0x80, 0xF0, //C
	0xE0, 0x90, 0x90, 0x90, 0xE0, //D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
	0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

Chip8::Chip8() {}
Chip8::~Chip8() {}

void Chip8::debugRender()
{
	// Draw
	for (int y = 0; y < 32; ++y)
	{
		for (int x = 0; x < 64; ++x)
		{
			if (gfx[(y * 64) + x] == 0)
				printf("O");
			else
				printf(" ");
		}
		printf("\n");
	}
	printf("\n");
}

void Chip8::init() {
	sp = NULL;
	pc = 0x200; //Program starts at addy 512
	opcode = 0;
	I = 0;
	delay_timer = 0;
	sound_timer = 0;

	for (int i = 0; i < 16; i++)
	{
		stack[i] = 0;
		V[i] = 0;
		key[i] = 0;
	}

	for (int i = 0; i < 4096; i++)
		memory[i] = 0;

	for (int i = 0; i < 80; i++)
		memory[i] = chip8_fontset[i];

	for (int i = 0; i < 2048; i++) {
		gfx[i] = 0;
	}

	drawFlag = true;
	srand((unsigned int)time(NULL));
}

bool Chip8::load(const char *file_path) {
	//Read in File
	//Grab File Size, check to see if it fits in memory
	//If so, read?

	std::cout << "Preparing to read Rom at location " << file_path << std::endl;

	FILE* rom = fopen(file_path, "rb");

	if (rom == NULL) {
		std::cout << "File not found" << std::endl;
		return false;
	}

	fseek(rom, 0, SEEK_END);
	long rom_size = ftell(rom);
	rewind(rom);
	printf("Filesize: %d\n", (int)rom_size);

	char* rom_buffer = (char*)malloc(sizeof(char) * rom_size);
	if (rom_buffer == NULL) {
		std::cerr << "Failed to allocate memory for ROM" << std::endl;
		return false;
	}

	// Copy ROM into buffer
	size_t result = fread(rom_buffer, sizeof(char), (size_t)rom_size, rom);
	if (result != rom_size) {
		std::cerr << "Failed to read ROM" << std::endl;
		return false;
	}

	if ((4096 - 512) > rom_size)
		for (int i = 0; i < rom_size; i++)
			memory[i + 512] = (uint8_t)rom_buffer[i];
	else
	{
		std::cout << "ROM too big! Aborting";
		return false;
	}
	fclose(rom);
	free(rom_buffer);
	std::cout << "File has been loaded!" << std::endl;
	return true;

}

void Chip8::emulate_cycle() {
	//What this does is to left shift that byte 8 bits then or it with 
	//the next byte because in Chip8 each opcode is 2 bytes long
	opcode = memory[pc] << 8 | memory[pc + 1];

	switch (opcode & 0xF000) {
	case 0x0000:
		switch (opcode & 0x000F)
		{
		case 0x0000: {
			for (int i = 0; i < 2048; i++)
				gfx[i] = 0;
			drawFlag = true; // signal to update screen
			pc += 2;
			break; }
		case 0x000E: {

			pc = stack[--sp];
			pc += 2; 
			break; }
		}
		break;
	case 0x1000: {
		pc = (opcode & 0x0FFF);
		break;
	}
	case 0x2000: {
		stack[sp++] = pc;

		pc = (opcode & 0x0FFF);
		break;
	}
	case 0x3000: {
		if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
			pc += 4;
		else
			pc += 2;
		break;
	}
	case 0x4000: {
		if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
			pc += 4;
		else
			pc += 2;
		break; }
	case 0x5000: {
		if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break; }
	case 0x6000: {
		V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
		pc += 2;
		break;
	}
	case 0x7000: {
		V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
		pc += 2;
		break;
	}
	case 0x8000:
		switch (opcode & 0x000F) {
		case 0x0000: {
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		}
		case 0x0001: {
			V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		}
		case 0x0002: {
			V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		}
		case 0x0003: {
			V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		}
		case 0x0004: {
			if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
				V[0xF] = 1;
			else
				V[0xF] = 0;
			V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		}
		case 0x0005: {
			if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
				V[0xF] = 0;
			else
				V[0xF] = 1;
			V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		}
		case 0x0006: {
			V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1; // Double Check
			V[(opcode & 0x0F00) >> 8] >>= 1;
			pc += 2;
			break;
		}
		case 0x0007: {
			if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
				V[0xF] = 0;
			else
				V[0xF] = 1;
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		}
		case 0x000E: {
			V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
			V[(opcode & 0x0F00) >> 8] <<= 1;
			pc += 2;
			break;
		}
		}
		break;
	case 0x9000: {
		if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break;
	}
	case 0xA000: { // Sets I to the address NNN.
		I = opcode & 0x0FFF;
		pc += 2;
		break;
	}

	case 0xB000: { //Jumps to the address NNN plus V0.
				   //stack[sp] = pc;//My Mod
				   //++sp;//My mod
		pc = (opcode & 0x0FFF) + V[0];
		break;
	}

	case 0xC000: { 
		V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
		pc += 2;
		break;
	}

	case 0xD000: {
		unsigned short x = V[(opcode & 0x0F00) >> 8];
		unsigned short y = V[(opcode & 0x00F0) >> 4];
		unsigned short height = opcode & 0x000F;
		unsigned short pixel;

		V[0xF] = 0;
		for (int vertical = 0; vertical < height; vertical++)
		{
			pixel = memory[I + vertical];
			for (int horizontal = 0; horizontal < 8; horizontal++)
			{
				if ((pixel & (0x80 >> horizontal)) != 0)
				{
					if (gfx[(x + horizontal + ((y + vertical) * 64))] == 1)
						V[0xF] = 1;
					gfx[x + horizontal + ((y + vertical) * 64)] ^= 1;
				}
			}
		}

		drawFlag = true;
		pc += 2;
		break;
	}
	case 0xE000: 
		switch (opcode & 0x00FF)
		{
		case 0x009E:
			if (key[V[(opcode & 0x0F00) >> 8]] != 0)
				pc += 4;
			else
				pc += 2;
			break;
		case 0x00A1: 
			if (key[V[(opcode & 0x0F00) >> 8]] == 0)
				pc += 4;
			else
				pc += 2;
			break;
		}
		break;
	case 0xF000:
		switch (opcode & 0x00FF) {
		case 0x0007: {
			V[(opcode & 0x0F00) >> 8] = delay_timer;
			pc += 2;
			break;
		}
		case 0x000A: {
			bool pressed = false;

			for (int i = 0; i < 16; i++)
				if (key[i] != 0)
				{
					V[(opcode & 0x0F00) >> 8] = i; //Why not key[i]! Try later
					pressed = true;
				}
			if (!pressed)
				return;
			pc += 2;
			break; }
		case 0x0015: {
			delay_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break; }
		case 0x0018: {
			sound_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break; }
		case 0x001E: {
			if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
				V[0xF] = 1;
			else
				V[0xF] = 0;
			I += V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break; }
		case 0x0029: {
			I = V[(opcode & 0x0F00) >> 8] * 0x5;
			pc += 2;
			break; }
		case 0x0033: {
			memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
			memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
			memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
			pc += 2;
			break; }
		case 0x0055: {
			for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
			{
				memory[I + i] = V[i];
			}

			// On the original interpreter, when the
			// operation is done, I = I + X + 1.
			I += ((opcode & 0x0F00) >> 8) + 1;

			pc += 2;
			break;
		}
		case 0x0065: {
			for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
			{
				V[i] = memory[I + i];
			}

			// On the original interpreter, when the
			// operation is done, I = I + X + 1.
			I += ((opcode & 0x0F00) >> 8) + 1;

			pc += 2;
			break; }
		}
		break;
	}
		if (delay_timer > 0)
			delay_timer--;
		if (sound_timer > 0)
			sound_timer--;
	
}
