#include <algs/oarray.h>
#include <memory/kheap.h>
#include <klog.h>
#include <string.h>

bool oarray_stdlthan_pred(const void * a, const void *b) {
	return (a < b) ? 1 : 0;
}

struct oarray oarray_create(uint32_t max_size, lthan_predicate less_than) {
	struct oarray returner;
	returner.array = (void*)kmalloc(max_size * sizeof(void *));
	memset(returner.array, 0, max_size * sizeof(void *));
	returner.size = 0;
	returner.max_size = max_size;
	returner.less_than = less_than;
	return returner;
}

struct oarray oarray_place(void *addr, uint32_t max_size, lthan_predicate less_than) {
	struct oarray returner;
	returner.array = addr;
	// TODO: This causes virtualbox to hang for ages, not sure why
	memset(returner.array, 0, max_size * sizeof(void *));
	returner.size = 0;
	returner.max_size = max_size;
	returner.less_than = less_than;
	return returner;
}

void oarray_destroy(struct oarray *array) {
	kfree(array->array);
}

void oarray_insert(void *item, struct oarray *array) {
	klog_assert(array->less_than);
	uint32_t iter = 0;
	while (iter < array->size && array->less_than(array->array[iter], item))
		iter++;

	if (iter == array->size)
		array->array[array->size++] = item;
	else {
		void *tmp = array->array[iter];
		array->array[iter] = item;
		while (iter < array->size) {
			iter++;
			void *tmp2 = array->array[iter];
			array->array[iter] = tmp;
			tmp = tmp2;
		}
		array->size++;
	}
}

void *oarray_lookup(uint32_t i, struct oarray *array) {
	klog_assert(i < array->size);
	return array->array[i];
}

void oarray_remove(uint32_t i, struct oarray *array) {
	while (i < array->size) {
		array->array[i] = array->array[i + 1];
		i++;
	}
	array->size--;
}
