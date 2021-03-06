#include <stddef.h>

#include "ds/linked.h"
#include "mm.h"
#include "util.h"
#include "libk.h"

#define STACK_MAX (512 - 2)

extern physaddr_t _kernel_end_phys;

struct stack_node {
	struct slist_entry list;
	size_t current_idx;
	// First entry in this array holds the physical address of the node itself
	uintptr_t free_pages[STACK_MAX];
} __attribute__((packed)); // Shouldn't be necessary, but just to be safe

struct stack_node *stack_start;

void boot_heap_init(void) {
	// Map the unused space that we already have allocated since boot
	const physaddr_t kern_end = ALIGNUP((physaddr_t)&_kernel_end_phys, PAGE_SIZE);

	// Make sure all the numbers are correct
	kassert(sizeof(struct stack_node) == PAGE_SIZE);
	kassert(STACK_MAX * sizeof(uintptr_t) + offsetof(struct stack_node, free_pages) == PAGE_SIZE);
	kassert(kern_end + PAGE_SIZE < KERNEL_PHYS_MAP_END);

	stack_start = phys_to_virt(kern_end);
	slist_set_next(stack_start, list, (struct stack_node *)NULL); // Mark as end of list
	stack_start->current_idx = 0;

	// free_pages[current_idx] must always be valid
	stack_start->free_pages[0] = kern_end + PAGE_SIZE;
	boot_heap_free_pages_phys(kern_end + (2 * PAGE_SIZE), (KERNEL_PHYS_MAP_END - kern_end - 2 * PAGE_SIZE) / PAGE_SIZE);
}

/* Only allow 4096-length allocations ATM. Larger allocs will be tricky in the
   future since we will have to do slow O(n^2) searches of the stack. Another
   thing to consider is that this allocator causes pretty large initial
   fragmentation since we allocate right off the top of the stack. This can be
   changed in the future if physical address space fragmentation is an issue.
*/
physaddr_t boot_heap_alloc_page(void) {
	if (stack_start == NULL)
		panic("boot heap can't satisfy page allocation");

	physaddr_t free_addr = stack_start->free_pages[stack_start->current_idx];
	if (free_addr == 0 && stack_start->current_idx == 0) {
		// Allocate the node itself
		free_addr = virt_to_phys((void *)stack_start);
		struct stack_node *next_node = slist_get_next(stack_start, list);
		stack_start = next_node;
	} else if (stack_start->current_idx == 0)
		stack_start->free_pages[0] = 0;
	else
		stack_start->current_idx--;
	return free_addr;
}

void boot_heap_free_pages_kern(kernaddr_t k, size_t n) {
	return boot_heap_free_pages_phys(kern_to_phys(k), n);
}

void boot_heap_free_pages_virt(virtaddr_t k, size_t n) {
	return boot_heap_free_pages_phys(virt_to_phys(k), n);
}

// TODO: Clarity on whether p needs to be page aligned
void boot_heap_free_pages_phys(physaddr_t p, size_t n) {
	kassert(n != 0);
	const physaddr_t kern_end = ALIGNUP((physaddr_t)&_kernel_end_phys, PAGE_SIZE);
	kassert(p >= kern_end);

	// Note: integer overflow should never happen here since addresses are
	// 48-bit (or 52-bit with PSE). TODO: Confirm this
	physaddr_t free_addr = ALIGNUP(p, PAGE_SIZE);
	physaddr_t free_end = p + (n * PAGE_SIZE);

	// TODO: Please test this!
	if (stack_start == NULL && (free_addr + PAGE_SIZE) >= free_end) {
		kprintf("stack_start is null, reallocating...\n");
		// TODO: Slightly dodgy usage of phys_to_virt, please fix
		if (!paging_has_flags(kernel_p4, phys_to_virt(free_addr), PAGE_PRESENT | PAGE_WRITABLE))
			panic("boot heap cannot create space to store free pages");
		stack_start = phys_to_virt(free_addr);
		stack_start->current_idx = 0;
		stack_start->free_pages[0] = free_addr;
		free_addr += PAGE_SIZE;
	}

	kprintf("Freeing physical pages from %p to %p\n", (void *)free_addr, (void *)free_end);
	for (; free_addr + PAGE_SIZE <= free_end; free_addr += PAGE_SIZE) {
		paging_map_page(kernel_p4, free_addr, phys_to_virt(free_addr), PAGE_WRITABLE | PAGE_NO_EXEC);
		if (stack_start->current_idx + 1 >= STACK_MAX) {
			// Need new stack node (TODO: Another dodgy phys_to_virt)
			struct stack_node *next_node = phys_to_virt(boot_heap_alloc_page());
			next_node->current_idx = 0;
			next_node->free_pages[0] = free_addr;
			slist_set_next(next_node, list, stack_start);
			stack_start = next_node;
		} else {
			// Use existing stack node
			stack_start->current_idx++;
			stack_start->free_pages[stack_start->current_idx] = free_addr;
		}
	}
}
