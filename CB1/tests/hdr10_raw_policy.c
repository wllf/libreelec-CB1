/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "dvbridge_placebo.h"
#include "task3_fixture.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    size_t bytes;
    AVDOVIMetadata *m = task3_fixture(&bytes);
    m->num_ext_blocks = 1;
    AVDOVIDmData *l6 = av_dovi_get_ext(m, 0);
    l6->level = 6; l6->l6.max_luminance = 4000; l6->l6.min_luminance = 50;
    l6->l6.max_cll = 3500; l6->l6.max_fall = 400;
    struct dvbridge_hdr10_session basic, raw;
    assert(dvbridge_hdr10_session_init(&basic, m, bytes) && !basic.raw);
    assert(dvbridge_hdr10_raw_session_init(&raw, m, bytes) && raw.raw);
    /* Same static target peak as Basic, but source light levels survive. */
    assert(raw.target.hdr.max_luma == 4000.0f && raw.output.max_luminance == 4000);
    assert(raw.output.level6 && raw.output.max_cll == 3500 && raw.output.max_fall == 400);
    assert(raw.output.min_luminance == 50 && fabsf(raw.target.hdr.min_luma - 0.005f) < 1e-6f);
    assert(raw.target.primaries == PL_COLOR_PRIM_BT_2020 && raw.target.transfer == PL_COLOR_TRC_PQ);
    /* Inconsistent light levels are reported unknown, not invented. */
    l6->l6.max_fall = 3600;
    assert(dvbridge_hdr10_raw_session_init(&raw, m, bytes));
    assert(!raw.output.max_cll && !raw.output.max_fall);
    /* Without L6, the RPU source range supplies the mastering envelope. */
    m->num_ext_blocks = 0;
    av_dovi_get_color(m)->source_max_pq = 3079;
    av_dovi_get_color(m)->source_min_pq = 62;
    assert(dvbridge_hdr10_raw_session_init(&raw, m, bytes) && !raw.output.level6);
    assert(fabsf(raw.target.hdr.max_luma - 1000) < 2 && !raw.output.max_cll);
    assert(raw.output.min_luminance > 0 && raw.target.hdr.min_luma < 0.01f);
    /* Failure is atomic, exactly as for Basic. */
    av_dovi_get_color(m)->source_max_pq = 0;
    struct dvbridge_hdr10_session before = raw;
    assert(!dvbridge_hdr10_raw_session_init(&raw, m, bytes));
    assert(!memcmp(&before, &raw, sizeof(raw)));
    assert(!dvbridge_hdr10_raw_session_init(NULL, m, bytes));
    av_free(m);
    puts("PASS: raw HDR10 session keeps source L6 light levels and mastering range");
}
