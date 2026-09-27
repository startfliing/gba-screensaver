#include "tonc.h"
#include "maxmod.h"

#include "terminal.hpp"

#include "attempt1.h"
#include "soundbank.h"
#include "soundbank_bin.h"

#define ENABLE_MUSIC false

//fixed point precision for color to make color transitions smooth
#define FPP 4

//take a color and build a full palette from it
void updatePal(int r, int g, int b){
    int dg = g / 15;
    int db = b / 15;
    int dr = r / 15;
    for(int i = 0; i < 16; i++){
        int ng = clamp((dg * i)>>FPP, 0, 32);
        int nb = clamp((db * i)>>FPP, 0, 32);
        int nr = clamp((dr * i)>>FPP, 0, 32);
        pal_bg_mem[15-i] = RGB15(31-nr, 31-ng, 31-nb);
    }
}
//colAsc[0] = rAsc, ...[1] = gAsc, ...[2] = bAsc
bool colAsc[3] = {false, true, true};

//update the color palette is based on
void updateColor(int* r, int* g, int* b){
    int* temp[3] = {r,g,b};
    int color = qran() % 3;
    int rand = qran()>>11;
    int diff = colAsc[color] ? rand : -1 * rand;
    *temp[color] = clamp((*temp[color] + diff), 0, (32<<FPP));
    //*temp[i] += diff;
    if((*temp[color]) <= (0<<FPP)) colAsc[color] = true;
    if((*temp[color]) >= (31<<FPP)) colAsc[color] = false;
}

//Horizontal Offset Precision
#define HOFSP 2

int hofs = 0;

void updateHOFS(){
    
    int tempHOFS = hofs%(32<<(HOFSP));
    REG_BG0HOFS = (tempHOFS < (16<<(HOFSP)) ? tempHOFS : (32<<(HOFSP))-tempHOFS)>>HOFSP;
    hofs++;
}

//Vertical Offset Precision
#define VOFSP 1



int main(){
    //enable Border BG
    u8 cbb = 0;
    u8 sbb = 16;
    REG_BG0CNT = BG_BUILD(cbb, sbb, 2, 0, 1, 0, 1);

    //load tiles
    LZ77UnCompVram(attempt1Tiles, tile_mem[cbb]);
    
    //load image
    LZ77UnCompVram(attempt1Map, &se_mem[sbb]);

    //enable Text BG
    REG_DISPCNT = DCNT_BG0 | DCNT_MODE0;

    // Initialize Interrupts
    irq_init(nullptr);

	// Maxmod requires the vblank interrupt to reset sound DMA. 
	irq_set( II_VBLANK, mmVBlank, 0);
	irq_enable(II_VBLANK);

    irq_set(II_HBLANK, updateHOFS, 0);
    irq_enable(II_HBLANK);

    int r = 31<<FPP, g = 0, b = 0;
    updatePal(r, g, b);

    int vofs = 0;

    while(1){
        
        updateColor(&r, &g, &b);

        updatePal(r, g, b);

        REG_BG0VOFS = vofs>>VOFSP;
        vofs++;
        hofs = vofs;

        //update random nunmber
        qran();

        //poll what keys are down
        key_poll();

        //helps with visual tearing
        VBlankIntrWait();

        

    }
}