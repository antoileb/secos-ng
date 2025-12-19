/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>
#include <cr.h>
#include <pagemem.h>

#define __usr__ __attribute__((section(".usr")))
#define __usrdata__ __attribute__((section(".usrdata")))

#define PGD_KERN  0x100000
#define PGD_USER1 0x180000
#define PGD_USER2 0x200000
#define SHARED_MEM_PHY_ADDR 0x800000
#define USER1_SHARED_VIRT_ADDR 0xF100000
#define USER2_SHARED_VIRT_ADDR 0xF200000
#define SHARED_COUNTER SHARED_MEM_PHY_ADDR
#define STACK_USR1 0x800800
#define STACK_USR2 0x500000

#define c0_idx  1
#define d0_idx  2
#define c3_idx  3
#define d3_idx  4
#define ts_idx  5

#define c0_sel  gdt_krn_seg_sel(c0_idx)
#define d0_sel  gdt_krn_seg_sel(d0_idx)
#define c3_sel  gdt_usr_seg_sel(c3_idx)
#define d3_sel  gdt_usr_seg_sel(d3_idx)
#define ts_sel  gdt_krn_seg_sel(ts_idx)

seg_desc_t GDT[6];
tss_t      TSS;

#define gdt_flat_dsc(_dSc_,_pVl_,_tYp_)                                 \
   ({                                                                   \
      (_dSc_)->raw     = 0;                                             \
      (_dSc_)->limit_1 = 0xffff;                                        \
      (_dSc_)->limit_2 = 0xf;                                           \
      (_dSc_)->type    = _tYp_;                                         \
      (_dSc_)->dpl     = _pVl_;                                         \
      (_dSc_)->d       = 1;                                             \
      (_dSc_)->g       = 1;                                             \
      (_dSc_)->s       = 1;                                             \
      (_dSc_)->p       = 1;                                             \
   })

#define tss_dsc(_dSc_,_tSs_)                                            \
   ({                                                                   \
      raw32_t addr    = {.raw = _tSs_};                                 \
      (_dSc_)->raw    = sizeof(tss_t);                                  \
      (_dSc_)->base_1 = addr.wlow;                                      \
      (_dSc_)->base_2 = addr._whigh.blow;                               \
      (_dSc_)->base_3 = addr._whigh.bhigh;                              \
      (_dSc_)->type   = SEG_DESC_SYS_TSS_AVL_32;                        \
      (_dSc_)->p      = 1;                                              \
   })

#define c0_dsc(_d) gdt_flat_dsc(_d,0,SEG_DESC_CODE_XR)
#define d0_dsc(_d) gdt_flat_dsc(_d,0,SEG_DESC_DATA_RW)
#define c3_dsc(_d) gdt_flat_dsc(_d,3,SEG_DESC_CODE_XR)
#define d3_dsc(_d) gdt_flat_dsc(_d,3,SEG_DESC_DATA_RW)

void init_gdt() {
   gdt_reg_t gdtr;

   GDT[0].raw = 0ULL;

   c0_dsc( &GDT[c0_idx] );
   d0_dsc( &GDT[d0_idx] );
   c3_dsc( &GDT[c3_idx] );
   d3_dsc( &GDT[d3_idx] );

   gdtr.desc  = GDT;
   gdtr.limit = sizeof(GDT) - 1;
   set_gdtr(gdtr);

   set_cs(c0_sel);

   set_ss(d0_sel);
   set_ds(d0_sel);
   set_es(d0_sel);
   set_fs(d0_sel);
   set_gs(d0_sel);
}

void sys_cnt_handler() {
	asm volatile ("pusha"); // backup general purpose registers
	uint32_t* addr;
	asm volatile("mov %%ebx, %0":"=r"(addr)); // get counter's address from EBX register
	debug("counter = %d\n", *addr); // display value of shared counter
	asm volatile("popa \n\t" // restore and leave
				  "leave \n\t" 
				  "iret");
}

__usr__
void sys_counter(uint32_t* addr) {
	// puts the given address in regiser EBX then syscall (int 80)
	asm volatile ("mov %0, %%ebx \n\t" 
				  "int $80\n"::"r"(addr));

	return;
}

__usr__
void user1() {
	// indefinitly increments a shared counter
	uint32_t* counter = (uint32_t*)USER1_SHARED_VIRT_ADDR;
	while(1) {
		if (*counter < 1000) {
			(*counter)++;
		} else {
			*counter = 0;
		}
	}
}	

__usr__
void user2() {
	uint32_t* counter = (uint32_t*)USER2_SHARED_VIRT_ADDR;
	while (1) {
		sys_counter(counter);
	}
}

