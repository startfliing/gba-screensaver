#include "tonc.h"

#include "attempt2_0.h"
#include "attempt2_1.h"

//fixed point precision for color to make color transitions smooth
#define FPP 4

/** Oscillate a value between a lower and upper bound
 * 
 * @param val The true val, should not be affected by oscillate
 * @param min The lower bound of the oscillation
 * @param max The upper bound of the oscillation
 * 
 * @return value between `min` and `max`
 */
int oscillate(int val, int min, int max){
    int diff = max - min;
    int temp = (val-min) % (diff*2);
    return temp <= diff ? min + temp : min + ((diff*2) - temp); 
}

//take a color and build a full palette from it
void updatePal(int pal, int r, int g, int b){
    int dg = g / 14;
    int db = b / 14;
    int dr = r / 14;
    for(int i = 0; i < 15; i++){
        int ng = clamp((dg * i)>>FPP, 0, 32);
        int nb = clamp((db * i)>>FPP, 0, 32);
        int nr = clamp((dr * i)>>FPP, 0, 32);
        pal_bg_bank[pal][i+1] = RGB15(31-nr, 31-ng, 31-nb);
    }
}
//colAsc[0] = rAsc, ...[1] = gAsc, ...[2] = bAsc
bool colAsc[2][3] = {
    {false, true, true},
    {true, false, true}
};

//update the color palette is based on
void updateColor(int pal, int* r, int* g, int* b){
    int* temp[3] = {r,g,b};
    int color = qran() % 3;
    int rand = qran()>>11;
    int diff = colAsc[pal][color] ? rand : -1 * rand;
    *temp[color] = clamp((*temp[color] + diff), 0, (32<<FPP));
    //*temp[i] += diff;
    if((*temp[color]) <= (0<<FPP)) colAsc[pal][color] = true;
    if((*temp[color]) >= (31<<FPP)) colAsc[pal][color] = false;
}

//Horizontal Offset Precision
#define HOFSP 7

int hofs = 0;

void updateHOFS(){
    REG_BG0HOFS = oscillate(hofs*71, 0, 16<<HOFSP)>>HOFSP;
    REG_BG1HOFS = oscillate(hofs*73, 0, 16<<HOFSP)>>HOFSP;
    hofs++;
}

//Vertical Offset Precision
#define VOFSP 2


int main(){
    //enable Border BG
    u8 cbb = 0;
    u8 sbb = 16;
    REG_BG0CNT = BG_BUILD(cbb, sbb, 2, 0, 1, 0, 2); //slimmer
    REG_BG1CNT = BG_BUILD(cbb, sbb+2, 2, 0, 0, 0, 1); //wider

    //load tiles
    LZ77UnCompVram(attempt2_0Tiles, tile_mem[0]);
    LZ77UnCompVram(attempt2_1Tiles, tile_mem[1]);
    
    //load image
    LZ77UnCompVram(attempt2_0Map, &se_mem[sbb]);

    //load image
    LZ77UnCompVram(attempt2_1Map, &se_mem[sbb+2]);
    for(int i = 0; i < 32*64; i++){
        se_mem[sbb+2][i] |= SE_PALBANK(1) | 512;
    }

    //enable Text BG
    REG_DISPCNT = DCNT_BG0 | DCNT_BG1 | DCNT_MODE0;

    // Initialize Interrupts
    irq_init(nullptr);

	// Maxmod requires the vblank interrupt to reset sound DMA. 
	irq_set( II_VBLANK, nullptr, 0);
	irq_enable(II_VBLANK);

    irq_set(II_HBLANK, updateHOFS, 0);
    irq_enable(II_HBLANK);

    REG_BLDCNT = BLD_BUILD(BLD_BG1, BLD_BG0, 1);
    REG_BLDALPHA = BLDA_BUILD(8,8);

    int r0 = qran(), g0 = qran(), b0 = qran();
    int r1 = qran(), g1 = qran(), b1 = qran();
    updatePal(0, r0, g0, b0);
    updatePal(1, r1, g1, b1);

    int vofs = 0;

    while(1){
        
        updateColor(0, &r0, &g0, &b0);
        updateColor(1, &r1, &g1, &b1);

        updatePal(0, r0, g0, b0);
        updatePal(1, r1, g1, b1);

        REG_BG0VOFS = vofs>>VOFSP;
        REG_BG1VOFS = (-1*vofs)>>VOFSP;
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