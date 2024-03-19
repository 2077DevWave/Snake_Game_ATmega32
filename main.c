/*
 * LCD_interface.c
 *
 * Created: 19/12/1402 07:35:18 ب.ظ
 * Author : 2077DevWave
 */ 

#include <avr/io.h>
#include <stdio.h>
#include <avr/delay.h>
#include <stdlib.h>

#define CPU_F 8000000

////////////////////////////////////////////////////////////////////////// useful struct and enum //////////////////////////////////////////////////////////////////////////
struct pos{
	uint8_t x;
	uint8_t y;
};

enum dir{
	UP,
	DOWN,
	LEFT,
	RIGHT,
	UNKOWN_DIR
};

enum color{
	BLACK,
	WHITE
};

struct linklist{
	void *next;
	volatile content;
};

typedef struct pos pos;
typedef enum color color;
typedef enum dir dir;
typedef struct linklist linklist;
////////////////////////////////////////////////////////////////////////// Display Controller Function //////////////////////////////////////////////////////////////////////////
#define DATA PORTB
#define CONTROLL PORTA
#define E 0
#define RW 1
#define RS 2
#define RESET 3

#define NUM_COLS 128  // x
#define NUM_ROWS 64   // y

#define DISPSETPAGE(y) exec_command(0xB0|(y))
#define DISPLINESTART(start) exec_command(((start) & 63) | 64)

void initial_display(){
	DDRA = 0x0f;
	DDRB = 0xff;
	SET_PIN(&CONTROLL,RESET,1);
	SET_PIN(&CONTROLL,E,0);
	
	exec_command(0x40);		// Start Display Line = 0
	exec_command(0xA0);		// ADC = normal (Anzeigerichtung nicht gespiegelt)
	exec_command(0xA3);		// LCD-Bias = 1/7 für 4,5-5,5V    1/9 für 3,3-4,5V
	exec_command(0xC0);		// Output Mode = nicht gespiegelt
	exec_command(0x2F);		// LCD-Spannungserzeugung/Regelung einschalten
	exec_command(0x20);		// V5 Regulator Resistor Ratio = 0
	exec_command(0xAC);		// Static Indicator off
	exec_command(0x81);		// Kontrasteinstellung auswählen
	exec_command(32);		// Kontrast = mittel (für Bias 1/7: 20-50, für 1/9: 2-10)
	exec_command(0xA4);		// Displaytest aus
	exec_command(0xAF);		// Display ein
	exec_command(0xA6);		// Display normal (nicht invertiert)
	dispClear();
}

void write_data(char data){
	SET_PIN(&CONTROLL,RW,0); // set as write mode
	SET_PIN(&CONTROLL,RS,1); // set as data (not command)
	DATA = data;
	create_pulse();
}

void exec_command(char command){
	SET_PIN(&CONTROLL,RW,0); // set as write mode
	SET_PIN(&CONTROLL,RS,0); // set as command
	DATA = command;
	create_pulse();
}

void create_pulse(){		// to create a pulse
	SET_PIN(&CONTROLL,E,1);
	SET_PIN(&CONTROLL,E,0);
}

void SET_PIN(volatile uint8_t* port, uint8_t pin, char value)
{
	if(value != 0){
		*port |= (1<< pin);
	}else{
		*port &= ~(1<< pin);
	}
}

void dispClear(void) {
	pos position = {0,0};
	
	for (position.y = 0; position.y < 8; position.y++) {
		dispGotoXY (position);
		for (position.x = 0; position.x < 128; position.x++){
			write_data(0);
		}
	}
	
	pos zero = {0,0};
	dispGotoXY (zero);
}

void dispGotoXY (pos position) {
	exec_command(0xB0|position.y);
	exec_command(0x10|(position.x/16));
	exec_command(position.x&15);
}

void dispSetPix(pos position, color Color)
{
	char a;
	if (position.x < NUM_COLS && position.y < NUM_ROWS) {
		
		DISPSETPAGE(position.y / 8);

		//set read/mod/write (read doesn't increment column)
		exec_command(0xe0);

		// select column addr
		exec_command(0x10|(position.x/16));
		exec_command(position.x&15);

		a = read_data();
		//write pix
		write_data(Color == BLACK ? (1 << (position.y%8)) | a : ~(1 << (position.y%8)) & a);

		//end read/mod/write
		exec_command(0xee);
	}
}

volatile read_data(){
	volatile data;
	SET_PIN(&CONTROLL, RW, 1);
	SET_PIN(&CONTROLL, RS, 1);

	DDRB = 0X00;
	
	SET_PIN(&CONTROLL, E, 1);
	
	data = PINB;
	
	SET_PIN(&CONTROLL, E, 0);
	DDRB = 0xff;
	
	return data;
}

void draw_cube(pos from, pos to){
	for (uint8_t x = from.x; x <= to.x; x++)
	{
		for (uint8_t y = from.y; y <= to.y; y++)
		{
			pos pixel = {x,y};
			dispSetPix(pixel,BLACK);
		}
	}
}


////////////////////////////////////////////////////////////////////////// Keypad Controller //////////////////////////////////////////////////////////////////////////
#define KEY_UP 2
#define KEY_DOWN 3
#define KEY_LEFT 1
#define KEY_RIGHT 0

void initial_keypad(){
	DDRC = 0X00;
	PORTC = 0x00;
	PINC = 0x00;
}

dir key_pressed(){
	if (PINC == (1<<KEY_RIGHT))
	{
		return RIGHT;
	}
	
	if (PINC == (1<<KEY_LEFT))
	{
		return LEFT;
	}
	
	if (PINC == (1<<KEY_UP))
	{
		return UP;
	}
		
	if (PINC == (1<<KEY_DOWN))
	{
		return DOWN;
	}
	
	return UNKOWN_DIR;
}

////////////////////////////////////////////////////////////////////////// snake game function //////////////////////////////////////////////////////////////////////////

void initial_snake(pos snake[], uint8_t snake_size){
	for (uint8_t i = 0; i < snake_size; i++)
	{
		snake[i].x = snake_size - i;
		snake[i].y = 0;
		dispSetPix(snake[i],BLACK);
	}
	dispSetPix(create_new_rat(),BLACK);
}

pos create_new_rat(){
	pos a;
	a.x = (uint8_t) (rand() % 128);
	a.y = (uint8_t) (rand() % 64);
	return a;
}

void move_snake(pos *snake, uint8_t snake_size, dir direction){
	dispSetPix(snake[snake_size - 1],WHITE);
	
	for (uint8_t i = snake_size - 1; i >= 1; i--)
	{
		snake[i] = snake[i-1];
	}
	
	switch (direction)
	{
		case UP:
			snake[0].y += 1;
			break;
		case DOWN:
			snake[0].y -= 1;
			break;
		case LEFT:
			snake[0].x += 1;
			break;
		case RIGHT:
			snake[0].x -= 1;
			break;
	}
	
	dispSetPix(snake[0],BLACK);
}


int main(void)
{
	initial_display();
	initial_keypad();
	
	// snake game main variable`s
	uint8_t snake_size = 10;
	uint8_t snake_limit = 100;
	pos snake[snake_limit];
	dir current_direction = LEFT;
	
	initial_snake(snake,snake_size);
	

	
    /* Replace with your application code */
    while (1) 
    {
		if(key_pressed() != UNKOWN_DIR){
			current_direction = key_pressed();
		}
		
		move_snake(&snake, snake_size, current_direction);
		_delay_ms(100);
    }
}

// 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0        x/y
// .....................................4
// .....................................3
// .....................................2
// .....................................1
// .....................................0

