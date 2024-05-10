#include <stdio.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "pico/multicore.h"
#include "arducam/arducam.h"
#include "hardware/pio.h"
#include "lib/lsm6ds3.h"
#include "lib/pio_i2c.h"
#include "lib/st7735.h"
#include "lib/fonts.h"
#include "lib/draw.h"
#include "time.h"
#include "stdlib.h"

const char* arrow_command[] = {"left", "down", "up", "right"};
//#define CFG_TUD_CDC 1


uint8_t image_buf[324*324];
uint8_t displayBuf[80*160*2];
uint8_t header[2] = {0x55,0xAA};

#define FLAG_VALUE 123
#define RESTART_PIN 

bool restart = true;

PIO pio = pio1;
uint sm = 1;



// #define ARROW_COUNT 4  // Define the number of arrows as a constant for easy maintenance
// int arrow_spacing = 80 / ARROW_COUNT;  // This calculates the spacing based on the number of arrows
// const int arrow_width = 10;  // Width in pixels

// int16_t arrow_y_int[ARROW_COUNT];
// float arrow_y[ARROW_COUNT];// = {-40,-40 - (rand()%60 + 90),-40 -(2 * rand()%60 + 90)};
// uint16_t arrow_x_position[ARROW_COUNT] =  {8, 30, 40, 55}; //three possible spawn location of block
// uint16_t arrow_x[ARROW_COUNT]; 
// bool arrow_active[ARROW_COUNT] = {false, false, false, false};

// Function to read a command from USB and return it
#define LINE_LENGTH 64


// char* read_usb_command() {
//     static char buf[LINE_LENGTH];
//     char *p = buf;
//     int ch;

//     while ((ch = getchar_timeout_us(10000)) != PICO_ERROR_TIMEOUT) {
//         if (ch == '\r' || ch == '\n' || ch < 0) {  // Check for end of line or invalid character
//             if (p != buf) {  // Only return a string if we've received something
//                 *p = '\0';  // Null-terminate the string
//                 return buf;
//             } else {
//                 return NULL;  // No data received
//             }
//         } else {
//             if (p < buf + LINE_LENGTH - 1) *p++ = (char) ch;  // Safely add character to buffer
//         }
//     }
//     return NULL;  // No data was ready
// }

char* read_usb_command() {
    static char buf[LINE_LENGTH] = {0};  // Initialize buffer to zero
    char *p = buf;
    int ch;

    while ((ch = getchar_timeout_us(10000)) != PICO_ERROR_TIMEOUT) {
        if (ch == '\r' || ch == '\n' || ch < 0) {
            if (p != buf) {
                *p = '\0';
                return buf;
            }
            return NULL;
        } else {
            if (p < buf + LINE_LENGTH - 1) {
                *p++ = (char) ch;
            }
        }
    }
    return NULL;
}


void gameOver(){
	char over[] = "Game Over";
 	ST7735_WriteString(8, 50, over, Font_16x26, ST7735_RED, ST7735_WHITE);
	bool loop = true;
	int16_t data[3];
	while (loop){
		//readGyro(pio,sm,data);
		if (data[2] < -2000)
			loop = false;
		sleep_ms(50);
	}
	restart = true;
}

int score = 0;  // Initialize score

// void display_score() {
//     char score_text[20];
//     sprintf(score_text, "Score: %d", score);
//     // Using a smaller font and adjusting the position on the display
//     ST7735_WriteString(40, 160, score_text, Font_16x26, ST7735_WHITE, ST7735_BLACK); // using a smaller font and positioning it in the upper right corner
// }

void display_score() {
    char score_text[20];
    sprintf(score_text, "Score: %d", score);
    ST7735_WriteString(8, 70, score_text, Font_16x26, ST7735_BLUE, ST7735_WHITE);
}


// void initialize_arrows() {
//     for (int i = 0; i < ARROW_COUNT; i++) {
//         arrow_y[i] = 160;  // Start position just below the screen
//         arrow_active[i] = (rand() % 2) == 1;  // Randomly activate each arrow (50% chance)
//     }
// }

// Initialize arrows with random types and starting positions
// for (int i = 0; i < ARROW_COUNT; i++) {
//     arrow_types[i] = rand() % 4;
//     arrow_y[i] = 160;  // Start below the screen to move into view
//     arrow_x[i] = (i * arrow_spacing) + (arrow_spacing / 2) - (arrow_width / 2);
// }


