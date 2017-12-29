#include <ruby.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static VALUE lfa;

#define SEGMENT_CAPACITY 16

typedef struct lfarray {
	uint32_t length;
	uint32_t is_inline;
	//uint32_t used;
	void* values;
} lfarray;

typedef struct segment {
	uint32_t left;
	uint32_t used; // used as a bitvector
	struct segment* next;
	int values[SEGMENT_CAPACITY];
} segment;

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define CLEAR_BIT(var,pos) ((var) &= ~(1<<(pos)))
#define SET_BIT(var,pos) ((var) |= (1<<(pos)))

static int constructed = 0;

static void lfa_free(void* ptr) {
	if (!ptr)
		return;

	lfarray* lfap = ptr;
	//xfree(lfap->values);
	xfree(lfap);
}

static size_t lfa_memsize(const void* ptr) {
	if (!ptr)
		return 0;

	return sizeof(lfarray);
}


static const rb_data_type_t lfa_type = {
	"LFA",
	{0, lfa_free, lfa_memsize,},
	0, 0,
	RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE lfa_alloc(VALUE klass) {
	//printf("allocator called\n");
	if (constructed) {
		return Qnil;
	}
	return TypedData_Wrap_Struct(klass, &lfa_type, 0);
}

static VALUE lfa_initialize(VALUE obj) {

	lfarray* arr = xmalloc(sizeof(lfarray) + SEGMENT_CAPACITY * sizeof(int));
	arr->values = (void*) ((char*) arr + sizeof(lfarray));
	//printf("VALUES %p\n", &arr->values);
	memset(arr->values, 0, SEGMENT_CAPACITY * sizeof(int));
	arr->length = 16;
	arr->is_inline = 1;
	//arr->used = 0;

	DATA_PTR(obj) = arr;
	constructed = 1;
	return obj;

}


static VALUE lfa_get_item_slow(lfarray* arr, uint32_t idx) {
	
	segment* iter = (segment*) arr->values;

	while (iter && (iter->left > idx || idx > iter->left + SEGMENT_CAPACITY - 1)) {
		iter = iter->next;
	}

	if (!iter) {
		return Qnil;
	}
	if (!CHECK_BIT(iter->used, idx - iter->left)) {
		return Qnil;
	}

	return INT2NUM(iter->values[idx - iter->left]);
}


static VALUE lfa_get_item(VALUE self, VALUE idx) {
	
	lfarray *lfap;

	if (TYPE(idx) != T_FIXNUM) {
		return Qnil;
	}

	int real_idx = NUM2INT(idx);
	/*printf("DEBUG: GET ITEM INDEX %d\n", real_idx);*/
	if (real_idx < 0) {
		return Qnil;
	}

	TypedData_Get_Struct(self, lfarray, &lfa_type, lfap);

	/*printf("DEBUG: ARRAY LENGTH %u\n", lfap->length);*/
	// todo treat negative index
	if (real_idx >= lfap->length) {
		return Qnil;
	}

	if (lfap->is_inline) {
		/*printf("inline access\n");*/
		//if (CHECK_BIT(lfap->used, real_idx))
		return INT2NUM(((int*) lfap->values)[real_idx]);
	}

	return lfa_get_item_slow(lfap, (uint32_t) real_idx);
}

static segment* create_segment(uint32_t left) {
	segment* seg = xmalloc(sizeof(segment));
	seg->left = left;
	seg->used = 0;
	seg->next = NULL;
	memset(seg->values, 0, SEGMENT_CAPACITY * sizeof(int));
	return seg;
}

static VALUE lfa_set_item_slow(lfarray* arr, uint32_t idx, int val) {

	if (arr->is_inline) {
		arr->is_inline = 0;
		segment* seg = create_segment(0);
		seg->left = 0;
		seg->used = 0xffff; // special case for first segment
		memcpy(seg->values, arr->values, SEGMENT_CAPACITY * sizeof(int));
		arr->values = (void*) seg;
	}

	segment* prev = NULL;
	segment* iter = (segment*) arr->values;

	while (iter && (iter->left > idx || idx > iter->left + SEGMENT_CAPACITY - 1)) {
		prev = iter;
		iter = iter->next;
	}

	if (iter) {
		iter->values[idx - iter->left] = val;
		SET_BIT(iter->used, idx - iter->left);
		return Qtrue;
	}

	segment* newseg = create_segment(idx);
	newseg->values[0] = val;
	SET_BIT(newseg->used, 0);
	prev->next = newseg;
	return Qtrue;
}

static VALUE lfa_set_item(VALUE self, VALUE idx, VALUE val) {
	
	lfarray *lfap;

	if (TYPE(idx) != T_FIXNUM) {
		return Qfalse;
	}

	int real_idx = NUM2INT(idx);
	if (real_idx < 0) {
		return Qfalse;
	}
	int real_val = NUM2INT(val);
	TypedData_Get_Struct(self, lfarray, &lfa_type, lfap);


	// fast path-inline
	if (lfap->is_inline && real_idx < lfap->length) {
		/*printf("DEBUG: SET FAST PATH INLINE\n");*/
		//SET_BIT(lfap->used, real_idx);
		((int*)lfap->values)[real_idx] = real_val;
		return Qtrue;
	}
	if (real_idx >= lfap->length) {
		lfap->length = real_idx + 1;
	}
	return lfa_set_item_slow(lfap, (uint32_t) real_idx, real_val);
}


static void remove_unused_segment(lfarray* arr) {
	
	segment* prev = NULL;
	segment* iter = (segment*) arr->values;

	while (iter && iter->used) {
		prev = iter;
		iter = iter->next;
	}

	if (!prev) {
		// first segment to be removed
		arr->values = (void*) iter->next;
	} else {
		prev->next = iter->next;
	}

	segment* seg  = (segment*) arr->values;
	// check what happens if segment 0-15 is deleted when another segment is still in used
	// and then that segment is also deleted
	if (!seg->next && seg->left == 0) {
		// re-inlining array
		/*printf("re-inlining\n");*/
		arr->values = (void*) ((char*) arr + sizeof(lfarray));
		memcpy(arr->values, seg->values, SEGMENT_CAPACITY * sizeof(int));
		arr->is_inline = 1;
	}

}

static VALUE lfa_remove_slow(lfarray* arr, uint32_t idx) {

	segment* iter = (segment*) arr->values;

	while (iter && (iter->left > idx || idx > iter->left + SEGMENT_CAPACITY - 1)) {
		iter = iter->next;
	}

	if (!iter) {
		return Qfalse;
	}
	if (!CHECK_BIT(iter->used, idx - iter->left)) {
		return Qfalse;
	}
	CLEAR_BIT(iter->used, idx - iter->left);
	if (!iter->used) {
		remove_unused_segment(arr);
	}
	return Qtrue;
}



static VALUE lfa_remove(VALUE self, VALUE idx) {

	lfarray *lfap;
	if (TYPE(idx) != T_FIXNUM) {
		return Qfalse;
	}

	int real_idx = NUM2INT(idx);
	if (real_idx < 0) {
		return Qfalse;
	}
	TypedData_Get_Struct(self, lfarray, &lfa_type, lfap);

	if (real_idx >= lfap->length) {
		return Qfalse;
	}

	
	if (lfap->is_inline && real_idx < lfap->length) {
		/*if (CHECK_BIT(lfap->used, real_idx)) {*/
			/*CLEAR_BIT(lfap->used, real_idx);*/
			/*return Qtrue;*/
		/*}*/
		return Qfalse;
	}
	return lfa_remove_slow(lfap, (uint32_t) real_idx);

}


static VALUE lfa_sum_slow(lfarray* arr) {

		
	segment* iter = (segment*) arr->values;
	int ret = 0;

	while (iter) {

		for (int i = 0; i < SEGMENT_CAPACITY; ++i) {
			ret += iter->values[i];
		}
		
		iter = iter->next;
	}

	return INT2NUM(ret);
}

static VALUE lfa_sum(VALUE self) {

	lfarray *lfap;
	TypedData_Get_Struct(self, lfarray, &lfa_type, lfap);


	if (lfap->is_inline) {
		int ret = 0;
		int* vals = (int*) lfap->values;
		for (int i = 0; i < SEGMENT_CAPACITY; ++i) {
			ret += vals[i];
		}
		return INT2NUM(ret);
	}

	return lfa_sum_slow(lfap);

}

void Init_LFA() {
	lfa = rb_define_class("LFA", rb_cObject);

	rb_define_alloc_func(lfa, lfa_alloc);

	rb_define_method(lfa, "initialize", lfa_initialize, 0);
	rb_define_method(lfa, "[]", lfa_get_item, 1);
	rb_define_method(lfa, "[]=", lfa_set_item, 2);
	rb_define_method(lfa, "remove", lfa_remove, 1);
	rb_define_method(lfa, "sum", lfa_sum, 0);


}
