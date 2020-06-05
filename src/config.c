#include "../include/r3s.h"
#include "util.h"
#include "hash.h"
#include "printer.h"
#include "packet.h"
#include "config.h"

#include <stdlib.h>

#define MAX(x,y) ((x) >= (y) ? (x) : (y))


void exitf(const char *message)
{
    fprintf(stderr, "BUG: %s.\n", message);
    exit(0);
}

void error_handler(Z3_context c, Z3_error_code e)
{
    printf("Error code: %d\n", e);
    exitf("incorrect use of Z3");
}

Z3_context mk_context_custom(Z3_config cfg, Z3_error_handler err)
{
    Z3_context ctx;

    Z3_set_param_value(cfg, "model", "true");

    #if DEBUG
        Z3_set_param_value(cfg, "unsat_core", "true");
    #endif

    ctx = Z3_mk_context(cfg);

    Z3_set_error_handler(ctx, err);

    return ctx;
}

Z3_context mk_context()
{
    Z3_config  cfg;
    Z3_context ctx;

    cfg = Z3_mk_config();
    Z3_set_param_value(cfg, "MODEL", "true");

    ctx = mk_context_custom(cfg, error_handler);
    Z3_del_config(cfg);
    
    return ctx;
}

void R3S_cfg_init(R3S_cfg_t *cfg)
{
    cfg->loaded_opts   = NULL;
    cfg->n_loaded_opts = 0;
    cfg->n_procs       = 0;
    cfg->n_keys        = 1;
    cfg->ctx           = NULL;

    cfg->key_fit_params.pcap_fname        = NULL;
    cfg->key_fit_params.std_dev_threshold = -1;
    cfg->key_fit_params.time_limit        = -1;
    cfg->key_fit_params.n_cores           = 0;

    R3S_cfg_reset_ctx(cfg);
}

void cfg_del_ctx(R3S_cfg_t *cfg) {
    if (cfg->ctx != NULL) Z3_del_context(cfg->ctx);
    cfg->ctx = NULL;
}

void R3S_cfg_reset_ctx(R3S_cfg_t *cfg) {
    cfg_del_ctx(cfg);
    cfg->ctx = mk_context();
}

void R3S_cfg_reset(R3S_cfg_t *cfg)
{
    R3S_cfg_delete(cfg);
    R3S_cfg_init(cfg);
}

void R3S_cfg_delete(R3S_cfg_t *cfg)
{
    cfg_del_ctx(cfg);

    free(cfg->loaded_opts);

    if (cfg->key_fit_params.pcap_fname != NULL)
        free(cfg->key_fit_params.pcap_fname);
}

bool is_valid_opt(R3S_opt_t opt)
{
    switch (opt)
    {
        case R3S_OPT_GENEVE_OAM:
        case R3S_OPT_VXLAN_GPE_OAM:
        case R3S_OPT_NON_FRAG_IPV4_TCP:
        case R3S_OPT_NON_FRAG_IPV4_UDP:
        case R3S_OPT_NON_FRAG_IPV4_SCTP:
        case R3S_OPT_NON_FRAG_IPV4:
        case R3S_OPT_FRAG_IPV4:
        case R3S_OPT_NON_FRAG_IPV6_TCP:
        case R3S_OPT_NON_FRAG_IPV6_UDP:
        case R3S_OPT_NON_FRAG_IPV6_SCTP:
        case R3S_OPT_NON_FRAG_IPV6:
        case R3S_OPT_FRAG_IPV6:
        case R3S_OPT_ETHERTYPE: return true;
    }

    return false;
}

