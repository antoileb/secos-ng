/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>
#include <cr.h>
#include <pagemem.h>

#define __usr__ __attribute__((section(".usr")))
#define __usrdata__ __attribute__((section(".usrdata")))

#define PGD_KERN  0x100000
#define PGD_USER1 0x410000
#define PGD_USER2 0x420000
#define SHARED_MEM_PHY_ADDR 0x800000
#define USER1_SHARED_VIRT_ADDR 0x100000
#define USER2_SHARED_VIRT_ADDR 0x200000
#define SHARED_COUNTER SHARED_MEM_PHY_ADDR


void sys_cnt_handler() {
	asm volatile ("pusha"); // backup general purpose registers
	uint32_t* addr;
	asm volatile("mov %%ebx, %0":"=r"(addr)); // get counter's address from EBX register
	debug("counter = %d\n", *addr); // display value of shared counter
	asm volatile("popa \n\t" // restore and leave
				  "leave \n\t" 
				  "iret");
}

void sys_counter(uint32_t* addr) {
	// puts the given address in regiser EBX then syscall (int 80)
	asm volatile ("mov %0, %%ebx \n\t" 
				  "int $80\n"::"r"(addr));
}

__usr__
void user1() {
	// indefinitly increments a shared counter
	uint32_t* counter = (uint32_t*)SHARED_COUNTER;
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
	uint32_t* counter = (uint32_t*)SHARED_COUNTER;
	while (1) {
		sys_counter(counter);
	}
}

void tp() {
	uint32_t* cnt = (uint32_t*)SHARED_COUNTER;
	*cnt = 1234; // init shared counter

	// sys_counter interrupt registration
	idt_reg_t idtr;
	get_idtr(idtr);
	int_desc_t sys_cnt;
	raw32_t off = {.raw = (uint32_t)sys_cnt_handler};
	sys_cnt.offset_1=off.wlow;
	sys_cnt.offset_2=off.whigh;
	sys_cnt.selector = 8;
	sys_cnt.dpl = 0;
	sys_cnt.ist = 0;
	sys_cnt.zero_1=0;
	sys_cnt.zero_2=0;
	sys_cnt.p=1;
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
	pte32_t* ptb1 = (pte32_t*)(PGD_USER1 + PAGE_SIZE);
	for(int i=0;i<1024;i++) {
		pg_set_entry(&ptb1[i], PG_USR|PG_RW, i);
	}
	pg_set_entry(&pgd_usr1[1], PG_USR|PG_RW, page_get_nr(ptb1));
	pte32_t* ptb2 = (pte32_t*)(PGD_USER1 + 2*PAGE_SIZE);
	uint32_t* target = (uint32_t*)USER1_SHARED_VIRT_ADDR;
	int pdg_idx = pd32_get_idx(target);
	int ptb_idx = pt32_get_idx(target);
	memset((void*)ptb2, 0, PAGE_SIZE);
	pg_set_entry(&ptb2[ptb_idx], PG_USR|PG_RW, page_get_nr(SHARED_MEM_PHY_ADDR));
	pg_set_entry(&pgd_usr1[pdg_idx], PG_USR|PG_RW, page_get_nr(ptb2));
	debug("PGD USER1 SET\n");


	// PGD User1
	pde32_t* pgd_usr2 = (pde32_t*)PGD_USER2;
	memset((void*)pgd_usr2, 0, PAGE_SIZE);
	pte32_t* ptb3 = (pte32_t*)(PGD_USER2 + PAGE_SIZE);
	for(int i=0;i<1024;i++) {
		pg_set_entry(&ptb3[i], PG_USR|PG_RW, i);
	}
	pg_set_entry(&pgd_usr2[1], PG_USR|PG_RW, page_get_nr(ptb3));
	pte32_t* ptb4 = (pte32_t*)(PGD_USER2 + 2*PAGE_SIZE);
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
	
		
	user2();
}
