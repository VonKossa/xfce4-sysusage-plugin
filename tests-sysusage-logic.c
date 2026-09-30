#include <glib.h>

#include "src/sysusage-logic.h"

static void test_net_device_name_validation(void)
{
    g_assert_true(sysusage_logic_net_device_name_is_valid("eth0"));
    g_assert_false(sysusage_logic_net_device_name_is_valid(NULL));
    g_assert_false(sysusage_logic_net_device_name_is_valid(""));
    g_assert_false(sysusage_logic_net_device_name_is_valid("../eth0"));
    g_assert_false(sysusage_logic_net_device_name_is_valid("eth 0"));
}

static void test_mem_swap_percent_basic(void)
{
    gdouble mem_percent = 0.0;
    gdouble swap_percent = 0.0;
    gulong mem_used = 0;
    gulong swap_used = 0;

    sysusage_logic_calculate_mem_swap_percent(1000, 250, 1000, 600,
                                              &mem_percent, &swap_percent,
                                              &mem_used, &swap_used);

    g_assert_cmpfloat(mem_percent, ==, 75.0);
    g_assert_cmpfloat(swap_percent, ==, 40.0);
    g_assert_cmpuint(mem_used, ==, 750);
    g_assert_cmpuint(swap_used, ==, 400);
}

static void test_mem_swap_percent_clamp_and_zero(void)
{
    gdouble mem_percent = -1.0;
    gdouble swap_percent = -1.0;

    sysusage_logic_calculate_mem_swap_percent(0, 0, 0, 0,
                                              &mem_percent, &swap_percent,
                                              NULL, NULL);
    g_assert_cmpfloat(mem_percent, ==, 0.0);
    g_assert_cmpfloat(swap_percent, ==, 0.0);

    sysusage_logic_calculate_mem_swap_percent(100, 150, 100, 120,
                                              &mem_percent, &swap_percent,
                                              NULL, NULL);
    g_assert_cmpfloat(mem_percent, ==, 0.0);
    g_assert_cmpfloat(swap_percent, ==, 0.0);
}

static void test_net_rate_calculation_and_wrap(void)
{
    gdouble in_mbit = 0.0;
    gdouble out_mbit = 0.0;

    sysusage_logic_calculate_net_rates(1000, 2000, 21000, 32000, 2.0,
                                       &in_mbit, &out_mbit);
    g_assert_cmpfloat(in_mbit, ==, 0.08);
    g_assert_cmpfloat(out_mbit, ==, 0.12);

    sysusage_logic_calculate_net_rates(5000, 6000, 3000, 1000, 1.0,
                                       &in_mbit, &out_mbit);
    g_assert_cmpfloat(in_mbit, ==, 0.0);
    g_assert_cmpfloat(out_mbit, ==, 0.0);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/sysusage/net-device-name-validation", test_net_device_name_validation);
    g_test_add_func("/sysusage/mem-swap-percent-basic", test_mem_swap_percent_basic);
    g_test_add_func("/sysusage/mem-swap-percent-clamp-zero", test_mem_swap_percent_clamp_and_zero);
    g_test_add_func("/sysusage/net-rate-calculation-wrap", test_net_rate_calculation_and_wrap);

    return g_test_run();
}
