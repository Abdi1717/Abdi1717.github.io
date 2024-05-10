# Abdi1717.github.io
# Hands Hands Revolution

![HHR Game Play](/images/HHRGamePlay.jpg)

## Introduction

Hands Hands Revolution is designed to bring the joy of dance to everyone, regardless of mobility constraints. This project constructs a simple augmented reality (AR) game on the specified PICO-4ML board. The game is played by one player, who holds up the board and interacts with the embedded LCD screen. As the arrows move up the screen, your goal is to show the correct direction when the arrow is about to overlay its outline. When the player does this correctly, the score increases.

![Design with Enclosure](/images/designOfEnclosure.jpg)

## Environment and Components

### Components

![Pico 4ML](/images/cable.png)
![Pico 4ML2](/images/pico4ml.jpg)

### Block Diagram

![Block Diagram](/images/BlockDiagram.png)

## Demo

Watch the gameplay video below:

<video width="640" height="480" controls>
    <source src="/images/demo.mp4" type="video/mp4">
</video>

## Development Overview

First, I made a game that can get the camera feed and have the game work without any human feedback. The scoring system and everything worked. Then I made my model with EdgeImpulse and trained it with my hand using 150 images of my hand in different settings, seeing that I had a lot of errors. After all of that, I incorporated it into the game.

![Labeled 150 different images of my hand in different positions and settings](/images/trainingImages.png)
![Result of model](/images/TrainedModel.png)
![Hand inference](/images/rightML.jpg)
![Hand inference](/images/upML.jpg)
![Terminal reading](/images/terminalReading.jpg)

## Challenges

One of the main challenges was ensuring accurate motion detection in varied lighting conditions. Solutions involved training more photos with different environmental lighting. Having the screen on the same plane as the camera to give the user a nice user interface to play along with. The Pico4ML has strong adhesives on the LCD which makes it tricky to remove. Even when the adhesive is removed, having the screen working afterward becomes very unreliable where it even had caught on fire. Running the tinyml inference while the game was running was not reliable and hard to set up. Some of the screens stopped working entirely after a long period of usage.

### Removing Screen

![Failure 1 Pressing the screen down with sticks](/images/failure1.jpg)
![Failure 2 Screen breaks easily](/images/failure2.jpg)

## Reflection on Design/Future Improvements

I think the design was nice and made the game innovative and achieved the design of HHR. I would use a larger screen. This would make the game more interactive and having it on the same plane of the camera it wouldn't stop me from having broken hardware since the current Pico4ML keeps breaking when I put it into the same plane. Training the ML with more photos with other hands. Increasing the difficulty instead of having the speed increase as the game goes on. Making more modes. Having Audio. Lastly having a leaderboard.

## Advantages

- Inclusive Design: Makes dancing games accessible to just needing your hands only and point to that direction.
- Just Pico4ML setup

## PIO Module in Hands Hands Revolution

We utilized the PIO capability of the RP2040 to drive various I/O functions like I2C and SPI more efficiently. Below is the detailed implementation for image processing with PIO.

### PIO Assembly Code

```assembly
.program image
.wrap_target
    wait 1 pin 9  ; wait for hsync
    wait 1 pin 8  ; wait for rising pclk
    in pins 1
    wait 0 pin 8
.wrap

```
###

```c
void image_program_init(PIO pio, uint sm, uint offset, uint pin_base) {
    pio_sm_set_consecutive_pindirs(pio, sm, pin_base, 1, false);

    pio_sm_config c = image_program_get_default_config(offset);
    sm_config_set_in_pins(&c, pin_base);
    sm_config_set_in_shift(&c, false, true, 8);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    pio_sm_init(pio, sm, offset, &c);
    //pio_sm_set_enabled(pio, sm, true);
}
```

