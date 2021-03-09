#include "rangecoder.h"

typedef struct _compressor comp_t;

struct _compressor {
	int16_t buffer;
	uint32_t counter;
	freq_t frequency[256];
	uint32_t id;
	uint64_u low;
	uint64_u range;
};

static void build_frequency(comp_t *comp);
static void finalize_compressor(comp_t *comp);
static void flush_frequency(comp_t *comp, int scale);
static int get_scale(const comp_t *comp);
static void initialize_compressor(comp_t *comp, uint32_t id);
static void normalize_compressor(comp_t *comp);
static void output_buffer(comp_t *comp, uint8_t carry, uint32_t count);
static void output_symbol(comp_t *comp, uint8_t symbol);

void compress(uint32_t id)
{
	comp_t comp;
	initialize_compressor(&comp, id);
	build_frequency(&comp);
	flush_frequency(&comp, get_scale(&comp));
	for (;;) {
		const int c = __getbyte(id);
		if (c < 0)
			break;
		output_symbol(&comp, (uint8_t)c);
	}
	finalize_compressor(&comp);
}

static void build_frequency(comp_t *comp)
{
	freq_t *freq = comp->frequency;
	for (int i = 0; i < 256; i++, freq++)
		freq->count = 0;
	for (freq = comp->frequency;;) {
		const int c = __getbyte(comp->id);
		if (c < 0)
			break;
		freq[c].count++;
	}
	__rewind(comp->id);
}

static void finalize_compressor(comp_t *comp)
{
	const uint8_t carry = comp->low.hi < 255 || (comp->low.hi == 255 && comp->low.lo < -1) ? -1 : 0;
	if (!carry)
		comp->buffer++;
	output_buffer(comp, carry, comp->counter);
	if (comp->low.hi & 255) {
		__putbyte(comp->id, (uint8_t)comp->low.hi);
		for (int i = 0; i < 4; i++) {
			const uint8_t byte = comp->low.lo >> ((3 - i) * 8);
			if (!byte)
				break;
			__putbyte(comp->id, byte);
		}
	}
	__flush(comp->id);
}

static void flush_frequency(comp_t *comp, int scale)
{
	freq_t *freq = comp->frequency, *prev;
	for (int i = 0; i < 256; i++, freq++) {
		freq->count = (freq->count >> scale | (freq->count ? 1 : 0)) & 65535;
		putfreq(comp->id, (uint8_t)i, (uint16_t)freq->count);
		freq->cumulated = i ? prev->count + prev->cumulated : 0;
		prev = freq;
	}
}

static int get_scale(const comp_t *comp)
{
	const freq_t *freq = comp->frequency;
	unsigned long max = 1;
	for (int i = 0; i < 256; i++, freq++)
		if (max < freq->count)
			max = freq->count;
	int scale = 0;
	for (; 65535 < max; max >>= 1, scale++);
	return scale;
}

static void initialize_compressor(comp_t *comp, uint32_t id)
{
	comp->buffer = -1;
	comp->counter = 0;
	comp->id = id;
	comp->low.lo = 0;
	comp->low.hi = 0;
	comp->range.lo = -1;
	comp->range.hi = (uint8_t)-1;
}

static void normalize_compressor(comp_t *comp)
{
	if (255 < comp->low.hi || (comp->low.hi == 255 && comp->low.lo == -1)) {
		comp->buffer++;
		comp->low.hi &= 255;
		if (comp->counter) {
			output_buffer(comp, 0, comp->counter - 1);
			comp->buffer = 0;
			comp->counter = 0;
		}
	}
	while (comp->range.hi == 0) {
		if (comp->low.hi < 255 || (comp->low.hi == 255 && comp->low.lo == 0)) {
			if (0 <= comp->buffer)
				output_buffer(comp, -1, comp->counter);
			comp->buffer = (uint8_t)comp->low.hi;
			comp->counter = 0;
		} else
			comp->counter++;
		shift_left(&comp->low, 8);
		comp->low.hi &= 255;
		shift_left(&comp->range, 8);
	}
}

static void output_buffer(comp_t *comp, uint8_t carry, uint32_t count)
{
	__putbyte(comp->id, (uint8_t)comp->buffer);
	for (uint32_t i = 0; i < count; i++)
		__putbyte(comp->id, carry);
}

static void output_symbol(comp_t *comp, uint8_t symbol)
{
	const freq_t *const freq = comp->frequency + symbol;
	uint32_t temp = comp->range.value / comp->frequency[255].cumulated;
	comp->low.value += (uint64_t)freq->cumulated * temp;
	comp->range.value = (uint64_t)freq->count * temp;
	normalize_compressor(comp);
}
