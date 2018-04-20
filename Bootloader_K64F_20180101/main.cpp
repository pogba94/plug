#include "mbed.h"

extern void bootloader(void);

int main() 
{   
    bootloader();
    while(1);
} 