R3S_status_t R3S_opt_to_pfs(R3S_opt_t opt, R3S_pf_t **pfs, unsigned *n_pfs)
{
    // TODO: check if is valid opt
    *n_pfs = 0;

    switch (opt)
    {
        case R3S_OPT_GENEVE_OAM:
        case R3S_OPT_VXLAN_GPE_OAM:
            *n_pfs    = 2;
            *pfs      = (R3S_pf_t*) malloc(sizeof(R3S_pf_t) * (*n_pfs));

            (*pfs)[0] = R3S_PF_VXLAN_UDP_OUTER;
            (*pfs)[1] = R3S_PF_VXLAN_VNI;

            break;
        case R3S_OPT_NON_FRAG_IPV4_UDP:
            *n_pfs    = 4;
            *pfs      = (R3S_pf_t*) malloc(sizeof(R3S_pf_t) * (*n_pfs));

            (*pfs)[0] = R3S_PF_IPV4_SRC;
            (*pfs)[1] = R3S_PF_IPV4_DST;
            (*pfs)[2] = R3S_PF_UDP_SRC;
            (*pfs)[3] = R3S_PF_UDP_DST;

            break;
        case R3S_OPT_NON_FRAG_IPV4_TCP:
            *n_pfs    = 4;
            *pfs      = (R3S_pf_t*) malloc(sizeof(R3S_pf_t) * (*n_pfs));

            (*pfs)[0] = R3S_PF_IPV4_SRC;
            (*pfs)[1] = R3S_PF_IPV4_DST;
            (*pfs)[2] = R3S_PF_TCP_SRC;
            (*pfs)[3] = R3S_PF_TCP_DST;

            break;
        case R3S_OPT_NON_FRAG_IPV4_SCTP:
            *n_pfs    = 5;
            *pfs      = (R3S_pf_t*) malloc(sizeof(R3S_pf_t) * (*n_pfs));

            (*pfs)[0] = R3S_PF_IPV4_SRC;
            (*pfs)[1] = R3S_PF_IPV4_DST;
            (*pfs)[2] = R3S_PF_SCTP_SRC;
            (*pfs)[3] = R3S_PF_SCTP_DST;
            (*pfs)[3] = R3S_PF_SCTP_V_TAG;

            break;
        case R3S_OPT_NON_FRAG_IPV4:
        case R3S_OPT_FRAG_IPV4:
            *n_pfs    = 2;
            *pfs      = (R3S_pf_t*) malloc(sizeof(R3S_pf_t) * (*n_pfs));

            (*pfs)[0] = R3S_PF_IPV4_SRC;
            (*pfs)[1] = R3S_PF_IPV4_DST;

            break;
        case R3S_OPT_NON_FRAG_IPV6_UDP:
            *n_pfs    = 4;
            *pfs      = (R3S_pf_t*) malloc(sizeof(R3S_pf_t) * (*n_pfs));

            (*pfs)[0] = R3S_PF_IPV6_SRC;
            (*pfs)[1] = R3S_PF_IPV6_DST;
            (*pfs)[2] = R3S_PF_UDP_SRC;
            (*pfs)[3] = R3S_PF_UDP_DST;

            break;
        case R3S_OPT_NON_FRAG_IPV6_TCP:
            *n_pfs    = 4;
            *pfs      = (R3S_pf_t*) malloc(sizeof(R3S_pf_t) * (*n_pfs));

            (*pfs)[0] = R3S_PF_IPV6_SRC;
            (*pfs)[1] = R3S_PF_IPV6_DST;
            (*pfs)[2] = R3S_PF_TCP_SRC;
            (*pfs)[3] = R3S_PF_TCP_DST;

            break;
        case R3S_OPT_NON_FRAG_IPV6_SCTP:
            *n_pfs    = 5;
            *pfs      = (R3S_pf_t*) malloc(sizeof(R3S_pf_t) * (*n_pfs));

            (*pfs)[0] = R3S_PF_IPV6_SRC;
            (*pfs)[1] = R3S_PF_IPV6_DST;
            (*pfs)[2] = R3S_PF_SCTP_SRC;
            (*pfs)[3] = R3S_PF_SCTP_DST;
            (*pfs)[3] = R3S_PF_SCTP_V_TAG;

            break;
        case R3S_OPT_NON_FRAG_IPV6:
        case R3S_OPT_FRAG_IPV6:
            *n_pfs    = 2;
            *pfs      = (R3S_pf_t*) malloc(sizeof(R3S_pf_t) * (*n_pfs));

            (*pfs)[0] = R3S_PF_IPV6_SRC;
            (*pfs)[1] = R3S_PF_IPV6_DST;
            
            break;
        case R3S_OPT_ETHERTYPE:
            *n_pfs    = 1;
            *pfs      = (R3S_pf_t*) malloc(sizeof(R3S_pf_t) * (*n_pfs));

            (*pfs)[0] = R3S_PF_ETHERTYPE;
    }

    return R3S_STATUS_SUCCESS;
}

bool R3S_cfg_are_compatible_pfs(R3S_cfg_t cfg, R3S_in_cfg_t pfs)
{
    for (unsigned iopt = 0; iopt < cfg.n_loaded_opts; iopt++) {
        if ((pfs & cfg.loaded_opts[iopt].pfs) != 0) return true;
    }
    return false;
}

