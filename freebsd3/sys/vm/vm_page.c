/*
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)vm_page.c	7.4 (Berkeley) 5/7/91
 *	$Id: vm_page.c,v 1.116.2.1 1999/03/23 04:38:33 alc Exp $
 */

/*
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Avadis Tevanian, Jr., Michael Wayne Young
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 *	Resident memory management module.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/vmmeter.h>
#include <sys/vnode.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_prot.h>
#include <sys/lock.h>
#include <vm/vm_kern.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/vm_extern.h>

static void	vm_page_queue_init __P((void));
static vm_page_t vm_page_select_free __P((vm_object_t object,
			vm_pindex_t pindex, int prefqueue));
static vm_page_t vm_page_select_cache __P((vm_object_t, vm_pindex_t));

/*
 *	Associated with page of user-allocatable memory is a
 *	page structure.
 */

static struct pglist *vm_page_buckets;	/* Array of buckets */
static int vm_page_bucket_count;	/* How big is array? */
static int vm_page_hash_mask;		/* Mask for hash function */
static volatile int vm_page_bucket_generation;

struct pglist vm_page_queue_free[PQ_L2_SIZE] = {0};
struct pglist vm_page_queue_zero[PQ_L2_SIZE] = {0};
struct pglist vm_page_queue_active = {0};
struct pglist vm_page_queue_inactive = {0};
struct pglist vm_page_queue_cache[PQ_L2_SIZE] = {0};

static int no_queue=0;

struct vpgqueues vm_page_queues[PQ_COUNT] = {0};
static int pqcnt[PQ_COUNT] = {0};

static void
vm_page_queue_init(void) {
	int i;

	vm_page_queues[PQ_NONE].pl = NULL;
	vm_page_queues[PQ_NONE].cnt = &no_queue;
	for(i=0;i<PQ_L2_SIZE;i++) {
		vm_page_queues[PQ_FREE+i].pl = &vm_page_queue_free[i];
		vm_page_queues[PQ_FREE+i].cnt = &cnt.v_free_count;
	}
	for(i=0;i<PQ_L2_SIZE;i++) {
		vm_page_queues[PQ_ZERO+i].pl = &vm_page_queue_zero[i];
		vm_page_queues[PQ_ZERO+i].cnt = &cnt.v_free_count;
	}
	vm_page_queues[PQ_INACTIVE].pl = &vm_page_queue_inactive;
	vm_page_queues[PQ_INACTIVE].cnt = &cnt.v_inactive_count;

	vm_page_queues[PQ_ACTIVE].pl = &vm_page_queue_active;
	vm_page_queues[PQ_ACTIVE].cnt = &cnt.v_active_count;
	for(i=0;i<PQ_L2_SIZE;i++) {
		vm_page_queues[PQ_CACHE+i].pl = &vm_page_queue_cache[i];
		vm_page_queues[PQ_CACHE+i].cnt = &cnt.v_cache_count;
	}
	for(i=0;i<PQ_COUNT;i++) {
		if (vm_page_queues[i].pl) {
			TAILQ_INIT(vm_page_queues[i].pl);
		} else if (i != 0) {
			panic("vm_page_queue_init: queue %d is null", i);
		}
		vm_page_queues[i].lcnt = &pqcnt[i];
	}
}

vm_page_t vm_page_array = 0;
static int vm_page_array_size = 0;
long first_page = 0;
static long last_page;
static vm_size_t page_mask;
static int page_shift;
int vm_page_zero_count = 0;

/*
 * map of contiguous valid DEV_BSIZE chunks in a page
 * (this list is valid for page sizes upto 16*DEV_BSIZE)
 */