void tp() {
	// Init GDT
	init_gdt();
	debug("GDT SET\n");

	// Set up TSS
	TSS.s0.esp = get_ebp();
    TSS.s0.ss  = d0_sel;
    tss_dsc(&GDT[ts_idx], (offset_t)&TSS);
    set_tr(ts_sel);
	debug("TSS SET\n");

	uint32_t* cnt = (uint32_t*)SHARED_COUNTER;
	*cnt = 1234; // init shared counter

	// sys_counter interrupt registration
	idt_reg_t idtr;
	get_idtr(idtr);
	int_desc_t sys_cnt;
	raw32_t off = {.raw = (uint32_t)sys_cnt_handler};
	sys_cnt.offset_1 = off.wlow;
	sys_cnt.offset_2 = off.whigh;
	sys_cnt.selector = 8;
	sys_cnt.dpl = 3;
	sys_cnt.ist = 0;
	sys_cnt.zero_1 = 0;
	sys_cnt.zero_2 = 0;
	sys_cnt.p = 1;
	sys_cnt.type = 14;
	idtr.desc[80] = sys_cnt;


	debug("IDTR SET\n");

	// PGD Kernel Identity Map (0x0 -> 0x3FFFFF)
	pde32_t* pgd_kern = (pde32_t*)PGD_KERN;
	memset((void*)pgd_kern, 0, PAGE_SIZE);
	pte32_t* ptb_kern1 = (pte32_t*)(PGD_KERN + PAGE_SIZE);
	for(int i=0;i<1024;i++) {
		pg_set_entry(&ptb_kern1[i], PG_KRN|PG_RW, i);
	}
	pg_set_entry(&pgd_kern[0], PG_KRN|PG_RW, page_get_nr(ptb_kern1));

	debug("PGD KERN SET\n");

	// PGD User1
	pde32_t* pgd_usr1 = (pde32_t*)PGD_USER1;
	memset((void*)pgd_usr1, 0, PAGE_SIZE);
	
	// Kernel identity mapping
	pte32_t* ptbX = (pte32_t*)(PGD_USER1 + PAGE_SIZE);
	for(int i=0;i<1024;i++) {
		pg_set_entry(&ptbX[i], PG_KRN|PG_RW, i);
	}
	pg_set_entry(&pgd_usr1[0], PG_KRN|PG_RW, page_get_nr(ptbX));
	
	// User program identity mapping
	pte32_t* ptb1 = (pte32_t*)(PGD_USER1 + 2*PAGE_SIZE);
	for(int i=0;i<1024;i++) {
		pg_set_entry(&ptb1[i], PG_USR|PG_RW, 1024+i);
	}
	pg_set_entry(&pgd_usr1[1], PG_USR|PG_RW, page_get_nr(ptb1));
	
	// Shared memory mapping
	pte32_t* ptb2 = (pte32_t*)(PGD_USER1 + 3*PAGE_SIZE);
	uint32_t* target = (uint32_t*)USER1_SHARED_VIRT_ADDR;
	uint32_t pdg_idx = pd32_get_idx(target);
	uint32_t ptb_idx = pt32_get_idx(target);
	memset((void*)ptb2, 0, PAGE_SIZE);
	pg_set_entry(&ptb2[ptb_idx], PG_USR|PG_RW, page_get_nr(SHARED_MEM_PHY_ADDR));
	pg_set_entry(&pgd_usr1[pdg_idx], PG_USR|PG_RW, page_get_nr(ptb2));
	debug("PGD USER1 SET\n");


	// PGD User2
	pde32_t* pgd_usr2 = (pde32_t*)PGD_USER2;
	memset((void*)pgd_usr2, 0, PAGE_SIZE);
	
	// Kernel identity mapping
	ptbX = (pte32_t*)(PGD_USER2 + PAGE_SIZE);
	for(int i=0;i<1024;i++) {
		pg_set_entry(&ptbX[i], PG_KRN|PG_RW, i);
	}
	pg_set_entry(&pgd_usr2[0], PG_KRN|PG_RW, page_get_nr(ptbX));
	
	// User program identity mapping
	pte32_t* ptb3 = (pte32_t*)(PGD_USER2 + 2*PAGE_SIZE);
	for(int i=0;i<1024;i++) {
		pg_set_entry(&ptb3[i], PG_USR|PG_RW, 1024+i);
	}
	pg_set_entry(&pgd_usr2[1], PG_USR|PG_RW, page_get_nr(ptb3));
	
	// Shared memory mapping
	pte32_t* ptb4 = (pte32_t*)(PGD_USER2 + 3*PAGE_SIZE);
	target = (uint32_t*)USER2_SHARED_VIRT_ADDR;
	pdg_idx = pd32_get_idx(target);
	ptb_idx = pt32_get_idx(target);
	memset((void*)ptb4, 0, PAGE_SIZE);
	pg_set_entry(&ptb4[ptb_idx], PG_USR|PG_RW, page_get_nr(SHARED_MEM_PHY_ADDR));
	pg_set_entry(&pgd_usr2[pdg_idx], PG_USR|PG_RW, page_get_nr(ptb4));
	
	debug("PGD USER2 SET\n");

	// ENABLE PAGINATION
	set_cr3((uint32_t)pgd_kern);
	set_cr0(get_cr0() | CR0_PG);
	debug("Pagination activated\n");
	
	// Go to userland (ring 3)
    set_cr3((uint32_t)pgd_usr2);
	
	asm volatile (
    	"push %0 \n" // ss
    	"push %1 \n" // esp pour du ring 3 !
		"pushf   \n" // eflags
		"push %2 \n" // cs
		"push %3 \n" // eip
		"iret"
		::
		"i"(d3_sel),
		"i"(STACK_USR2),
		"i"(c3_sel),
		"r"(&user2)
	);
}
