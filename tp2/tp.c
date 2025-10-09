/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>

// void bp_handler() {
//    debug("bp_handler triggerd\n");
// }

void bp_handler() {
	asm volatile("pusha");
	uint32_t val;
    asm volatile ("mov 4(%%ebp), %0":"=r"(val));
	debug("ebp-4: 0x%x", val);
	asm volatile("popa \n\t \
				  leave \n\t \
				  iret");

}

void bp_trigger() {
	asm volatile("int3"::);
	debug("returned to bp_trigger\n");
}


void print_idt_content(idt_reg_t idtr_ptr) {
    int_desc_t* idt_ptr;
    idt_ptr = (int_desc_t *)(idtr_ptr.addr);
    int i=0;
    while ((uint32_t)idt_ptr < ((idtr_ptr.addr) + idtr_ptr.limit)) {

		uint64_t offset = idt_ptr->offset_1 << 16 | idt_ptr->offset_2;
		debug("%d offset=0x%llx ", i, offset);
		debug("selector=0x%x ", idt_ptr->selector);
		debug("ist=0x%x ", idt_ptr->ist);
		debug("type=0x%x ", idt_ptr->type);
		debug("dpl=0x%x ", idt_ptr->dpl);
		debug("p=0x%x \n", idt_ptr->p);

	    idt_ptr++;
        i++;
    }
}


void tp() {
	// TODO print idtr
	idt_reg_t idtr;
	get_idtr(idtr);
	print_idt_content(idtr);
	
	int_desc_t bphl;
	raw32_t off = {.raw = (uint32_t)bp_handler};
	bphl.offset_1=off.wlow;
	bphl.offset_2=off.whigh;
	bphl.selector = 8;
	bphl.dpl = 0;
	bphl.ist = 0;
	bphl.zero_1=0;
	bphl.zero_2=0;
	bphl.p=1;
	bphl.type = 14;
	idtr.desc[3] = bphl;

	// TODO call bp_trigger
   bp_trigger();
}