static u_short vm_page_dev_bsize_chunks[] = {
	0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff,
	0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

static __inline int vm_page_hash __P((vm_object_t object, vm_pindex_t pindex));
static int vm_page_freechk_and_unqueue __P((vm_page_t m));
static void vm_page_free_wakeup __P((void));

/*
 *	vm_set_page_size:
 *
 *	Sets the page size, perhaps based upon the memory
 *	size.  Must be called before any use of page-size
 *	dependent functions.
 *
 *	Sets page_shift and page_mask from cnt.v_page_size.
 */
void
vm_set_page_size()
{

	if (cnt.v_page_size == 0)
		cnt.v_page_size = DEFAULT_PAGE_SIZE;
	page_mask = cnt.v_page_size - 1;
	if ((page_mask & cnt.v_page_size) != 0)
		panic("vm_set_page_size: page size not a power of two");
	for (page_shift = 0;; page_shift++)
		if ((1 << page_shift) == cnt.v_page_size)
			break;
}

/*
 *	vm_page_startup:
 *
 *	Initializes the resident memory module.
 *
 *	Allocates memory for the page cells, and
 *	for the object/offset-to-page hash table headers.
 *	Each page cell is initialized and placed on the free list.
 */

vm_offset_t
vm_page_startup(starta, enda, vaddr)
	register vm_offset_t starta;
	vm_offset_t enda;
	register vm_offset_t vaddr;
{
	register vm_offset_t mapped;
	register vm_page_t m;
	register struct pglist *bucket;
	vm_size_t npages, page_range;
	register vm_offset_t new_start;
	int i;
	vm_offset_t pa;
	int nblocks;
	vm_offset_t first_managed_page;

	/* the biggest memory array is the second group of pages */
	vm_offset_t start;
	vm_offset_t biggestone, biggestsize;

	vm_offset_t total;

	total = 0;
	biggestsize = 0;
	biggestone = 0;
	nblocks = 0;
	vaddr = round_page(vaddr);

	for (i = 0; phys_avail[i + 1]; i += 2) {
		phys_avail[i] = round_page(phys_avail[i]);
		phys_avail[i + 1] = trunc_page(phys_avail[i + 1]);
	}

	for (i = 0; phys_avail[i + 1]; i += 2) {
		int size = phys_avail[i + 1] - phys_avail[i];

		if (size > biggestsize) {
			biggestone = i;
			biggestsize = size;
		}
		++nblocks;
		total += size;
	}

	start = phys_avail[biggestone];

	/*
	 * Initialize the queue headers for the free queue, the active queue
	 * and the inactive queue.
	 */

	vm_page_queue_init();

	/*
	 * Allocate (and initialize) the hash table buckets.
	 *
	 * The number of buckets MUST BE a power of 2, and the actual value is
	 * the next power of 2 greater than the number of physical pages in
	 * the system.
	 *
	 * Note: This computation can be tweaked if desired.
	 */
	vm_page_buckets = (struct pglist *) vaddr;
	bucket = vm_page_buckets;
	if (vm_page_bucket_count == 0) {
		vm_page_bucket_count = 1;
		while (vm_page_bucket_count < atop(total))
			vm_page_bucket_count <<= 1;
	}
	vm_page_hash_mask = vm_page_bucket_count - 1;

	/*
	 * Validate these addresses.
	 */

	new_start = start + vm_page_bucket_count * sizeof(struct pglist);
	new_start = round_page(new_start);
	mapped = round_page(vaddr);
	vaddr = pmap_map(mapped, start, new_start,
	    VM_PROT_READ | VM_PROT_WRITE);
	start = new_start;
	vaddr = round_page(vaddr);
	bzero((caddr_t) mapped, vaddr - mapped);

	for (i = 0; i < vm_page_bucket_count; i++) {
		TAILQ_INIT(bucket);
		bucket++;
	}

	/*
	 * Compute the number of pages of memory that will be available for
	 * use (taking into account the overhead of a page structure per
	 * page).
	 */

	first_page = phys_avail[0] / PAGE_SIZE;
	last_page = phys_avail[(nblocks - 1) * 2 + 1] / PAGE_SIZE;

	page_range = last_page - (phys_avail[0] / PAGE_SIZE);
	npages = (total - (page_range * sizeof(struct vm_page)) -
	    (start - phys_avail[biggestone])) / PAGE_SIZE;

	/*
	 * Initialize the mem entry structures now, and put them in the free
	 * queue.
	 */
	vm_page_array = (vm_page_t) vaddr;
	mapped = vaddr;

	/*
	 * Validate these addresses.
	 */
	new_start = round_page(start + page_range * sizeof(struct vm_page));
	mapped = pmap_map(mapped, start, new_start,
	    VM_PROT_READ | VM_PROT_WRITE);
	start = new_start;

	first_managed_page = start / PAGE_SIZE;

	/*
	 * Clear all of the page structures
	 */
	bzero((caddr_t) vm_page_array, page_range * sizeof(struct vm_page));
	vm_page_array_size = page_range;

	/*
	 * Construct the free queue(s) in descending order (by physical
	 * address) so that the first 16MB of physical memory is allocated
	 * last rather than first.  On large-memory machines, this avoids
	 * the exhaustion of low physical memory before isa_dmainit has run.
	 */
	cnt.v_page_count = 0;
	cnt.v_free_count = 0;
	for (i = 0; phys_avail[i + 1] && npages > 0; i += 2) {
		if (i == biggestone)
			pa = ptoa(first_managed_page);
		else
			pa = phys_avail[i];
		while (pa < phys_avail[i + 1] && npages-- > 0) {
			++cnt.v_page_count;
			++cnt.v_free_count;
			m = PHYS_TO_VM_PAGE(pa);
			m->phys_addr = pa;
			m->flags = 0;
			m->pc = (pa >> PAGE_SHIFT) & PQ_L2_MASK;
			m->queue = m->pc + PQ_FREE;
			TAILQ_INSERT_HEAD(vm_page_queues[m->queue].pl, m, pageq);
			++(*vm_page_queues[m->queue].lcnt);
			pa += PAGE_SIZE;
		}
	}
	return (mapped);
}

/*
 *	vm_page_hash:
 *
 *	Distributes the object/offset key pair among hash buckets.
 *
 *	NOTE:  This macro depends on vm_page_bucket_count being a power of 2.
 *	This routine may not block.
 */
static __inline int
vm_page_hash(object, pindex)
	vm_object_t object;
	vm_pindex_t pindex;
{
	return ((((uintptr_t) object) >> 5) + (pindex >> 1)) & vm_page_hash_mask;
}

/*
 *	vm_page_insert:		[ internal use only ]
 *
 *	Inserts the given mem entry into the object and object list.
 *
 *	The pagetables are not updated but will presumably fault the page
 *	in if necessary, or if a kernel page the caller will at some point
 *	enter the page into the kernel's pmap.  We are not allowed to block
 *	here so we *can't* do this anyway.
 *
 *	The object and page must be locked, and must be splhigh.
 *	This routine may not block.
 */

void
vm_page_insert(m, object, pindex)
	register vm_page_t m;
	register vm_object_t object;
	register vm_pindex_t pindex;
{
	register struct pglist *bucket;

	if (m->object != NULL)
		panic("vm_page_insert: already inserted");

	/*
	 * Record the object/offset pair in this page
	 */

	m->object = object;
	m->pindex = pindex;

	/*
	 * Insert it into the object_object/offset hash table
	 */

	bucket = &vm_page_buckets[vm_page_hash(object, pindex)];
	TAILQ_INSERT_TAIL(bucket, m, hashq);
	vm_page_bucket_generation++;

	/*
	 * Now link into the object's list of backed pages.
	 */

	TAILQ_INSERT_TAIL(&object->memq, m, listq);
	m->object->page_hint = m;
	m->object->generation++;

	if (m->wire_count)
		object->wire_count++;

	if ((m->queue - m->pc) == PQ_CACHE)
		object->cache_count++;

	/*
	 * And show that the object has one more resident page.
	 */

	object->resident_page_count++;
}

/*
 *	vm_page_remove:		[ internal use only ]
 *				NOTE: used by device pager as well -wfj
 *
 *	Removes the given mem entry from the object/offset-page
 *	table and the object page list.
 *
 *	The object and page must be locked, and at splhigh.
 *	This routine may not block.
 *
 *	I do not think the underlying pmap entry (if any) is removed here.
 */

void
vm_page_remove(m)
	register vm_page_t m;
{
	register struct pglist *bucket;
	vm_object_t object;

	if (m->object == NULL)
		return;

#if !defined(MAX_PERF)
	if ((m->flags & PG_BUSY) == 0) {
		panic("vm_page_remove: page not busy");
	}
#endif
	
	vm_page_flag_clear(m, PG_BUSY);
	if (m->flags & PG_WANTED) {
		vm_page_flag_clear(m, PG_WANTED);
		wakeup(m);
	}

	object = m->object;
	if (object->page_hint == m)
		object->page_hint = NULL;

	if (m->wire_count)
		object->wire_count--;

	if ((m->queue - m->pc) == PQ_CACHE) 
		object->cache_count--;

	/*
	 * Remove from the object_object/offset hash table
	 */

	bucket = &vm_page_buckets[vm_page_hash(m->object, m->pindex)];
	TAILQ_REMOVE(bucket, m, hashq);
	vm_page_bucket_generation++;

	/*
	 * Now remove from the object's list of backed pages.
	 */

	TAILQ_REMOVE(&object->memq, m, listq);

	/*
	 * And show that the object has one fewer resident page.
	 */

	object->resident_page_count--;
	object->generation++;

	m->object = NULL;
}

/*
 *	vm_page_lookup:
 *
 *	Returns the page associated with the object/offset
 *	pair specified; if none is found, NULL is returned.
 *
 *	The object must be locked.  No side effects.
 *	This routine may not block.
 */

vm_page_t
vm_page_lookup(object, pindex)
	register vm_object_t object;
	register vm_pindex_t pindex;
{
	register vm_page_t m;
	register struct pglist *bucket;
	int generation;

	/*
	 * Search the hash table for this object/offset pair
	 */

	if (object->page_hint && (object->page_hint->pindex == pindex) &&
		(object->page_hint->object == object))
		return object->page_hint;

retry:
	generation = vm_page_bucket_generation;
	bucket = &vm_page_buckets[vm_page_hash(object, pindex)];
	for (m = TAILQ_FIRST(bucket); m != NULL; m = TAILQ_NEXT(m,hashq)) {
		if ((m->object == object) && (m->pindex == pindex)) {
			if (vm_page_bucket_generation != generation)
				goto retry;
			m->object->page_hint = m;
			return (m);
		}
	}
	if (vm_page_bucket_generation != generation)
		goto retry;
	return (NULL);
}

/*
 *	vm_page_rename:
 *
 *	Move the given memory entry from its
 *	current object to the specified target object/offset.
 *
 *	The object must be locked.
 *	This routine may not block.
 *
 *	Note: this routine will raise itself to splvm(), the caller need not. 
 */

void
vm_page_rename(m, new_object, new_pindex)
	register vm_page_t m;
	register vm_object_t new_object;
	vm_pindex_t new_pindex;
{
	int s;

	s = splvm();
	vm_page_remove(m);
	vm_page_insert(m, new_object, new_pindex);
	splx(s);
}

/*
 * vm_page_unqueue_nowakeup:
 *
 * 	vm_page_unqueue() without any wakeup
 *
 *	This routine must be called at splhigh().
 *	This routine may not block.
 */

void
vm_page_unqueue_nowakeup(m)
	vm_page_t m;
{
	int queue = m->queue;
	struct vpgqueues *pq;
	if (queue != PQ_NONE) {
		pq = &vm_page_queues[queue];
		m->queue = PQ_NONE;
		TAILQ_REMOVE(pq->pl, m, pageq);
		(*pq->cnt)--;
		(*pq->lcnt)--;
		if ((queue - m->pc) == PQ_CACHE) {
			if (m->object)
				m->object->cache_count--;
		}
	}
}

/*
 * vm_page_unqueue:
 *
 *	Remove a page from its queue.
 *
 *	This routine must be called at splhigh().
 *	This routine may not block.
 */

void
vm_page_unqueue(m)
	vm_page_t m;
{
	int queue = m->queue;
	struct vpgqueues *pq;
	if (queue != PQ_NONE) {
		m->queue = PQ_NONE;
		pq = &vm_page_queues[queue];
		TAILQ_REMOVE(pq->pl, m, pageq);
		(*pq->cnt)--;
		(*pq->lcnt)--;
		if ((queue - m->pc) == PQ_CACHE) {
			if ((cnt.v_cache_count + cnt.v_free_count) <
				(cnt.v_free_reserved + cnt.v_cache_min))
				pagedaemon_wakeup();
			if (m->object)
				m->object->cache_count--;
		}
	}
}

/*
 *	vm_page_list_find:
 *
 *	Find a page on the specified queue with color optimization.
 *
 *	This routine must be called at splvm().
 *	This routine may not block.
 */
vm_page_t
vm_page_list_find(basequeue, index)
	int basequeue, index;
{
#if PQ_L2_SIZE > 1

	int i,j;
	vm_page_t m;
	int hindex;
	struct vpgqueues *pq;

	pq = &vm_page_queues[basequeue];

	m = TAILQ_FIRST(pq[index].pl);
	if (m)
		return m;

	for(j = 0; j < PQ_L1_SIZE; j++) {
		int ij;
		for(i = (PQ_L2_SIZE / 2) - PQ_L1_SIZE;
			(ij = i + j) > 0;
			i -= PQ_L1_SIZE) {

			hindex = index + ij;
			if (hindex >= PQ_L2_SIZE)
				hindex -= PQ_L2_SIZE;
			if (m = TAILQ_FIRST(pq[hindex].pl))
				return m;

			hindex = index - ij;
			if (hindex < 0)
				hindex += PQ_L2_SIZE;
			if (m = TAILQ_FIRST(pq[hindex].pl))
				return m;
		}
	}

	hindex = index + PQ_L2_SIZE / 2;
	if (hindex >= PQ_L2_SIZE)
		hindex -= PQ_L2_SIZE;
	m = TAILQ_FIRST(pq[hindex].pl);
	if (m)
		return m;

	return NULL;
#else
	return TAILQ_FIRST(vm_page_queues[basequeue].pl);
#endif

}

/*
 *	vm_page_select:
 *
 *	Find a page on the specified queue with color optimization.
 *
 *	This routine must be called at splvm().
 *	This routine may not block.
 */
vm_page_t
vm_page_select(object, pindex, basequeue)
	vm_object_t object;
	vm_pindex_t pindex;
	int basequeue;
{

#if PQ_L2_SIZE > 1
	int index;
	index = (pindex + object->pg_color) & PQ_L2_MASK;
	return vm_page_list_find(basequeue, index);

#else
	return TAILQ_FIRST(vm_page_queues[basequeue].pl);
#endif

}

/*
 *	vm_page_select_cache:
 *
 *	Find a page on the cache queue with color optimization.  As pages
 *	might be found, but not applicable, they are deactivated.  This
 *	keeps us from using potentially busy cached pages.
 *
 *	This routine must be called at splvm().
 *	This routine may not block.
 */
vm_page_t
vm_page_select_cache(object, pindex)
	vm_object_t object;
	vm_pindex_t pindex;
{
	vm_page_t m;

	while (TRUE) {
#if PQ_L2_SIZE > 1
		int index;
		index = (pindex + object->pg_color) & PQ_L2_MASK;
		m = vm_page_list_find(PQ_CACHE, index);

#else
		m = TAILQ_FIRST(vm_page_queues[PQ_CACHE].pl);
#endif
		if (m && ((m->flags & PG_BUSY) || m->busy ||
			       m->hold_count || m->wire_count)) {
			vm_page_deactivate(m);
			continue;
		}
		return m;
	}
}

/*
 *	vm_page_select_free:
 *
 *	Find a free or zero page, with specified preference.
 *
 *	This routine must be called at splvm().
 *	This routine may not block.
 */

static vm_page_t
vm_page_select_free(object, pindex, prefqueue)
	vm_object_t object;
	vm_pindex_t pindex;
	int prefqueue;
{
#if PQ_L2_SIZE > 1
	int i,j;
	int index, hindex;
#endif
	vm_page_t m, mh;
	int oqueuediff;
	struct vpgqueues *pq;

	if (prefqueue == PQ_ZERO)
		oqueuediff = PQ_FREE - PQ_ZERO;
	else
		oqueuediff = PQ_ZERO - PQ_FREE;

	if (mh = object->page_hint) {
		 if (mh->pindex == (pindex - 1)) {
			if ((mh->flags & PG_FICTITIOUS) == 0) {
				if ((mh < &vm_page_array[cnt.v_page_count-1]) &&
					(mh >= &vm_page_array[0])) {
					int queue;
					m = mh + 1;
					if (VM_PAGE_TO_PHYS(m) == (VM_PAGE_TO_PHYS(mh) + PAGE_SIZE)) {
						queue = m->queue - m->pc;
						if (queue == PQ_FREE || queue == PQ_ZERO) {
							return m;
						}
					}
				}
			}
		}
	}

	pq = &vm_page_queues[prefqueue];

#if PQ_L2_SIZE > 1

	index = (pindex + object->pg_color) & PQ_L2_MASK;

	if (m = TAILQ_FIRST(pq[index].pl))
		return m;
	if (m = TAILQ_FIRST(pq[index + oqueuediff].pl))
		return m;

	for(j = 0; j < PQ_L1_SIZE; j++) {
		int ij;
		for(i = (PQ_L2_SIZE / 2) - PQ_L1_SIZE;
			(ij = i + j) >= 0;
			i -= PQ_L1_SIZE) {

			hindex = index + ij;
			if (hindex >= PQ_L2_SIZE)
				hindex -= PQ_L2_SIZE;
			if (m = TAILQ_FIRST(pq[hindex].pl)) 
				return m;
			if (m = TAILQ_FIRST(pq[hindex + oqueuediff].pl))
				return m;

			hindex = index - ij;
			if (hindex < 0)
				hindex += PQ_L2_SIZE;
			if (m = TAILQ_FIRST(pq[hindex].pl)) 
				return m;
			if (m = TAILQ_FIRST(pq[hindex + oqueuediff].pl))
				return m;
		}
	}

	hindex = index + PQ_L2_SIZE / 2;
	if (hindex >= PQ_L2_SIZE)
		hindex -= PQ_L2_SIZE;
	if (m = TAILQ_FIRST(pq[hindex].pl))
		return m;
	if (m = TAILQ_FIRST(pq[hindex+oqueuediff].pl))
		return m;

#else
	if (m = TAILQ_FIRST(pq[0].pl))
		return m;
	else
		return TAILQ_FIRST(pq[oqueuediff].pl);
#endif

	return NULL;
}

/*
 *	vm_page_alloc:
 *
 *	Allocate and return a memory cell associated
 *	with this VM object/offset pair.
 *
 *	page_req classes:
 *	VM_ALLOC_NORMAL		normal process request
 *	VM_ALLOC_SYSTEM		system *really* needs a page
 *	VM_ALLOC_INTERRUPT	interrupt time request
 *	VM_ALLOC_ZERO		zero page
 *
 *	Object must be locked.
 *	This routine may not block.
 *
 *	Additional special handling is required when called from an
 *	interrupt (VM_ALLOC_INTERRUPT).  We are not allowed to mess with
 *	the page cache in this case.
 */
vm_page_t
vm_page_alloc(object, pindex, page_req)
	vm_object_t object;
	vm_pindex_t pindex;
	int page_req;
{
	register vm_page_t m;
	struct vpgqueues *pq;
	vm_object_t oldobject;
	int queue, qtype;
	int s;

	KASSERT(!vm_page_lookup(object, pindex),
		("vm_page_alloc: page already allocated"));

	if ((curproc == pageproc) && (page_req != VM_ALLOC_INTERRUPT)) {
		page_req = VM_ALLOC_SYSTEM;
	};

	s = splvm();

	switch (page_req) {

	case VM_ALLOC_NORMAL:
		if (cnt.v_free_count >= cnt.v_free_reserved) {
			m = vm_page_select_free(object, pindex, PQ_FREE);
			KASSERT(m != NULL, ("vm_page_alloc(NORMAL): missing page on free queue\n"));
		} else {
			m = vm_page_select_cache(object, pindex);
			if (m == NULL) {
				splx(s);
#if defined(DIAGNOSTIC)
				if (cnt.v_cache_count > 0)
					printf("vm_page_alloc(NORMAL): missing pages on cache queue: %d\n", cnt.v_cache_count);
#endif
				vm_pageout_deficit++;
				pagedaemon_wakeup();
				return (NULL);
			}
		}
		break;

	case VM_ALLOC_ZERO:
		if (cnt.v_free_count >= cnt.v_free_reserved) {
			m = vm_page_select_free(object, pindex, PQ_ZERO);
			KASSERT(m != NULL, ("vm_page_alloc(ZERO): missing page on free queue\n"));
		} else {
			m = vm_page_select_cache(object, pindex);
			if (m == NULL) {
				splx(s);
#if defined(DIAGNOSTIC)
				if (cnt.v_cache_count > 0)
					printf("vm_page_alloc(ZERO): missing pages on cache queue: %d\n", cnt.v_cache_count);
#endif
				vm_pageout_deficit++;
				pagedaemon_wakeup();
				return (NULL);
			}
		}
		break;

	case VM_ALLOC_SYSTEM:
		if ((cnt.v_free_count >= cnt.v_free_reserved) ||
		    ((cnt.v_cache_count == 0) &&
		    (cnt.v_free_count >= cnt.v_interrupt_free_min))) {
			m = vm_page_select_free(object, pindex, PQ_FREE);
			KASSERT(m != NULL, ("vm_page_alloc(SYSTEM): missing page on free queue\n"));
		} else {
			m = vm_page_select_cache(object, pindex);
			if (m == NULL) {
				splx(s);
#if defined(DIAGNOSTIC)
				if (cnt.v_cache_count > 0)
					printf("vm_page_alloc(SYSTEM): missing pages on cache queue: %d\n", cnt.v_cache_count);
#endif
				vm_pageout_deficit++;
				pagedaemon_wakeup();
				return (NULL);
			}
		}
		break;

	case VM_ALLOC_INTERRUPT:
		if (cnt.v_free_count > 0) {
			m = vm_page_select_free(object, pindex, PQ_FREE);
			KASSERT(m != NULL, ("vm_page_alloc(INTERRUPT): missing page on free queue\n"));
		} else {
			splx(s);
			vm_pageout_deficit++;
			pagedaemon_wakeup();
			return (NULL);
		}
		break;

	default:
		m = NULL;
#if !defined(MAX_PERF)
		panic("vm_page_alloc: invalid allocation class");
#endif
	}

	queue = m->queue;
	qtype = queue - m->pc;
	if (qtype == PQ_ZERO)
		vm_page_zero_count--;
	pq = &vm_page_queues[queue];
	TAILQ_REMOVE(pq->pl, m, pageq);
	(*pq->cnt)--;
	(*pq->lcnt)--;
	oldobject = NULL;
	if (qtype == PQ_ZERO) {
		m->flags = PG_ZERO | PG_BUSY;
	} else if (qtype == PQ_CACHE) {
		oldobject = m->object;
		vm_page_busy(m);
		vm_page_remove(m);
		m->flags = PG_BUSY;
	} else {
		m->flags = PG_BUSY;
	}
	m->wire_count = 0;
	m->hold_count = 0;
	m->act_count = 0;
	m->busy = 0;
	m->valid = 0;
	m->dirty = 0;
	m->queue = PQ_NONE;

	/*
	 * vm_page_insert() is safe prior to the splx().  Note also that
	 * inserting a page here does not insert it into the pmap (which
	 * could cause us to block allocating memory).  We cannot block 
	 * anywhere.
	 */

	vm_page_insert(m, object, pindex);

	/*
	 * Don't wakeup too often - wakeup the pageout daemon when
	 * we would be nearly out of memory.
	 */
	if (((cnt.v_free_count + cnt.v_cache_count) <
		(cnt.v_free_reserved + cnt.v_cache_min)) ||
			(cnt.v_free_count < cnt.v_pageout_free_min))
		pagedaemon_wakeup();

	if ((qtype == PQ_CACHE) &&
		((page_req == VM_ALLOC_NORMAL) || (page_req == VM_ALLOC_ZERO)) &&
		oldobject && (oldobject->type == OBJT_VNODE) &&
		((oldobject->flags & OBJ_DEAD) == 0)) {
		struct vnode *vp;
		vp = (struct vnode *) oldobject->handle;
		if (vp && VSHOULDFREE(vp)) {
			if ((vp->v_flag & (VFREE|VTBFREE|VDOOMED)) == 0) {
				TAILQ_INSERT_TAIL(&vnode_tobefree_list, vp, v_freelist);
				vp->v_flag |= VTBFREE;
			}
		}
	}
	splx(s);

	return (m);
}

/*
 *	vm_wait:	(also see VM_WAIT macro)
 *
 *	Block until free pages are available for allocation
 */

void
vm_wait()
{
	int s;

	s = splvm();
	if (curproc == pageproc) {
		vm_pageout_pages_needed = 1;
		tsleep(&vm_pageout_pages_needed, PSWP, "vmwait", 0);
	} else {
		if (!vm_pages_needed) {
			vm_pages_needed++;
			wakeup(&vm_pages_needed);
		}
		tsleep(&cnt.v_free_count, PVM, "vmwait", 0);
	}
	splx(s);
}

/*
 *	vm_page_sleep:
 *
 *	Block until page is no longer busy.
 */

int
vm_page_sleep(vm_page_t m, char *msg, char *busy) {
	int slept = 0;
	if ((busy && *busy) || (m->flags & PG_BUSY)) {
		int s;
		s = splvm();
		if ((busy && *busy) || (m->flags & PG_BUSY)) {
			vm_page_flag_set(m, PG_WANTED);
			tsleep(m, PVM, msg, 0);
			slept = 1;
		}
		splx(s);
	}
	return slept;
}

/*
 *	vm_page_activate:
 *
 *	Put the specified page on the active list (if appropriate).
 *
 *	The page queues must be locked.
 *	This routine may not block.
 */
void
vm_page_activate(m)
	register vm_page_t m;
{
	int s;

	s = splvm();
	if (m->queue != PQ_ACTIVE) {
		if ((m->queue - m->pc) == PQ_CACHE)
			cnt.v_reactivated++;

		vm_page_unqueue(m);

		if (m->wire_count == 0) {
			m->queue = PQ_ACTIVE;
			++(*vm_page_queues[PQ_ACTIVE].lcnt);
			TAILQ_INSERT_TAIL(&vm_page_queue_active, m, pageq);
			if (m->act_count < ACT_INIT)
				m->act_count = ACT_INIT;
			cnt.v_active_count++;
		}
	} else {
		if (m->act_count < ACT_INIT)
			m->act_count = ACT_INIT;
	}

	splx(s);
}

/*
 * helper routine for vm_page_free and vm_page_free_zero.
 *
 * This routine may not block.
 */
static int
vm_page_freechk_and_unqueue(m)
	vm_page_t m;
{
	vm_object_t oldobject;

	oldobject = m->object;

#if !defined(MAX_PERF)
	if (m->busy || ((m->queue - m->pc) == PQ_FREE) ||
		(m->hold_count != 0)) {
		printf(
		"vm_page_free: pindex(%lu), busy(%d), PG_BUSY(%d), hold(%d)\n",
		    (u_long)m->pindex, m->busy, (m->flags & PG_BUSY) ? 1 : 0,
		    m->hold_count);
		if ((m->queue - m->pc) == PQ_FREE)
			panic("vm_page_free: freeing free page");
		else
			panic("vm_page_free: freeing busy page");
	}
#endif

	vm_page_unqueue_nowakeup(m);
	vm_page_remove(m);

	if ((m->flags & PG_FICTITIOUS) != 0) {
		return 0;
	}

	m->valid = 0;

	if (m->wire_count != 0) {
#if !defined(MAX_PERF)
		if (m->wire_count > 1) {
			panic("vm_page_free: invalid wire count (%d), pindex: 0x%x",
				m->wire_count, m->pindex);
		}
#endif
		printf("vm_page_free: freeing wired page\n");
		m->wire_count = 0;
		if (m->object)
			m->object->wire_count--;
		cnt.v_wire_count--;
	}

	if (oldobject && (oldobject->type == OBJT_VNODE) &&
		((oldobject->flags & OBJ_DEAD) == 0)) {
		struct vnode *vp;
		vp = (struct vnode *) oldobject->handle;
		if (vp && VSHOULDFREE(vp)) {
			if ((vp->v_flag & (VTBFREE|VDOOMED|VFREE)) == 0) {
				TAILQ_INSERT_TAIL(&vnode_tobefree_list, vp, v_freelist);
				vp->v_flag |= VTBFREE;
			}
		}
	}

#ifdef __alpha__
	pmap_page_is_free(m);
#endif

	return 1;
}

/*
 * helper routine for vm_page_free and vm_page_free_zero.
 *
 * This routine may not block.
 */
static __inline void
vm_page_free_wakeup()
{
	
/*
 * if pageout daemon needs pages, then tell it that there are
 * some free.
 */
	if (vm_pageout_pages_needed) {
		wakeup(&vm_pageout_pages_needed);
		vm_pageout_pages_needed = 0;
	}
	/*
	 * wakeup processes that are waiting on memory if we hit a
	 * high water mark. And wakeup scheduler process if we have
	 * lots of memory. this process will swapin processes.
	 */
	if (vm_pages_needed &&
		((cnt.v_free_count + cnt.v_cache_count) >= cnt.v_free_min)) {
		wakeup(&cnt.v_free_count);
		vm_pages_needed = 0;
	}
}

/*
 *	vm_page_free:
 *
 *	Returns the given page to the free list,
 *	disassociating it with any VM object.
 *
 *	Object and page must be locked prior to entry.
 *	This routine may not block.
 */
void
vm_page_free(m)
	register vm_page_t m;
{
	int s;
	struct vpgqueues *pq;

	s = splvm();

	cnt.v_tfree++;

	if (!vm_page_freechk_and_unqueue(m)) {
		splx(s);
		return;
	}

	m->queue = PQ_FREE + m->pc;
	pq = &vm_page_queues[m->queue];
	++(*pq->lcnt);
	++(*pq->cnt);
	/*
	 * If the pageout process is grabbing the page, it is likely
	 * that the page is NOT in the cache.  It is more likely that
	 * the page will be partially in the cache if it is being
	 * explicitly freed.
	 */
	if (curproc == pageproc) {
		TAILQ_INSERT_TAIL(pq->pl, m, pageq);
	} else {
		TAILQ_INSERT_HEAD(pq->pl, m, pageq);
	}

	vm_page_free_wakeup();
	splx(s);
}

void
vm_page_free_zero(m)
	register vm_page_t m;
{
	int s;
	struct vpgqueues *pq;

	s = splvm();

	cnt.v_tfree++;

	if (!vm_page_freechk_and_unqueue(m)) {
		splx(s);
		return;
	}

	m->queue = PQ_ZERO + m->pc;
	pq = &vm_page_queues[m->queue];
	++(*pq->lcnt);
	++(*pq->cnt);

	TAILQ_INSERT_HEAD(pq->pl, m, pageq);
	++vm_page_zero_count;
	vm_page_free_wakeup();
	splx(s);
}

/*
 *	vm_page_wire:
 *
 *	Mark this page as wired down by yet
 *	another map, removing it from paging queues
 *	as necessary.
 *
 *	The page queues must be locked.
 *	This routine may not block.
 */
void
vm_page_wire(m)
	register vm_page_t m;
{
	int s;

	s = splvm();
	if (m->wire_count == 0) {
		vm_page_unqueue(m);
		cnt.v_wire_count++;
		if (m->object)
			m->object->wire_count++;
	}
	m->wire_count++;
	splx(s);
	(*vm_page_queues[PQ_NONE].lcnt)++;
	vm_page_flag_set(m, PG_MAPPED);
}

/*
 *	vm_page_unwire:
 *
 *	Release one wiring of this page, potentially
 *	enabling it to be paged again.
 *
 *	The page queues must be locked.
 *	This routine may not block.
 */
void
vm_page_unwire(m, activate)
	register vm_page_t m;
	int activate;
{
	int s;

	s = splvm();

	if (m->wire_count > 0) {
		m->wire_count--;
		if (m->wire_count == 0) {
			if (m->object)
				m->object->wire_count--;
			cnt.v_wire_count--;
			if (activate) {
				TAILQ_INSERT_TAIL(&vm_page_queue_active, m, pageq);
				m->queue = PQ_ACTIVE;
				(*vm_page_queues[PQ_ACTIVE].lcnt)++;
				cnt.v_active_count++;
			} else {
				TAILQ_INSERT_TAIL(&vm_page_queue_inactive, m, pageq);
				m->queue = PQ_INACTIVE;
				(*vm_page_queues[PQ_INACTIVE].lcnt)++;
				cnt.v_inactive_count++;
			}
		}
	} else {
#if !defined(MAX_PERF)
		panic("vm_page_unwire: invalid wire count: %d\n", m->wire_count);
#endif
	}
	splx(s);
}


/*
 * Move the specified page to the inactive queue.
 *
 * This routine may not block.
 */
void
vm_page_deactivate(m)
	register vm_page_t m;
{
	int s;

	/*
	 * Ignore if already inactive.
	 */
	if (m->queue == PQ_INACTIVE)
		return;

	s = splvm();
	if (m->wire_count == 0) {
		if ((m->queue - m->pc) == PQ_CACHE)
			cnt.v_reactivated++;
		vm_page_unqueue(m);
		TAILQ_INSERT_TAIL(&vm_page_queue_inactive, m, pageq);
		m->queue = PQ_INACTIVE;
		++(*vm_page_queues[PQ_INACTIVE].lcnt);
		cnt.v_inactive_count++;
	}
	splx(s);
}

/*
 * vm_page_cache
 *
 * Put the specified page onto the page cache queue (if appropriate). 
 * This routine may not block.
 */
void
vm_page_cache(m)
	register vm_page_t m;
{
	int s;

#if !defined(MAX_PERF)
	if ((m->flags & PG_BUSY) || m->busy || m->wire_count) {
		printf("vm_page_cache: attempting to cache busy page\n");
		return;
	}
#endif
	if ((m->queue - m->pc) == PQ_CACHE)
		return;

	vm_page_protect(m, VM_PROT_NONE);
#if !defined(MAX_PERF)
	if (m->dirty != 0) {
		panic("vm_page_cache: caching a dirty page, pindex: %d", m->pindex);
	}
#endif
	s = splvm();
	vm_page_unqueue_nowakeup(m);
	m->queue = PQ_CACHE + m->pc;
	(*vm_page_queues[m->queue].lcnt)++;
	TAILQ_INSERT_TAIL(vm_page_queues[m->queue].pl, m, pageq);
	cnt.v_cache_count++;
	m->object->cache_count++;
	vm_page_free_wakeup();
	splx(s);
}

/*
 * Grab a page, waiting until we are waken up due to the page
 * changing state.  We keep on waiting, if the page continues
 * to be in the object.  If the page doesn't exist, allocate it.
 *
 * This routine may block.
 */
vm_page_t
vm_page_grab(object, pindex, allocflags)
	vm_object_t object;
	vm_pindex_t pindex;
	int allocflags;
{

	vm_page_t m;
	int s, generation;

retrylookup:
	if ((m = vm_page_lookup(object, pindex)) != NULL) {
		if (m->busy || (m->flags & PG_BUSY)) {
			generation = object->generation;

			s = splvm();
			while ((object->generation == generation) &&
					(m->busy || (m->flags & PG_BUSY))) {
				vm_page_flag_set(m, PG_WANTED | PG_REFERENCED);
				tsleep(m, PVM, "pgrbwt", 0);
				if ((allocflags & VM_ALLOC_RETRY) == 0) {
					splx(s);
					return NULL;
				}
			}
			splx(s);
			goto retrylookup;
		} else {
			vm_page_busy(m);
			return m;
		}
	}

	m = vm_page_alloc(object, pindex, allocflags & ~VM_ALLOC_RETRY);
	if (m == NULL) {
		VM_WAIT;
		if ((allocflags & VM_ALLOC_RETRY) == 0)
			return NULL;
		goto retrylookup;
	}

	return m;
}

/*
 * mapping function for valid bits or for dirty bits in
 * a page.  May not block.
 */
__inline int
vm_page_bits(int base, int size)
{
	u_short chunk;

	if ((base == 0) && (size >= PAGE_SIZE))
		return VM_PAGE_BITS_ALL;

	size = (size + DEV_BSIZE - 1) & ~(DEV_BSIZE - 1);
	base &= PAGE_MASK;
	if (size > PAGE_SIZE - base) {
		size = PAGE_SIZE - base;
	}

	base = base / DEV_BSIZE;
	chunk = vm_page_dev_bsize_chunks[size / DEV_BSIZE];
	return (chunk << base) & VM_PAGE_BITS_ALL;
}

/*
 * set a page valid and clean.  May not block.
 */
void
vm_page_set_validclean(m, base, size)
	vm_page_t m;
	int base;
	int size;
{
	int pagebits = vm_page_bits(base, size);
	m->valid |= pagebits;
	m->dirty &= ~pagebits;
	if( base == 0 && size == PAGE_SIZE)
		pmap_clear_modify(VM_PAGE_TO_PHYS(m));
}

/*
 * set a page (partially) invalid.  May not block.
 */
void
vm_page_set_invalid(m, base, size)
	vm_page_t m;
	int base;
	int size;
{
	int bits;

	m->valid &= ~(bits = vm_page_bits(base, size));
	if (m->valid == 0)
		m->dirty &= ~bits;
	m->object->generation++;
}

/*
 * is (partial) page valid?  May not block.
 */
int
vm_page_is_valid(m, base, size)
	vm_page_t m;
	int base;
	int size;
{
	int bits = vm_page_bits(base, size);

	if (m->valid && ((m->valid & bits) == bits))
		return 1;
	else
		return 0;
}

/*
 * update dirty bits from pmap/mmu.  May not block.
 */

void
vm_page_test_dirty(m)
	vm_page_t m;
{
	if ((m->dirty != VM_PAGE_BITS_ALL) &&
	    pmap_is_modified(VM_PAGE_TO_PHYS(m))) {
		m->dirty = VM_PAGE_BITS_ALL;
	}
}

/*
 * This interface is for merging with malloc() someday.
 * Even if we never implement compaction so that contiguous allocation
 * works after initialization time, malloc()'s data structures are good
 * for statistics and for allocations of less than a page.
 */
void *
contigmalloc1(size, type, flags, low, high, alignment, boundary, map)
	unsigned long size;	/* should be size_t here and for malloc() */
	struct malloc_type *type;
	int flags;
	unsigned long low;
	unsigned long high;
	unsigned long alignment;
	unsigned long boundary;
	vm_map_t map;
{
	int i, s, start;
	vm_offset_t addr, phys, tmp_addr;
	int pass;
	vm_page_t pga = vm_page_array;

	size = round_page(size);
#if !defined(MAX_PERF)
	if (size == 0)
		panic("contigmalloc1: size must not be 0");
	if ((alignment & (alignment - 1)) != 0)
		panic("contigmalloc1: alignment must be a power of 2");
	if ((boundary & (boundary - 1)) != 0)
		panic("contigmalloc1: boundary must be a power of 2");
#endif

	start = 0;
	for (pass = 0; pass <= 1; pass++) {
		s = splvm();
again:
		/*
		 * Find first page in array that is free, within range, aligned, and
		 * such that the boundary won't be crossed.
		 */
		for (i = start; i < cnt.v_page_count; i++) {
			int pqtype;
			phys = VM_PAGE_TO_PHYS(&pga[i]);
			pqtype = pga[i].queue - pga[i].pc;
			if (((pqtype == PQ_ZERO) || (pqtype == PQ_FREE) || (pqtype == PQ_CACHE)) &&
			    (phys >= low) && (phys < high) &&
			    ((phys & (alignment - 1)) == 0) &&
			    (((phys ^ (phys + size - 1)) & ~(boundary - 1)) == 0))
				break;
		}

		/*
		 * If the above failed or we will exceed the upper bound, fail.
		 */
		if ((i == cnt.v_page_count) ||
			((VM_PAGE_TO_PHYS(&pga[i]) + size) > high)) {
			vm_page_t m, next;

again1:
			for (m = TAILQ_FIRST(&vm_page_queue_inactive);
				m != NULL;
				m = next) {

				if (m->queue != PQ_INACTIVE) {
					break;
				}

				next = TAILQ_NEXT(m, pageq);
				if (vm_page_sleep(m, "vpctw0", &m->busy))
					goto again1;
				vm_page_test_dirty(m);
				if (m->dirty) {
					if (m->object->type == OBJT_VNODE) {
						vn_lock(m->object->handle, LK_EXCLUSIVE | LK_RETRY, curproc);
						vm_object_page_clean(m->object, 0, 0, OBJPC_SYNC);
						VOP_UNLOCK(m->object->handle, 0, curproc);
						goto again1;
					} else if (m->object->type == OBJT_SWAP ||
								m->object->type == OBJT_DEFAULT) {
						vm_pageout_flush(&m, 1, 0);
						goto again1;
					}
				}
				if ((m->dirty == 0) && (m->busy == 0) && (m->hold_count == 0))
					vm_page_cache(m);
			}

			for (m = TAILQ_FIRST(&vm_page_queue_active);
				m != NULL;
				m = next) {

				if (m->queue != PQ_ACTIVE) {
					break;
				}

				next = TAILQ_NEXT(m, pageq);
				if (vm_page_sleep(m, "vpctw1", &m->busy))
					goto again1;
				vm_page_test_dirty(m);
				if (m->dirty) {
					if (m->object->type == OBJT_VNODE) {
						vn_lock(m->object->handle, LK_EXCLUSIVE | LK_RETRY, curproc);
						vm_object_page_clean(m->object, 0, 0, OBJPC_SYNC);
						VOP_UNLOCK(m->object->handle, 0, curproc);
						goto again1;
					} else if (m->object->type == OBJT_SWAP ||
								m->object->type == OBJT_DEFAULT) {
						vm_pageout_flush(&m, 1, 0);
						goto again1;
					}
				}
				if ((m->dirty == 0) && (m->busy == 0) && (m->hold_count == 0))
					vm_page_cache(m);
			}

			splx(s);
			continue;
		}
		start = i;

		/*
		 * Check successive pages for contiguous and free.
		 */
		for (i = start + 1; i < (start + size / PAGE_SIZE); i++) {
			int pqtype;
			pqtype = pga[i].queue - pga[i].pc;
			if ((VM_PAGE_TO_PHYS(&pga[i]) !=
			    (VM_PAGE_TO_PHYS(&pga[i - 1]) + PAGE_SIZE)) ||
			    ((pqtype != PQ_ZERO) && (pqtype != PQ_FREE) && (pqtype != PQ_CACHE))) {
				start++;
				goto again;
			}
		}

		for (i = start; i < (start + size / PAGE_SIZE); i++) {
			int pqtype;
			vm_page_t m = &pga[i];

			pqtype = m->queue - m->pc;
			if (pqtype == PQ_CACHE) {
				vm_page_busy(m);
				vm_page_free(m);
			}

			TAILQ_REMOVE(vm_page_queues[m->queue].pl, m, pageq);
			(*vm_page_queues[m->queue].lcnt)--;
			cnt.v_free_count--;
			m->valid = VM_PAGE_BITS_ALL;
			m->flags = 0;
			m->dirty = 0;
			m->wire_count = 0;
			m->busy = 0;
			m->queue = PQ_NONE;
			m->object = NULL;
			vm_page_wire(m);
		}

		/*
		 * We've found a contiguous chunk that meets are requirements.
		 * Allocate kernel VM, unfree and assign the physical pages to it and
		 * return kernel VM pointer.
		 */
		tmp_addr = addr = kmem_alloc_pageable(map, size);
		if (addr == 0) {
			/*
			 * XXX We almost never run out of kernel virtual
			 * space, so we don't make the allocated memory
			 * above available.
			 */
			splx(s);
			return (NULL);
		}

		for (i = start; i < (start + size / PAGE_SIZE); i++) {
			vm_page_t m = &pga[i];
			vm_page_insert(m, kernel_object,
				OFF_TO_IDX(tmp_addr - VM_MIN_KERNEL_ADDRESS));
			pmap_kenter(tmp_addr, VM_PAGE_TO_PHYS(m));
			tmp_addr += PAGE_SIZE;
		}

		splx(s);
		return ((void *)addr);
	}
	return NULL;
}

void *
contigmalloc(size, type, flags, low, high, alignment, boundary)
	unsigned long size;	/* should be size_t here and for malloc() */
	struct malloc_type *type;
	int flags;
	unsigned long low;
	unsigned long high;
	unsigned long alignment;
	unsigned long boundary;
{
	return contigmalloc1(size, type, flags, low, high, alignment, boundary,
			     kernel_map);
}

vm_offset_t
vm_page_alloc_contig(size, low, high, alignment)
	vm_offset_t size;
	vm_offset_t low;
	vm_offset_t high;
	vm_offset_t alignment;
{
	return ((vm_offset_t)contigmalloc1(size, M_DEVBUF, M_NOWAIT, low, high,
					  alignment, 0ul, kernel_map));
}

#include "opt_ddb.h"
#ifdef DDB
#include <sys/kernel.h>

#include <ddb/ddb.h>

DB_SHOW_COMMAND(page, vm_page_print_page_info)
{
	db_printf("cnt.v_free_count: %d\n", cnt.v_free_count);
	db_printf("cnt.v_cache_count: %d\n", cnt.v_cache_count);
	db_printf("cnt.v_inactive_count: %d\n", cnt.v_inactive_count);
	db_printf("cnt.v_active_count: %d\n", cnt.v_active_count);
	db_printf("cnt.v_wire_count: %d\n", cnt.v_wire_count);
	db_printf("cnt.v_free_reserved: %d\n", cnt.v_free_reserved);
	db_printf("cnt.v_free_min: %d\n", cnt.v_free_min);
	db_printf("cnt.v_free_target: %d\n", cnt.v_free_target);
	db_printf("cnt.v_cache_min: %d\n", cnt.v_cache_min);
	db_printf("cnt.v_inactive_target: %d\n", cnt.v_inactive_target);
}

DB_SHOW_COMMAND(pageq, vm_page_print_pageq_info)
{
	int i;
	db_printf("PQ_FREE:");
	for(i=0;i<PQ_L2_SIZE;i++) {
		db_printf(" %d", *vm_page_queues[PQ_FREE + i].lcnt);
	}
	db_printf("\n");
		
	db_printf("PQ_CACHE:");
	for(i=0;i<PQ_L2_SIZE;i++) {
		db_printf(" %d", *vm_page_queues[PQ_CACHE + i].lcnt);
	}
	db_printf("\n");

	db_printf("PQ_ZERO:");
	for(i=0;i<PQ_L2_SIZE;i++) {
		db_printf(" %d", *vm_page_queues[PQ_ZERO + i].lcnt);
	}
	db_printf("\n");

	db_printf("PQ_ACTIVE: %d, PQ_INACTIVE: %d\n",
		*vm_page_queues[PQ_ACTIVE].lcnt,
		*vm_page_queues[PQ_INACTIVE].lcnt);
}
#endif /* DDB */
