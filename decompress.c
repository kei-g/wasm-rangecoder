#include "rangecoder.h"

typedef struct _decompressor decomp_t;

struct _decompressor {
  freq_t frequency[256];
  uint32_t id;
  uint64_u low;
  uint64_u range;
};

static void finalize_decompressor(decomp_t *decomp);
static void initialize_decompressor(decomp_t *decomp, uint32_t id);
static void normalize_decompressor(decomp_t *decomp);
static void read_frequency(decomp_t *decomp);
static void read_initial_low(decomp_t *decomp);
static int read_symbol(decomp_t *decomp);
static int search_symbol(const decomp_t *decomp, uint32_t value);

int decompress(uint32_t id)
{
  decomp_t decomp;
  int success = 1;
  initialize_decompressor(&decomp, id);
  read_frequency(&decomp);
  read_initial_low(&decomp);
  uint32_t size = getdecompressedsize(id);
  for (uint32_t i = 0; i < size; i++) {
    const int symbol = read_symbol(&decomp);
    if (symbol < 0 || 255 < symbol) {
      success = 0;
      break;
    }
    __putbyte(id, (uint8_t)symbol);
  }
  finalize_decompressor(&decomp);
  return success;
}

static void finalize_decompressor(decomp_t *decomp)
{
  __flush(decomp->id);
}

static void initialize_decompressor(decomp_t *decomp, uint32_t id)
{
  decomp->id = id;
  decomp->low.lo = 0;
  decomp->low.hi = 0;
  decomp->range.lo = -1;
  decomp->range.hi = (uint8_t)-1;
}

static void normalize_decompressor(decomp_t *decomp)
{
  while (!decomp->range.hi) {
    const int c = __getbyte(decomp->id);
    shift_left(&decomp->range, 8);
    shift_left(&decomp->low, 8);
    decomp->low.hi &= 255;
    decomp->low.lo |= c < 0 ? 0 : (uint8_t)c;
  }
}

static void read_frequency(decomp_t *decomp)
{
  freq_t *freq = decomp->frequency, *prev;
  for (int i = 0; i < 256; i++, freq++) {
    freq->count = getfreq(decomp->id, (uint8_t)i);
    freq->cumulated = i ? prev->count + prev->cumulated : 0;
    prev = freq;
  }
}

static void read_initial_low(decomp_t *decomp)
{
  for (int i = 0; i < 5; i++) {
    const int c = __getbyte(decomp->id);
    shift_left(&decomp->low, 8);
    decomp->low.lo |= c < 0 ? 0 : (uint8_t)c;
  }
}

static int read_symbol(decomp_t *decomp)
{
  uint32_t temp = decomp->range.value / decomp->frequency[255].cumulated;
  uint32_t value = decomp->low.value / temp;
  if (decomp->frequency[255].cumulated <= value)
    return 256;
  int symbol = search_symbol(decomp, value);
  if (symbol < 0 || 255 < symbol)
    return symbol;
  const freq_t *const freq = &decomp->frequency[symbol];
  if (!freq->count)
    return -1;
  decomp->low.value -= (uint64_t)freq->cumulated * temp;
  decomp->range.value = (uint64_t)freq->count * temp;
  normalize_decompressor(decomp);
  return symbol;
}

static int search_symbol(const decomp_t *decomp, uint32_t value)
{
  int i = 0, j = 255;
  while (i < j) {
    int k = (i + j) / 2;
    if (decomp->frequency[k + 1].cumulated <= value)
      i = k + 1;
    else
      j = k;
  }
  return i;
}
