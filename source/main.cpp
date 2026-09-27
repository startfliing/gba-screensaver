#include "tonc.h"

#include "attempt1.h"

//fixed point precision for color to make color transitions smooth
#define FPP 4

//take a color and build a full palette from it
void updatePal(int pal, int r, int g, int b){
    int dg = g / 15;
    int db = b / 15;
    int dr = r / 15;
    for(int i = 0; i < 16; i++){
        int ng = clamp((dg * i)>>FPP, 0, 32);
        int nb = clamp((db * i)>>FPP, 0, 32);
        int nr = clamp((dr * i)>>FPP, 0, 32);
        pal_bg_bank[pal][i] = RGB15(31-nr, 31-ng, 31-nb);
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
    REG_BG1CNT = BG_BUILD(cbb, sbb+2, 2, 0, 0, 0, 1);

    //load tiles
    LZ77UnCompVram(attempt1Tiles, tile_mem[cbb]);
    
    //load image
    LZ77UnCompVram(attempt1Map, &se_mem[sbb]);

    //load image
    LZ77UnCompVram(attempt1Map, &se_mem[sbb+2]);
    for(int i = 0; i < 32*64; i++){
        se_mem[sbb+2][i] |= SE_PALBANK(1);
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
    REG_BLDALPHA = BLDA_BUILD(6,8);

    int r1 = qran(), g1 = qran(), b1 = qran();
    int r2 = qran(), g2 = qran(), b2 = qran();
    updatePal(0, r1, g1, b1);
    updatePal(1, r2, g2, b2);

    int vofs = 0;

    while(1){
        
        updateColor(&r1, &g1, &b1);
        updateColor(&r2, &g2, &b2);

        updatePal(0, r1, g1, b1);
        updatePal(1, r2, g2, b2);

        REG_BG0VOFS = vofs>>VOFSP;
        REG_BG1VOFS = (vofs+73)>>VOFSP;
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