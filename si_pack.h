#ifndef SI_PACK_H
#define SI_PACK_H

#ifndef SIP_FUNC_DEF
#   if defined(SIP_STATIC)
#      define SIP_FUNC_DEF static
#   elif defined(_WIN32) && defined(SIP_DLL) && defined(SI_PACK_IMPL)
#       define SIP_FUNC_DEF __declspec(dllexport)
#   elif defined(_WIN32) && defined(SIP_DLL)
#       define SIP_FUNC_DEF __declspec(dllimport)
#   else
#      define SIP_FUNC_DEF extern
#   endif
#endif

#ifndef SI_ARENA_H
#   error "si_arena.h must be included before si_pack.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SIP_TRUE  1
#define SIP_FALSE 0

typedef sia_i32 sip_b32;

typedef struct {
    sia_u32 id;
    sia_u32 w, h;
} sip_rect_input;

typedef struct {
    sia_u32 id;
    sia_u32 x, y;
    sia_u32 w, h;
    sip_b32 rotated;
} sip_rect_result;

typedef struct {
    sia_u32 atlas_w, atlas_h;
    sia_u32 num_rects;
    sip_rect_result* rects;
    float occupancy;
} sip_pack_result;

typedef enum {
    SIP_ALGO_SHELF,
    SIP_ALGO_GUILLOTINE,
    SIP_ALGO_SKYLINE,
    SIP_ALGO_AUTO
} sip_algorithm;

typedef struct {
    si_arena* arena;
    sip_algorithm algorithm;
    sia_u32 max_width;
    sia_u32 max_height;
    sia_u32 padding;
    sip_b32 allow_rotate;
    sip_b32 power_of_two;
} sip_desc;

SIP_FUNC_DEF sip_pack_result sip_pack(const sip_desc* desc, const sip_rect_input* rects, sia_u32 num_rects);

SIP_FUNC_DEF const sip_rect_result* sip_find(const sip_pack_result* result, sia_u32 id);

SIP_FUNC_DEF sip_b32 sip_pack_append(const sip_desc* desc, sip_pack_result* result,
                                      const sip_rect_input* rects, sia_u32 num_rects);

SIP_FUNC_DEF void sip_sort_by_area(sip_rect_input* rects, sia_u32 num_rects);

#ifdef __cplusplus
}
#endif

#endif /* SI_PACK_H */


#ifdef SI_PACK_IMPL

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SIP_MEMCPY
#   include <string.h>
#   define SIP_MEMCPY memcpy
#endif

#ifndef SIP_MEMSET
#   include <string.h>
#   define SIP_MEMSET memset
#endif

#define _SIP_MAX(a, b) ((a) > (b) ? (a) : (b))
#define _SIP_MIN(a, b) ((a) < (b) ? (a) : (b))

/* =========================================================================
   Internal: power-of-two rounding
   ========================================================================= */