R3S_status_t R3S_cfg_load_opt(R3S_cfg_t *cfg, R3S_opt_t opt)
{
    R3S_status_t s;
    R3S_pf_t     *pfs;
    unsigned     n_pfs;
    unsigned     iopt;

    if (!is_valid_opt(opt)) return R3S_STATUS_OPT_UNKNOWN;

    iopt = cfg->n_loaded_opts;

    cfg->n_loaded_opts++;
    cfg->loaded_opts = (R3S_loaded_opt_t*) realloc(
        cfg->loaded_opts,
        sizeof(R3S_loaded_opt_t) * cfg->n_loaded_opts);
    
    cfg->loaded_opts[iopt].opt = opt;
    cfg->loaded_opts[iopt].pfs = 0;
    cfg->loaded_opts[iopt].sz  = 0;

    s = R3S_opt_to_pfs(opt, &pfs, &n_pfs);
    if (s != R3S_STATUS_SUCCESS) return s;

    for (unsigned i = 0; i < n_pfs; i++)
    {
        s = R3S_cfg_load_pf(cfg, iopt, pfs[i]);
        if (s == R3S_STATUS_PF_UNKNOWN) 
        {
            free(pfs);
            return s;
        }
    }

    free(pfs);

    return R3S_STATUS_SUCCESS;
}

bool is_valid_pf(R3S_pf_t pf)
{
    return pf >= R3S_FIRST_PF && pf <= R3S_LAST_PF;
}

R3S_status_t R3S_cfg_load_pf(R3S_cfg_t *cfg, unsigned iopt, R3S_pf_t pf)
{
    R3S_status_t status;

    status = R3S_cfg_check_pf(*cfg, cfg->loaded_opts[iopt], pf);

    if (status == R3S_STATUS_PF_NOT_LOADED)
    {
        cfg->loaded_opts[iopt].pfs |= (1 << pf);
        cfg->loaded_opts[iopt].sz  += R3S_pf_sz_bits(pf);

        return R3S_STATUS_SUCCESS;
    }

    return status;
}

R3S_status_t R3S_cfg_check_pf(R3S_cfg_t cfg, R3S_loaded_opt_t opt, R3S_pf_t pf)
{
    if (!is_valid_pf(pf)) return R3S_STATUS_PF_UNKNOWN;
    
    return ((opt.pfs >> pf) & 1)
        ? R3S_STATUS_PF_LOADED
        : R3S_STATUS_PF_NOT_LOADED;
}

unsigned R3S_cfg_max_in_sz(R3S_cfg_t cfg)
{
    unsigned max_sz;

    max_sz = 0;

    for (unsigned iopt = 0; iopt < cfg.n_loaded_opts; iopt++)
        max_sz = MAX(max_sz, cfg.loaded_opts[iopt].sz);
    
    return max_sz;
}

typedef struct {
    R3S_opt_t opt;

    R3S_pf_t *missing;
    size_t    missing_sz;

    R3S_pf_t *excess;
    size_t    excess_sz;
} opt_pfs_match_t;

int cmp_opt_pfs_match(opt_pfs_match_t opm1, opt_pfs_match_t opm2) {

    if (opm1.missing_sz < opm2.missing_sz) return 1;
    if (opm1.missing_sz > opm2.missing_sz) return -1;
    
    // opm1.missing_sz is equal to opm2.missing_sz
    
    if (opm1.excess_sz < opm2.excess_sz)  return 1;
    if (opm1.excess_sz > opm2.excess_sz)  return -1;

    // opm1.excess_sz is equal to opm2.excess_sz

    return 0;
}

