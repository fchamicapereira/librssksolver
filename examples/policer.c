/*
 * Policer example.
 * 
 * Policer is a network function responsible for controlling the bandwidth used by clients,
 * which typically is associated with a contract.
 * 
 * Each bandwidth is associated with a client's IP address, so packet counting is made on
 * every packet routed to the client. This means that the network function keeps a global
 * state associated with the destination IP of each incoming packet.
 * 
 * In order to fully parallelize this network function, the packets with equal destination
 * IP must be sent to the same core. This allows for access to shared data structures
 * without the need to use locking mechanisms.
 * 
 * This example provides the means to retrieve an RSS key that complies with these
 * requirements, i.e., an RSS configuration (RSS key and configuration options) that
 * sends packets with the same destination IP address to the same core.
 */

#include <r3s.h>
#include <stdio.h>
#include <stdlib.h>

Z3_ast mk_p_cnstrs(R3S_cfg_t cfg, R3S_packet_ast_t p1, R3S_packet_ast_t p2)
{
    R3S_status_t status;
    Z3_ast       p1_ipv4_src;
    Z3_ast       p2_ipv4_src;
    Z3_ast       eq_src_ip;

    status = R3S_packet_extract_pf(cfg, p1, R3S_PF_IPV4_DST, &p1_ipv4_src);
    if (status != R3S_STATUS_SUCCESS) return NULL;

    status = R3S_packet_extract_pf(cfg, p2, R3S_PF_IPV4_DST, &p2_ipv4_src);
    if (status != R3S_STATUS_SUCCESS) return NULL;

    eq_src_ip = Z3_mk_eq(cfg.ctx, p1_ipv4_src, p2_ipv4_src);
    
    printf("\np1 option: %s\n", R3S_opt_to_string(p1.loaded_opt.opt));
    printf("\np2 option: %s\n", R3S_opt_to_string(p2.loaded_opt.opt));
    printf("\nConstraint:\n%s\n", Z3_ast_to_string(cfg.ctx, eq_src_ip));
    printf("\nSimplified:\n%s\n", Z3_ast_to_string(cfg.ctx, Z3_simplify(cfg.ctx, eq_src_ip)));
    return Z3_simplify(cfg.ctx, eq_src_ip);
}

int generate_packets(R3S_cfg_t cfg)
{
    R3S_packet_t p1, p2;

    for (int i = 0; i < 25; i++)
    {
        R3S_packet_rand(cfg, &p1);
        R3S_packet_from_cnstrs(cfg, p1, &mk_p_cnstrs, &p2);

        printf("\n===== iteration %d =====\n", i);

        printf("%s\n", R3S_packet_to_string(p1));
        printf("%s\n", R3S_packet_to_string(p2));
    }

    return 1;
}

int main() {
    R3S_cfg_t cfg;
    R3S_key_t k;
    R3S_cnstrs_func cnstrs[1];
    R3S_status_t status;
    R3S_opt_t* opts;
    size_t opts_sz;
    R3S_pf_t pfs[1] = { R3S_PF_IPV4_DST };

    R3S_cfg_init(&cfg);
    R3S_opts_from_pfs(pfs, 1, &opts, &opts_sz);

    /*
    printf("Resulting options:\n");
    for (unsigned i = 0; i < opts_sz; i++) {
        printf("%s\n", R3S_opt_to_string(opts[i]));
        R3S_cfg_load_opt(&cfg, opts[i]);
    }
    */
    
    R3S_cfg_load_opt(&cfg, R3S_OPT_NON_FRAG_IPV4);
    
    printf("\nConfiguration:\n%s\n", R3S_cfg_to_string(cfg));

    cnstrs[0] = &mk_p_cnstrs;
    /*
    status = R3S_keys_fit_cnstrs(cfg, cnstrs, &k);
    
    printf("%s\n", R3S_status_to_string(status));

    if (status == R3S_STATUS_SUCCESS)
        printf("result:\n%s\n", R3S_key_to_string(k));

    status = R3S_keys_test_cnstrs(cfg, cnstrs, &k);
    printf("valid keys: %s\n", R3S_status_to_string(status));
    */
    
    generate_packets(cfg);

    R3S_cfg_delete(&cfg);

    free(opts);
}

