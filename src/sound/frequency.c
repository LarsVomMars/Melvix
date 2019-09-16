#include <stdint.h>
#include "../io/io.h"
#include "../timer/timer.h"

static void play_sound(uint32_t frequency) {
    uint32_t divided;
    uint8_t tmp;

    divided = 1193180 / frequency;
    send(0x43, 0xb6);
    send(0x42, (uint8_t) (divided));
    send(0x42, (uint8_t) (divided >> 8));

    //And play the sound using the PC speaker
    tmp = receive(0x61);
    if (tmp != (tmp | 3)) {
        send(0x61, tmp | 3);
    }
}

static void shut_up() {
    uint8_t tmp = receive(0x61) & 0xFC;

    send(0x61, tmp);
}

//Make a beep
void beep() {
    play_sound(1000);
    timer_wait(100);
    shut_up();
}