void opt_cmp_pfs(R3S_opt_t opt, R3S_pf_t *pfs, size_t pfs_sz, opt_pfs_match_t *report) {
    R3S_pf_t *opt_pfs;
    unsigned  opt_pfs_sz;

    if (report->missing_sz > 0) free(report->missing);
    if (report->excess_sz > 0)  free(report->excess);
    
    report->opt        = opt;
    report->missing_sz = 0;
    report->missing    = NULL;
    report->excess_sz  = 0;
    report->excess     = NULL;

    if (pfs_sz == 0) return;

    R3S_opt_to_pfs(opt, &opt_pfs, &opt_pfs_sz);

    for (unsigned i = 0; i < pfs_sz; i++) {
        if (!find((void*) (pfs+i), (void*) opt_pfs, opt_pfs_sz, sizeof(R3S_pf_t))) {
            report->missing_sz++;
            report->missing = (R3S_pf_t*) realloc(
                report->missing,
                sizeof(R3S_pf_t) * report->missing_sz
            );
            report->missing[report->missing_sz - 1] = pfs[i];
        }
    }

    for (unsigned i = 0; i < opt_pfs_sz; i++) {
        if (!find((void*) (opt_pfs+i), (void*) pfs, pfs_sz, sizeof(R3S_pf_t))) {
            report->excess_sz++;
            report->excess = (R3S_pf_t*) realloc(
                report->excess,
                sizeof(R3S_pf_t) * report->excess_sz
            );
            report->excess[report->excess_sz - 1] = opt_pfs[i];
        }
    }
}

R3S_status_t R3S_opts_from_pfs(R3S_pf_t *pfs, size_t pfs_sz, out R3S_opt_t** opts, out size_t *opts_sz) {
    R3S_status_t     status;
    R3S_opt_t        opt;
    opt_pfs_match_t *reports;
    size_t           reports_sz;
    bool             change;

    *opts    = NULL;
    *opts_sz = 0;

    reports_sz = R3S_LAST_OPT - R3S_FIRST_OPT + 1;
    reports    = (opt_pfs_match_t*) malloc(sizeof(opt_pfs_match_t) * reports_sz);

    remove_dup((void**) &pfs, &pfs_sz, sizeof(R3S_pf_t));

    for (unsigned ipf = 0; ipf < pfs_sz; ipf++) {
        if (!is_valid_pf(pfs[ipf])) return R3S_STATUS_PF_UNKNOWN;
    }


    for (unsigned iopt = R3S_FIRST_OPT; iopt <= R3S_LAST_OPT; iopt++) {
        opt = (R3S_opt_t) iopt;
        opt_cmp_pfs(opt, pfs, pfs_sz, reports+iopt);
    }

    // bubble sort yay!
    change = true;
    while (change) {
        change = false;

        for (unsigned i = 0; i < reports_sz - 1; i++) {
            if (cmp_opt_pfs_match(reports[i], reports[i+1]) < 0) {
                opt_pfs_match_t tmp = reports[i+1];
                reports[i+1] = reports[i];
                reports[i]   = tmp;

                change = true;
            }
        }
    }

    if (reports[0].missing_sz == pfs_sz) {
        DEBUG_LOG("FAILED: no set of opts for given pfs\n");
        status = R3S_STATUS_BAD_SOLUTION;
    } else {
        for (unsigned i = 0; i < reports_sz; i++) {
            if (arr_eq(
                (void*) reports[0].missing, reports[0].missing_sz,
                (void*) reports[i].missing, reports[i].missing_sz,
                sizeof(R3S_pf_t)
            )) {
                (*opts_sz)++;
                *opts = (R3S_opt_t*) realloc(
                    *opts,
                    sizeof(R3S_opt_t) * (*opts_sz)
                );
                (*opts)[(*opts_sz) - 1] = reports[i].opt;
            }
        }

        if (reports[0].missing_sz == 0) {
            status = R3S_STATUS_SUCCESS;
        } else {
            R3S_opt_t *missing_opts;
            size_t    missing_opts_sz;

            status = R3S_opts_from_pfs(
                reports[0].missing,
                reports[0].missing_sz,
                &missing_opts,
                &missing_opts_sz
            );

            for (unsigned i = 0; i < missing_opts_sz; i++) {
                (*opts_sz)++;
                *opts = (R3S_opt_t*) realloc(
                    *opts,
                    sizeof(R3S_opt_t) * (*opts_sz)
                );
                (*opts)[(*opts_sz) - 1] = missing_opts[i];
            }

            if (missing_opts_sz) free(missing_opts);
        }
    }

    for (unsigned i = 0; i < reports_sz - 1; i++) {
        if (reports[i].missing_sz > 0) free(reports[i].missing);
        if (reports[i].excess_sz > 0)  free(reports[i].excess);
    }

    free(reports);

    return status;
}

void R3S_set_user_data(out R3S_cfg_t *cfg, void* data) {
    cfg->user_data = data;
}

void R3S_get_user_data(R3S_cfg_t cfg, out void** data) {
    *data = cfg.user_data;
}
