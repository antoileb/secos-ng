/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <string.h>

void userland() {
   asm volatile ("mov %eax, %cr0");
}

void print_gdt_content(gdt_reg_t gdtr_ptr) {
    seg_desc_t* gdt_ptr;
    gdt_ptr = (seg_desc_t*)(gdtr_ptr.addr);
    int i=0;
    while ((uint32_t)gdt_ptr < ((gdtr_ptr.addr) + gdtr_ptr.limit)) {
        uint32_t start = gdt_ptr->base_3<<24 | gdt_ptr->base_2<<16 | gdt_ptr->base_1;
        uint32_t end;
        if (gdt_ptr->g) {
            end = start + ( (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1) <<12) + 4095;
        } else {
            end = start + (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1);
        }
        debug("%d ", i);
        debug("[0x%x ", start);
        debug("- 0x%x] ", end);
        debug("seg_t: 0x%x ", gdt_ptr->type);
        debug("desc_t: %d ", gdt_ptr->s);
        debug("priv: %d ", gdt_ptr->dpl);
        debug("present: %d ", gdt_ptr->p);
        debug("avl: %d ", gdt_ptr->avl);
        debug("longmode: %d ", gdt_ptr->l);
        debug("default: %d ", gdt_ptr->d);
        debug("gran: %d ", gdt_ptr->g);
        debug("\n");
        gdt_ptr++;
        i++;
    }
}


void tp() {
	

    // lecture de la gdt après grub
    gdt_reg_t gdtr_ptr;
    get_gdtr(gdtr_ptr);
    print_gdt_content(gdtr_ptr);

    // création gdt custom
    seg_desc_t gdt[6];

    gdt[0].raw = 0;
    gdt[0].limit_1 = 0xfff0;
    // CODE
    gdt[1].type = SEG_DESC_CODE_XR;
    gdt[1].base_1 = 0;
    gdt[1].base_2 = 0;
    gdt[1].base_3 = 0;
    gdt[1].limit_1 = 0xffff;
    gdt[1].limit_2 = 0xf;
    gdt[1].s = 1;
    gdt[1].p = 1;
    gdt[1].l = 0;
    gdt[1].d = 1;
    gdt[1].avl = 0;
    gdt[1].g = 1;
    gdt[1].dpl = 0;

    // DATA
    gdt[2].type=SEG_DESC_DATA_RW;
    gdt[2].base_1=0x0;
    gdt[2].base_2=0x0;
    gdt[2].base_3=0x0;
    gdt[2].limit_1 = 0xffff;
    gdt[2].limit_2 = 0xf;
    gdt[2].s = 1;
    gdt[2].p = 1;
    gdt[2].l = 0;
    gdt[2].d = 1;
    gdt[2].avl = 0; 
    gdt[2].g = 1;
    gdt[2].dpl = 0;
    seg_sel_t sel2 = {.rpl=0, .ti=0, .index=2};


    gdt[3].type= SEG_DESC_DATA_RW;
    gdt[3].base_1=0x0000;
    gdt[3].base_2=0x60;
    gdt[3].base_3=0x0;
    gdt[3].limit_1 = 32;
    gdt[3].limit_2 = 0x0;
    gdt[3].s = 1;
    gdt[3].p = 1;
    gdt[3].l = 0;
    gdt[3].d = 1;
    gdt[3].avl = 0;
    gdt[3].g = 0;
    gdt[3].dpl = 0;
    seg_sel_t sel3 = {.rpl=0, .ti=0, .index=3};

    gdt[4].type= SEG_DESC_CODE_XR;
    gdt[4].base_1=0x0000;
    gdt[4].base_2=0x00;
    gdt[4].base_3=0x0;
    gdt[4].limit_1 = 0xffff;
    gdt[4].limit_2 = 0xf;
    gdt[4].s = 1;
    gdt[4].p = 1;
    gdt[4].l = 0;
    gdt[4].d = 1;
    gdt[4].avl = 0;
    gdt[4].g = 1;
    gdt[4].dpl = 3;
    seg_sel_t sel4 = {.rpl=3, .ti=0, .index=4};

    gdt[5].type= SEG_DESC_DATA_RW;
    gdt[5].base_1=0x0000;
    gdt[5].base_2=0x00;
    gdt[5].base_3=0x0;
    gdt[5].limit_1 = 0xffff;
    gdt[5].limit_2 = 0xf;
    gdt[5].s = 1;
    gdt[5].p = 1;
    gdt[5].l = 0;
    gdt[5].d = 1;
    gdt[5].avl = 0;
    gdt[5].g = 1;
    gdt[5].dpl = 3;
    seg_sel_t sel5 = {.rpl=3, .ti=0, .index=5};

    gdt_reg_t gdtr;
    gdtr.desc = gdt; // set addr 
    gdtr.limit = sizeof(gdt);

    // use new GDT, set GDTR and selectors
    set_gdtr(gdtr);
    get_gdtr(gdtr_ptr);
    print_gdt_content(gdtr_ptr);
    set_ds(sel2);
    set_ss(sel2);
    set_fs(sel2);
    set_gs(sel2);
    set_es(sel3);

    uint16_t ss = get_ss();
    uint16_t ds = get_ds();
    uint16_t es = get_es();
    uint16_t fs = get_fs();
    uint16_t gs = get_gs();
    uint16_t cs = get_seg_sel(cs);
    debug("active selectors cs:0x%x ss:0x%x ds:0x%x es:0x%x fs:0x%x gs:0x%x \n", cs, ss, ds, es, fs, gs);

    char  src[64];
    char *dst = 0;
    memset(src, 0xff, 16);
    _memcpy8(dst, src, 32);

    set_ds(sel5);
    debug("ds set\n");
    set_es(sel5);
    debug("es set\n");
    set_fs(sel5);
    debug("fs set\n");    
    set_gs(sel5);
    debug("gs set\n");

    fptr32_t usrld;
    usrld.offset = (uint32_t)&userland;
    usrld.segment = sel4.raw;
    farjump(usrld);

}