static sia_u32 _sip_round_pow2(sia_u32 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

/* =========================================================================
   Internal: merge sort (arena scratch for temp buffer)
   ========================================================================= */

static void _sip_merge(sip_rect_input* arr, sip_rect_input* tmp,
                        sia_u32 left, sia_u32 mid, sia_u32 right) {
    sia_u32 i = left, j = mid, k = left;

    while (i < mid && j < right) {
        sia_u64 area_i = (sia_u64)arr[i].w * arr[i].h;
        sia_u64 area_j = (sia_u64)arr[j].w * arr[j].h;
        if (area_i >= area_j) {
            tmp[k++] = arr[i++];
        } else {
            tmp[k++] = arr[j++];
        }
    }
    while (i < mid)   { tmp[k++] = arr[i++]; }
    while (j < right)  { tmp[k++] = arr[j++]; }

    for (sia_u32 n = left; n < right; n++) {
        arr[n] = tmp[n];
    }
}

static void _sip_merge_sort_impl(sip_rect_input* arr, sip_rect_input* tmp,
                                  sia_u32 left, sia_u32 right) {
    if (right - left <= 1) return;
    sia_u32 mid = left + (right - left) / 2;
    _sip_merge_sort_impl(arr, tmp, left, mid);
    _sip_merge_sort_impl(arr, tmp, mid, right);
    _sip_merge(arr, tmp, left, mid, right);
}

static void _sip_sort_by_area_with_arena(si_arena* arena, sip_rect_input* rects, sia_u32 num_rects) {
    if (num_rects <= 1) return;

    sia_temp scratch = sia_scratch_get(&arena, 1);
    sip_rect_input* tmp = SIA_PUSH_ARRAY(scratch.arena, sip_rect_input, num_rects);
    _sip_merge_sort_impl(rects, tmp, 0, num_rects);
    sia_scratch_release(scratch);
}

/* Sort by height descending (used by shelf algorithm) */
static void _sip_merge_height(sip_rect_input* arr, sip_rect_input* tmp,
                               sia_u32 left, sia_u32 mid, sia_u32 right) {
    sia_u32 i = left, j = mid, k = left;

    while (i < mid && j < right) {
        if (arr[i].h >= arr[j].h) {
            tmp[k++] = arr[i++];
        } else {
            tmp[k++] = arr[j++];
        }
    }
    while (i < mid)   { tmp[k++] = arr[i++]; }
    while (j < right)  { tmp[k++] = arr[j++]; }

    for (sia_u32 n = left; n < right; n++) {
        arr[n] = tmp[n];
    }
}

static void _sip_sort_height_impl(sip_rect_input* arr, sip_rect_input* tmp,
                                   sia_u32 left, sia_u32 right) {
    if (right - left <= 1) return;
    sia_u32 mid = left + (right - left) / 2;
    _sip_sort_height_impl(arr, tmp, left, mid);
    _sip_sort_height_impl(arr, tmp, mid, right);
    _sip_merge_height(arr, tmp, left, mid, right);
}

static void _sip_sort_by_height(si_arena* arena, sip_rect_input* rects, sia_u32 num_rects) {
    if (num_rects <= 1) return;

    sia_temp scratch = sia_scratch_get(&arena, 1);
    sip_rect_input* tmp = SIA_PUSH_ARRAY(scratch.arena, sip_rect_input, num_rects);
    _sip_sort_height_impl(rects, tmp, 0, num_rects);
    sia_scratch_release(scratch);
}

/* =========================================================================
   Internal: copy input array into arena (so we can sort without mutating caller's data)
   ========================================================================= */

static sip_rect_input* _sip_copy_inputs(si_arena* arena, const sip_rect_input* rects, sia_u32 num_rects) {
    sip_rect_input* copy = SIA_PUSH_ARRAY(arena, sip_rect_input, num_rects);
    SIP_MEMCPY(copy, rects, sizeof(sip_rect_input) * num_rects);
    return copy;
}

/* =========================================================================
   Internal: compute atlas bounds from placed rects, apply padding and pow2
   ========================================================================= */

static void _sip_finalize_result(sip_pack_result* result, const sip_desc* desc) {
    sia_u32 max_x = 0, max_y = 0;
    sia_u64 filled_area = 0;

    for (sia_u32 i = 0; i < result->num_rects; i++) {
        sip_rect_result* r = &result->rects[i];
        sia_u32 rx = r->x + r->w;
        sia_u32 ry = r->y + r->h;
        if (rx > max_x) max_x = rx;
        if (ry > max_y) max_y = ry;
        filled_area += (sia_u64)r->w * r->h;
    }

    result->atlas_w = max_x;
    result->atlas_h = max_y;

    if (desc->power_of_two) {
        if (result->atlas_w > 0) result->atlas_w = _sip_round_pow2(result->atlas_w);
        if (result->atlas_h > 0) result->atlas_h = _sip_round_pow2(result->atlas_h);
    }

    sia_u64 atlas_area = (sia_u64)result->atlas_w * result->atlas_h;
    result->occupancy = atlas_area > 0 ? (float)filled_area / (float)atlas_area : 0.0f;
}

/* =========================================================================
   Internal: effective dimensions with padding
   ========================================================================= */

static void _sip_padded_dims(const sip_rect_input* r, sia_u32 padding,
                              sia_u32* out_w, sia_u32* out_h) {
    *out_w = r->w + padding;
    *out_h = r->h + padding;
}

/* =========================================================================
   Shelf Algorithm
   ========================================================================= */

typedef struct {
    sia_u32 y;
    sia_u32 height;
    sia_u32 x_cursor;
} _sip_shelf;

static sip_pack_result _sip_pack_shelf(si_arena* arena, const sip_desc* desc,
                                        const sip_rect_input* rects, sia_u32 num_rects) {
    sip_pack_result result = { 0 };
    result.num_rects = num_rects;
    result.rects = SIA_PUSH_ZERO_ARRAY(arena, sip_rect_result, num_rects);

    sip_rect_input* sorted = _sip_copy_inputs(arena, rects, num_rects);
    _sip_sort_by_height(arena, sorted, num_rects);

    sia_u32 atlas_w = desc->max_width > 0 ? desc->max_width : 4096;
    sia_u32 padding = desc->padding;

    sia_u32 shelf_cap = 64;
    _sip_shelf* shelves = SIA_PUSH_ZERO_ARRAY(arena, _sip_shelf, shelf_cap);
    sia_u32 num_shelves = 0;

    for (sia_u32 i = 0; i < num_rects; i++) {
        sia_u32 pw, ph;
        _sip_padded_dims(&sorted[i], padding, &pw, &ph);

        sia_u32 rw = sorted[i].w;
        sia_u32 rh = sorted[i].h;
        sip_b32 rotated = SIP_FALSE;

        if (desc->allow_rotate && pw > ph) {
            sia_u32 tmp = pw; pw = ph; ph = tmp;
            tmp = rw; rw = rh; rh = tmp;
            rotated = SIP_TRUE;
        }

        sip_b32 placed = SIP_FALSE;

        for (sia_u32 s = 0; s < num_shelves; s++) {
            if (shelves[s].x_cursor + pw <= atlas_w && ph <= shelves[s].height) {
                result.rects[i].id = sorted[i].id;
                result.rects[i].x = shelves[s].x_cursor;
                result.rects[i].y = shelves[s].y;
                result.rects[i].w = rw;
                result.rects[i].h = rh;
                result.rects[i].rotated = rotated;
                shelves[s].x_cursor += pw;
                placed = SIP_TRUE;
                break;
            }
        }

        if (!placed) {
            sia_u32 shelf_y = 0;
            if (num_shelves > 0) {
                _sip_shelf* last = &shelves[num_shelves - 1];
                shelf_y = last->y + last->height;
            }

            if (desc->max_height > 0 && shelf_y + ph > desc->max_height) {
                result.num_rects = 0;
                return result;
            }

            if (num_shelves >= shelf_cap) {
                sia_u32 new_cap = shelf_cap * 2;
                _sip_shelf* new_shelves = SIA_PUSH_ZERO_ARRAY(arena, _sip_shelf, new_cap);
                SIP_MEMCPY(new_shelves, shelves, sizeof(_sip_shelf) * num_shelves);
                shelves = new_shelves;
                shelf_cap = new_cap;
            }

            shelves[num_shelves].y = shelf_y;
            shelves[num_shelves].height = ph;
            shelves[num_shelves].x_cursor = pw;

            result.rects[i].id = sorted[i].id;
            result.rects[i].x = 0;
            result.rects[i].y = shelf_y;
            result.rects[i].w = rw;
            result.rects[i].h = rh;
            result.rects[i].rotated = rotated;

            num_shelves++;
        }
    }

    _sip_finalize_result(&result, desc);
    return result;
}

/* =========================================================================
   Guillotine Algorithm
   ========================================================================= */

typedef struct {
    sia_u32 x, y, w, h;
} _sip_free_rect;

typedef struct {
    _sip_free_rect* data;
    sia_u32 count;
    sia_u32 cap;
    si_arena* arena;
} _sip_free_list;

static _sip_free_list _sip_free_list_create(si_arena* arena, sia_u32 initial_cap) {
    _sip_free_list fl;
    fl.arena = arena;
    fl.cap = initial_cap;
    fl.count = 0;
    fl.data = SIA_PUSH_ZERO_ARRAY(arena, _sip_free_rect, initial_cap);
    return fl;
}

static void _sip_free_list_push(_sip_free_list* fl, _sip_free_rect r) {
    if (fl->count >= fl->cap) {
        sia_u32 new_cap = fl->cap * 2;
        _sip_free_rect* new_data = SIA_PUSH_ZERO_ARRAY(fl->arena, _sip_free_rect, new_cap);
        SIP_MEMCPY(new_data, fl->data, sizeof(_sip_free_rect) * fl->count);
        fl->data = new_data;
        fl->cap = new_cap;
    }
    fl->data[fl->count++] = r;
}

static void _sip_free_list_remove(_sip_free_list* fl, sia_u32 index) {
    fl->count--;
    if (index < fl->count) {
        fl->data[index] = fl->data[fl->count];
    }
}

static sip_pack_result _sip_pack_guillotine(si_arena* arena, const sip_desc* desc,
                                             const sip_rect_input* rects, sia_u32 num_rects) {
    sip_pack_result result = { 0 };
    result.num_rects = num_rects;
    result.rects = SIA_PUSH_ZERO_ARRAY(arena, sip_rect_result, num_rects);

    sip_rect_input* sorted = _sip_copy_inputs(arena, rects, num_rects);
    _sip_sort_by_area_with_arena(arena, sorted, num_rects);

    sia_u32 atlas_w = desc->max_width  > 0 ? desc->max_width  : 4096;
    sia_u32 atlas_h = desc->max_height > 0 ? desc->max_height : 4096;
    sia_u32 padding = desc->padding;

    _sip_free_list fl = _sip_free_list_create(arena, num_rects + 16);
    _sip_free_list_push(&fl, (_sip_free_rect){ 0, 0, atlas_w, atlas_h });

    for (sia_u32 i = 0; i < num_rects; i++) {
        sia_u32 pw, ph;
        _sip_padded_dims(&sorted[i], padding, &pw, &ph);

        sia_u32 rw = sorted[i].w;
        sia_u32 rh = sorted[i].h;

        sia_u32 best_idx = (sia_u32)-1;
        sia_u64 best_fit = (sia_u64)-1;
        sip_b32 best_rotated = SIP_FALSE;

        for (sia_u32 f = 0; f < fl.count; f++) {
            _sip_free_rect* fr = &fl.data[f];

            if (pw <= fr->w && ph <= fr->h) {
                sia_u64 leftover = (sia_u64)(fr->w - pw) * fr->h + (sia_u64)fr->w * (fr->h - ph);
                if (leftover < best_fit) {
                    best_fit = leftover;
                    best_idx = f;
                    best_rotated = SIP_FALSE;
                }
            }

            if (desc->allow_rotate && ph <= fr->w && pw <= fr->h) {
                sia_u64 leftover = (sia_u64)(fr->w - ph) * fr->h + (sia_u64)fr->w * (fr->h - pw);
                if (leftover < best_fit) {
                    best_fit = leftover;
                    best_idx = f;
                    best_rotated = SIP_TRUE;
                }
            }
        }

        if (best_idx == (sia_u32)-1) {
            result.num_rects = 0;
            return result;
        }

        if (best_rotated) {
            sia_u32 tmp = pw; pw = ph; ph = tmp;
            tmp = rw; rw = rh; rh = tmp;
        }

        _sip_free_rect chosen = fl.data[best_idx];
        _sip_free_list_remove(&fl, best_idx);

        result.rects[i].id = sorted[i].id;
        result.rects[i].x = chosen.x;
        result.rects[i].y = chosen.y;
        result.rects[i].w = rw;
        result.rects[i].h = rh;
        result.rects[i].rotated = best_rotated;

        sia_u32 rem_w = chosen.w - pw;
        sia_u32 rem_h = chosen.h - ph;

        /* Split along the axis that produces the larger remaining rect */
        if (rem_w > rem_h) {
            if (rem_w > 0)
                _sip_free_list_push(&fl, (_sip_free_rect){ chosen.x + pw, chosen.y, rem_w, chosen.h });
            if (rem_h > 0)
                _sip_free_list_push(&fl, (_sip_free_rect){ chosen.x, chosen.y + ph, pw, rem_h });
        } else {
            if (rem_h > 0)
                _sip_free_list_push(&fl, (_sip_free_rect){ chosen.x, chosen.y + ph, chosen.w, rem_h });
            if (rem_w > 0)
                _sip_free_list_push(&fl, (_sip_free_rect){ chosen.x + pw, chosen.y, rem_w, ph });
        }
    }

    _sip_finalize_result(&result, desc);
    return result;
}

/* =========================================================================
   Skyline Algorithm
   ========================================================================= */

typedef struct {
    sia_u32 x, y, w;
} _sip_skyline_node;

typedef struct {
    _sip_skyline_node* nodes;
    sia_u32 count;
    sia_u32 cap;
    si_arena* arena;
} _sip_skyline;

static _sip_skyline _sip_skyline_create(si_arena* arena, sia_u32 width, sia_u32 initial_cap) {
    _sip_skyline sl;
    sl.arena = arena;
    sl.cap = initial_cap;
    sl.count = 1;
    sl.nodes = SIA_PUSH_ZERO_ARRAY(arena, _sip_skyline_node, initial_cap);
    sl.nodes[0].x = 0;
    sl.nodes[0].y = 0;
    sl.nodes[0].w = width;
    return sl;
}

static void _sip_skyline_insert(_sip_skyline* sl, sia_u32 index, _sip_skyline_node node) {
    if (sl->count >= sl->cap) {
        sia_u32 new_cap = sl->cap * 2;
        _sip_skyline_node* new_nodes = SIA_PUSH_ZERO_ARRAY(sl->arena, _sip_skyline_node, new_cap);
        SIP_MEMCPY(new_nodes, sl->nodes, sizeof(_sip_skyline_node) * sl->count);
        sl->nodes = new_nodes;
        sl->cap = new_cap;
    }

    for (sia_u32 i = sl->count; i > index; i--) {
        sl->nodes[i] = sl->nodes[i - 1];
    }
    sl->nodes[index] = node;
    sl->count++;
}

static void _sip_skyline_remove(_sip_skyline* sl, sia_u32 index) {
    sl->count--;
    for (sia_u32 i = index; i < sl->count; i++) {
        sl->nodes[i] = sl->nodes[i + 1];
    }
}

/* Find the y position where a rect of width pw can sit starting at skyline node `index`.
   Returns the maximum y across all skyline segments the rect would span. */
static sip_b32 _sip_skyline_fit(_sip_skyline* sl, sia_u32 index, sia_u32 pw, sia_u32 ph,
                                 sia_u32 max_w, sia_u32 max_h, sia_u32* out_y) {
    sia_u32 x = sl->nodes[index].x;
    if (x + pw > max_w) return SIP_FALSE;

    sia_u32 width_left = pw;
    sia_u32 y = sl->nodes[index].y;
    sia_u32 i = index;

    while (width_left > 0 && i < sl->count) {
        if (sl->nodes[i].y > y) y = sl->nodes[i].y;
        if (y + ph > max_h) return SIP_FALSE;

        sia_u32 consumed = _SIP_MIN(sl->nodes[i].w, width_left);
        width_left -= consumed;
        i++;
    }

    if (width_left > 0) return SIP_FALSE;

    *out_y = y;
    return SIP_TRUE;
}

static void _sip_skyline_add_rect(_sip_skyline* sl, sia_u32 x, sia_u32 y, sia_u32 pw, sia_u32 ph) {
    _sip_skyline_node new_node;
    new_node.x = x;
    new_node.y = y + ph;
    new_node.w = pw;

    /* Find where to insert and which nodes get consumed */
    sia_u32 insert_idx = 0;
    for (sia_u32 i = 0; i < sl->count; i++) {
        if (sl->nodes[i].x <= x && sl->nodes[i].x + sl->nodes[i].w > x) {
            insert_idx = i;
            break;
        }
    }

    _sip_skyline_insert(sl, insert_idx, new_node);

    /* Trim or remove nodes that the new rect covers */
    for (sia_u32 i = insert_idx + 1; i < sl->count; ) {
        _sip_skyline_node* prev = &sl->nodes[i - 1];
        _sip_skyline_node* cur  = &sl->nodes[i];

        sia_u32 prev_end = prev->x + prev->w;
        if (cur->x < prev_end) {
            sia_u32 shrink = prev_end - cur->x;
            if (shrink >= cur->w) {
                _sip_skyline_remove(sl, i);
            } else {
                cur->x += shrink;
                cur->w -= shrink;
                break;
            }
        } else {
            break;
        }
    }

    /* Merge adjacent nodes at the same height */
    for (sia_u32 i = 0; i + 1 < sl->count; ) {
        if (sl->nodes[i].y == sl->nodes[i + 1].y) {
            sl->nodes[i].w += sl->nodes[i + 1].w;
            _sip_skyline_remove(sl, i + 1);
        } else {
            i++;
        }
    }
}

static sip_pack_result _sip_pack_skyline(si_arena* arena, const sip_desc* desc,
                                          const sip_rect_input* rects, sia_u32 num_rects) {
    sip_pack_result result = { 0 };
    result.num_rects = num_rects;
    result.rects = SIA_PUSH_ZERO_ARRAY(arena, sip_rect_result, num_rects);

    sip_rect_input* sorted = _sip_copy_inputs(arena, rects, num_rects);
    _sip_sort_by_area_with_arena(arena, sorted, num_rects);

    sia_u32 atlas_w = desc->max_width  > 0 ? desc->max_width  : 4096;
    sia_u32 atlas_h = desc->max_height > 0 ? desc->max_height : 4096;
    sia_u32 padding = desc->padding;

    _sip_skyline sl = _sip_skyline_create(arena, atlas_w, num_rects + 16);

    for (sia_u32 i = 0; i < num_rects; i++) {
        sia_u32 pw, ph;
        _sip_padded_dims(&sorted[i], padding, &pw, &ph);

        sia_u32 rw = sorted[i].w;
        sia_u32 rh = sorted[i].h;

        sia_u32 best_y = (sia_u32)-1;
        sia_u32 best_x = 0;
        sia_u32 best_idx = (sia_u32)-1;
        sip_b32 best_rotated = SIP_FALSE;

        for (sia_u32 s = 0; s < sl.count; s++) {
            sia_u32 y;
            if (_sip_skyline_fit(&sl, s, pw, ph, atlas_w, atlas_h, &y)) {
                if (y < best_y || (y == best_y && sl.nodes[s].x < best_x)) {
                    best_y = y;
                    best_x = sl.nodes[s].x;
                    best_idx = s;
                    best_rotated = SIP_FALSE;
                }
            }

            if (desc->allow_rotate && pw != ph) {
                if (_sip_skyline_fit(&sl, s, ph, pw, atlas_w, atlas_h, &y)) {
                    if (y < best_y || (y == best_y && sl.nodes[s].x < best_x)) {
                        best_y = y;
                        best_x = sl.nodes[s].x;
                        best_idx = s;
                        best_rotated = SIP_TRUE;
                    }
                }
            }
        }

        if (best_idx == (sia_u32)-1) {
            result.num_rects = 0;
            return result;
        }

        if (best_rotated) {
            sia_u32 tmp = pw; pw = ph; ph = tmp;
            tmp = rw; rw = rh; rh = tmp;
        }

        result.rects[i].id = sorted[i].id;
        result.rects[i].x = best_x;
        result.rects[i].y = best_y;
        result.rects[i].w = rw;
        result.rects[i].h = rh;
        result.rects[i].rotated = best_rotated;

        _sip_skyline_add_rect(&sl, best_x, best_y, pw, ph);
    }

    _sip_finalize_result(&result, desc);
    return result;
}

/* =========================================================================
   Public API
   ========================================================================= */

sip_pack_result sip_pack(const sip_desc* desc, const sip_rect_input* rects, sia_u32 num_rects) {
    sip_pack_result result = { 0 };

    if (desc == NULL || desc->arena == NULL || rects == NULL || num_rects == 0) {
        return result;
    }

    si_arena* arena = desc->arena;

    if (desc->algorithm == SIP_ALGO_AUTO) {
        typedef sip_pack_result (*_sip_algo_fn)(si_arena*, const sip_desc*, const sip_rect_input*, sia_u32);
        _sip_algo_fn algos[3] = { _sip_pack_shelf, _sip_pack_guillotine, _sip_pack_skyline };

        float best_occupancy = -1.0f;
        int winner = 0;

        for (int a = 0; a < 3; a++) {
            sia_temp temp = sia_temp_begin(arena);
            sip_pack_result trial = algos[a](arena, desc, rects, num_rects);

            if (trial.num_rects > 0 && trial.occupancy > best_occupancy) {
                best_occupancy = trial.occupancy;
                winner = a;
            }

            sia_temp_end(temp);
        }

        return algos[winner](arena, desc, rects, num_rects);
    }

    switch (desc->algorithm) {
        case SIP_ALGO_SHELF:
            result = _sip_pack_shelf(arena, desc, rects, num_rects);
            break;
        case SIP_ALGO_GUILLOTINE:
            result = _sip_pack_guillotine(arena, desc, rects, num_rects);
            break;
        case SIP_ALGO_SKYLINE:
            result = _sip_pack_skyline(arena, desc, rects, num_rects);
            break;
        default:
            break;
    }

    return result;
}

const sip_rect_result* sip_find(const sip_pack_result* result, sia_u32 id) {
    if (result == NULL || result->rects == NULL) return NULL;

    for (sia_u32 i = 0; i < result->num_rects; i++) {
        if (result->rects[i].id == id) {
            return &result->rects[i];
        }
    }
    return NULL;
}

sip_b32 sip_pack_append(const sip_desc* desc, sip_pack_result* result,
                         const sip_rect_input* rects, sia_u32 num_rects) {
    if (desc == NULL || desc->arena == NULL || result == NULL ||
        rects == NULL || num_rects == 0) {
        return SIP_FALSE;
    }

    si_arena* arena = desc->arena;

    /* Build a combined input array: existing results + new rects */
    sia_u32 total = result->num_rects + num_rects;
    sip_rect_input* combined = SIA_PUSH_ARRAY(arena, sip_rect_input, total);

    for (sia_u32 i = 0; i < result->num_rects; i++) {
        combined[i].id = result->rects[i].id;
        combined[i].w  = result->rects[i].rotated ? result->rects[i].h : result->rects[i].w;
        combined[i].h  = result->rects[i].rotated ? result->rects[i].w : result->rects[i].h;
    }
    SIP_MEMCPY(&combined[result->num_rects], rects, sizeof(sip_rect_input) * num_rects);

    sip_pack_result new_result = sip_pack(desc, combined, total);

    if (new_result.num_rects == 0) {
        return SIP_FALSE;
    }

    *result = new_result;
    return SIP_TRUE;
}

void sip_sort_by_area(sip_rect_input* rects, sia_u32 num_rects) {
    /* Simple insertion sort for the public API (no arena needed) */
    for (sia_u32 i = 1; i < num_rects; i++) {
        sip_rect_input key = rects[i];
        sia_u64 key_area = (sia_u64)key.w * key.h;
        sia_u32 j = i;
        while (j > 0) {
            sia_u64 prev_area = (sia_u64)rects[j - 1].w * rects[j - 1].h;
            if (prev_area >= key_area) break;
            rects[j] = rects[j - 1];
            j--;
        }
        rects[j] = key;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* SI_PACK_IMPL */

/*
License
=================================
  _    ___ ___ ___ _  _ ___ ___ 
 | |  |_ _/ __| __| \| / __| __|
 | |__ | | (__| _|| .` \__ \ _| 
 |____|___\___|___|_|\_|___/___|
                                
=================================

MIT License

Copyright (c) 2025

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
