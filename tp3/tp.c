/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>

void userland() {
    asm volatile("nop");
    asm volatile("mov 4, %eax");
    asm volatile("int3");
    //asm volatile("mov %eax, %cr0");
}

void tp() {
    gdt_reg_t gdtr_ptr;
    get_gdtr(gdtr_ptr);
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
    seg_sel_t sel1 = {.rpl = 0, .ti = 0, .index = 1};
    // DATA
    gdt[2].type = SEG_DESC_DATA_RW;
    gdt[2].base_1 = 0x0;
    gdt[2].base_2 = 0x0;
    gdt[2].base_3 = 0x0;
    gdt[2].limit_1 = 0xffff;
    gdt[2].limit_2 = 0xf;
    gdt[2].s = 1;
    gdt[2].p = 1;
    gdt[2].l = 0;
    gdt[2].d = 1;
    gdt[2].avl = 0;
    gdt[2].g = 1;
    gdt[2].dpl = 0;
    //seg_sel_t sel2 = {.rpl = 0, .ti = 0, .index = 2};
    gdt[3].type = SEG_DESC_DATA_RW;
    gdt[3].base_1 = 0x0000;
    gdt[3].base_2 = 0x60;
    gdt[3].base_3 = 0x0;
    gdt[3].limit_1 = 32;
    gdt[3].limit_2 = 0x0;
    gdt[3].s = 1;
    gdt[3].p = 1;
    gdt[3].l = 0;
    gdt[3].d = 1;
    gdt[3].avl = 0;
    gdt[3].g = 0;
    gdt[3].dpl = 0;
    //seg_sel_t sel3 = {.rpl = 0, .ti = 0, .index = 3};
    gdt[4].type = SEG_DESC_CODE_XR;
    gdt[4].base_1 = 0x0000;
    gdt[4].base_2 = 0x00;
    gdt[4].base_3 = 0x0;
    gdt[4].limit_1 = 0xffff;
    gdt[4].limit_2 = 0xf;
    gdt[4].s = 1;
    gdt[4].p = 1;
    gdt[4].l = 0;
    gdt[4].d = 1;
    gdt[4].avl = 0;
    gdt[4].g = 1;
    gdt[4].dpl = 3;
    //seg_sel_t sel4 = {.rpl = 3, .ti = 0, .index = 4};
    gdt[5].type = SEG_DESC_DATA_RW;
    gdt[5].base_1 = 0x0000;
    gdt[5].base_2 = 0x00;
    gdt[5].base_3 = 0x0;
    gdt[5].limit_1 = 0xffff;
    gdt[5].limit_2 = 0xf;
    gdt[5].s = 1;
    gdt[5].p = 1;
    gdt[5].l = 0;
    gdt[5].d = 1;
    gdt[5].avl = 0;
    gdt[5].g = 1;
    gdt[5].dpl = 3;
    //seg_sel_t sel5 = {.rpl = 3, .ti = 0, .index = 5};

    gdt_reg_t gdtr;
    gdtr.desc = gdt; // set addr 
    gdtr.limit = sizeof(gdt);
    set_gdtr(gdtr);

    asm volatile ("push %%ss \n\t"
                   "push %%esp \n\t"
                   "pushf \n\t"
                   "push %0 \n\t" 
                   "push %1\n\t"
                   "iret"::"r"(sel1), "r"(userland));


    debug("fin tp\n");
}