void core1_entry() {
	multicore_fifo_push_blocking(FLAG_VALUE);

	uint32_t g = multicore_fifo_pop_blocking();

	if (g != FLAG_VALUE)
		printf("Hmm, that's not right on core 1!\n");
	else
		printf("It's all gone well on core 1!\n");

	gpio_init(PIN_LED);
	gpio_set_dir(PIN_LED, GPIO_OUT);

	ST7735_Init();
	ST7735_DrawImage(0, 0, 80, 160, arducam_logo);
	uint offset = pio_add_program(pio, &i2c_program);
    i2c_program_init(pio, sm, offset, PIN_SDA, PIN_SCL);

	LSM6DS3_init(pio, sm);

	struct arducam_config config;
	config.sccb = i2c0;
	config.sccb_mode = I2C_MODE_16_8;
	config.sensor_address = 0x24;
	config.pin_sioc = PIN_CAM_SIOC;
	config.pin_siod = PIN_CAM_SIOD;
	config.pin_resetb = PIN_CAM_RESETB;
	config.pin_xclk = PIN_CAM_XCLK;
	config.pin_vsync = PIN_CAM_VSYNC;
	config.pin_y2_pio_base = PIN_CAM_Y2_PIO_BASE;

	config.pio = pio0;
	config.pio_sm = 0;

	config.dma_channel = 0;
	config.image_buf = image_buf;
	config.image_buf_size = sizeof(image_buf);

	arducam_init(&config);

	
	int16_t data[3];

    // Draw static arrows for UI controls

	draw_outline_up_arrow(50,5, displayBuf, ST7735_WHITE, 7);
	draw_outline_down_arrow(32,15, displayBuf, ST7735_WHITE, 7);
	draw_outline_left_arrow(7,10, displayBuf, ST7735_WHITE, 7);
	draw_outline_right_arrow(75, 10, displayBuf, ST7735_WHITE, 7);

	srand(time(NULL));

	// lane initialize
	int16_t lane_y_int[5];// = {-40,0,40,80,120};
	float lane_y[5];// = {-40,0,40,80,120};
	float mov_speed;// = 2;
	
	// block initialize
	int16_t block_y_int[3];
	float time; 

	// arrow
	#define ARROW_COUNT 4  // Define the number of arrows as a constant for easy maintenance
	int arrow_spacing = 80 / ARROW_COUNT;  // This calculates the spacing based on the number of arrows
	const int arrow_width = 10;  // Width in pixels
	int16_t arrow_y_int[ARROW_COUNT];
	float arrow_y[ARROW_COUNT];// = {-40,-40 - (rand()%60 + 90),-40 -(2 * rand()%60 + 90)};
	uint16_t arrow_x_position[ARROW_COUNT] =  {8, 30, 40, 55}; //three possible spawn location of block
	//uint16_t arrow_x[ARROW_COUNT]; 

	bool initialized = false;    // Track initialization
	int active_arrow_index;     // Tracks which arrow is active
	int16_t arrow_types[ARROW_COUNT];
	uint16_t arrow_x_left = (0 * arrow_spacing) + (arrow_spacing / 2) - (arrow_width / 2)+2;
	uint16_t arrow_x_right = (int)(3.5 * arrow_spacing) + (arrow_spacing / 2) - (arrow_width / 2) + 4;
	uint16_t arrow_x_down = (1 * arrow_spacing) + (arrow_spacing / 2) - (arrow_width / 2)+7;
	uint16_t arrow_x_up = (2 * arrow_spacing) + (arrow_spacing / 2) - (arrow_width / 2)+5;
    int16_t arrow_outline_y_positions[ARROW_COUNT] = {10, 15, 5, 10};  // Y positions for each arrow type outline
 	//int16_t arrow_outline_y_positions[ARROW_COUNT] = {10, 15, 5, 10};  // Y positions for each arrow type outline
	uint16_t arrow_x[ARROW_COUNT] = {arrow_x_left, arrow_x_down, arrow_x_up, arrow_x_right};
    
	uint32_t control;

	bool flag = false;

	uint integtime = 3;
	bool integflag = false;
	float gyrosum[2] = {0,0};

    for (int i = 0; i < 4; i++) {
        arrow_types[i] = rand() % 4;
        arrow_y[i] = 160;  // Initialize all arrows off-screen
    }
    active_arrow_index = rand() % 4;

	//initialize_arrows();
	while (true) {
		gpio_put(PIN_LED, !gpio_get(PIN_LED)); 
		arducam_capture_frame(&config);


		uint16_t index = 0;
		const uint16_t max_index = sizeof(displayBuf) - 2; 
		for (int y = 0; y < 160; y++) {
			for (int x = 0; x < 80; x++) {
				//if (index > max_index) break;
				// Calculate the source row in the image buffer based on the flipped y-coordinate
				int src_y = 319 - 2 * y;  // Flipping and mapping to the larger image buffer
				// Calculate the source column in the image buffer
				int src_x = 2 + 40 + 2 * x;

				// Check if src_y or src_x goes out of the bounds of the image buffer dimensions
				if (src_y < 324 && src_x < 324) {
					uint8_t c = image_buf[src_y * 324 + src_x];
					uint16_t imageRGB = ST7735_COLOR565(c, c, c);
					displayBuf[index++] = (uint8_t)(imageRGB >> 8) & 0xFF;
					displayBuf[index++] = (uint8_t)(imageRGB) & 0xFF;
				} else {
					// Handle the case where src_y or src_x are out of bounds
					displayBuf[index++] = 0; // Default to black or another background color
					displayBuf[index++] = 0;
				}
			}
		}

		//all parameter restart
		if (restart) {
			// Initialize properties of each arrow
			for (int i = 0; i < ARROW_COUNT; i++) {
				arrow_y[i] = 160;  // Start all arrows just below the visible screen

			}
			mov_speed = 2;  // Movement speed of arrows
			time = 0;       // Reset time if it's used for some timing logic
			restart = false;  // Reset the restart flag to avoid reinitialization until needed again
		}


		//draw grass sides; boundary of the game for easy divide of blocks 
		//left 4 right 4, 80 - 12 = 68 left
		draw_rec_onbuf(0,0,4,160,displayBuf,ST7735_DARK_GREEN);
		draw_rec_onbuf(76,0,4,160,displayBuf,ST7735_DARK_GREEN);

		// Handle arrows movement
		// for (int i = 0; i < ARROW_COUNT; i++) {
		// 	arrow_y[i] -= mov_speed;  // Move arrows up
			
			

		// 	// If an arrow goes off the screen from the top, reset it to come from the bottom
		// 	if (arrow_y[i] <= 10) {  // Arrow height is assumed to be 30 pixels
		// 		int r = rand() % ARROW_COUNT;

		// 		arrow_x[i] = (i * arrow_spacing) + (arrow_spacing / 2) - (arrow_width / 2);
		// 		arrow_y[i] = 160;  // Reset Y to just below the screen so it comes from the bottom
		// 	}

		// 	// Draw the arrow only if it's within the visible screen area
		// 	if (arrow_y[i] <= 180) {
		// 		arrow_y_int[i] = (int16_t)arrow_y[i];
		// 		arrow_x[i] = (i * arrow_spacing) + (arrow_spacing / 2) - (arrow_width / 2);

		// 		draw_filled_left_arrow(arrow_x_left, arrow_y_int[i], displayBuf, ST7735_BLUE, 6);
		// 		draw_filled_down_arrow(arrow_x_dowm, arrow_y_int[i], displayBuf, ST7735_BLUE, 6);
		// 		draw_filled_up_arrow(arrow_x_up, arrow_y_int[i], displayBuf, ST7735_BLUE, 6);
		// 		draw_filled_right_arrow(arrow_x_right, arrow_y_int[i], displayBuf, ST7735_BLUE, 6);
		// 	}
		// }

		// Handle arrows movement

		for (int i = 0; i < ARROW_COUNT; i++) {
			// Move arrows up
			arrow_y[i] -= 2;

			// Check if the arrow has reached the top of the screen
			// if (arrow_y[i] <= 10) {
			// 	// Check if this arrow is the active one and has collided with its outlined counterpart
			// 	if (i == active_arrow_index) {
			// 		score++;  // Increase score
			// 		display_score();
			// 		// Randomize new active arrow
			// 		active_arrow_index = rand() % ARROW_COUNT;
			// 	}
			// 	// Reset position of all arrows
			// 	arrow_y[i] = 160;
			// 	// Re-randomize arrow type
			// 	arrow_types[i] = rand() % 4;
			// }

			if (arrow_y[i] <= 10) {
				char* cmd = read_usb_command(); // Read the command from USB

				// Increase score if the command matches the current active arrow and it's in position
				if (cmd && i == active_arrow_index && strcmp(cmd, arrow_command[arrow_types[i]]) == 0) {
					score++;
					//display_score();
					// Randomize new active arrow
					active_arrow_index = rand() % ARROW_COUNT;
				}

				// Reset position of all arrows
				arrow_y[i] = 160;
				// Re-randomize arrow type
				arrow_types[i] = rand() % 4;
			}

			// Draw arrows based on their type only if they are within the visible screen area
			if (arrow_y[i] <= 160 && i == active_arrow_index) {
				switch (arrow_types[i]) {
					case 0:
						draw_filled_left_arrow(arrow_x[0], arrow_y[i], displayBuf, ST7735_BLUE, 6);
						break;
					case 1:
						draw_filled_down_arrow(arrow_x[1], arrow_y[i], displayBuf, ST7735_BLUE, 6);
						break;
					case 2:
						draw_filled_up_arrow(arrow_x[2], arrow_y[i], displayBuf, ST7735_BLUE, 6);
						break;
					case 3:
						draw_filled_right_arrow(arrow_x[3], arrow_y[i], displayBuf, ST7735_BLUE, 6);
						break;
				}
			}
		}

		//repl control
		// control = getchar_timeout_us(0);
		// if ((control != PICO_ERROR_TIMEOUT) && (!flag)){
		// 	switch(control){
        //     case 'a': // change r/w mode
        //         car_x = car_x - 5;
		// 		car_x = car_x_lim(car_x,car_w);
		// 		flag = true;
        //         break;
		// 	case 'd': // change r/w mode
		// 		car_x = car_x + 5;
		// 		car_x = car_x_lim(car_x,car_w);
		// 		flag = true;
		// 		break;
		// 	case 'w':
		// 		car_y = car_y - mov_speed;
		// 		car_y = car_y_lim(car_y);
		// 		flag = true;
		// 		break;
		// 	case 's':
		// 		car_y = car_y + mov_speed;
		// 		car_y = car_y_lim(car_y);
		// 		flag = true;
		// 		break;
		// 	}
		// }
		// if ((control = PICO_ERROR_TIMEOUT) && (flag))
		// 	flag = false;

		// sensor control


		
		bool collFlag = 0;
		// for ( int i = 0; i < 3; i++){
		// 	bool collisionX = block_x[i] + block_w[i] >= car_x && car_x + car_w >= block_x[i];
		// 	bool collisionY = block_y_int[i] + block_h[i] >= car_y && car_y + car_h >= block_y_int[i];
		// 	collFlag = collisionX && collisionY;
		// 	if(collFlag){
		// 		break;
		// 	}
		// }
		
		draw_outline_up_arrow(50,5, displayBuf, ST7735_WHITE, 7);
		draw_outline_down_arrow(32,15, displayBuf, ST7735_WHITE, 7);
		draw_outline_left_arrow(7,10, displayBuf, ST7735_WHITE, 7);
		draw_outline_right_arrow(75, 10, displayBuf, ST7735_WHITE, 7);


	  	ST7735_DrawImage(0, 0, 80, 160, displayBuf);
		sleep_ms(50);

		if (mov_speed <= 7.5){
			mov_speed += (rand() % 2) * 0.75;
		}
		

		if (collFlag){
			// gameOver();
			
		}

		display_score();


	}
}



#include "hardware/vreg.h"

int main() {
	int loops=20;
	stdio_init_all();
	

	while (!tud_cdc_connected()) { sleep_ms(100); if (--loops==0) break;  }

	printf("tud_cdc_connected(%d)\n", tud_cdc_connected()?1:0);

	

	vreg_set_voltage(VREG_VOLTAGE_1_30);
	sleep_ms(1000);
	set_sys_clock_khz(250000, true);



	multicore_launch_core1(core1_entry);

	uint32_t g = multicore_fifo_pop_blocking();

	if (g != FLAG_VALUE)
		printf("Hmm, that's not right on core 0!\n");
	else {
		multicore_fifo_push_blocking(FLAG_VALUE);
		printf("It's all gone well on core 0!\n");
	}

	while (1)
	tight_loop_contents();
}
