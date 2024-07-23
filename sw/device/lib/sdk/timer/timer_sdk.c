// Copyright 2023 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: timer_sdk.c
// Author: Michele Caon, Francesco Poluzzi
// Date: 23/07/2024
// Author: Michele Caon, Francesco Poluzzi
// Date: 23/07/2024
// Description: Timer functions

#include <stdint.h>

#include "timer_sdk.h"
#include "csr.h"
#include "soc_ctrl.h"


/******************************/
/* ---- GLOBAL VARIABLES ---- */
/******************************/

// Timer value
uint32_t timer_value = 0;
uint32_t hw_timer_value = 0;

rv_timer_t timer;

rv_timer_config_t timer_cfg = {
    .hart_count = RV_TIMER_PARAM_N_HARTS,
    .comparator_count = RV_TIMER_PARAM_N_TIMERS,
};

mmio_region_t timer_base = {
    .base = (void *)RV_TIMER_AO_START_ADDRESS,
};

rv_timer_tick_params_t tick_params;


/*************************************/
/* ---- FUNCTION IMPLEMENTATION ---- */
/*************************************/

uint32_t hw_timer_get_cycles()
{
    uint64_t cycle_count;
    rv_timer_counter_read(&timer, 0, &cycle_count);
    return (uint32_t)cycle_count;
}

void timer_irq_enable()
{
  rv_timer_irq_enable(&timer, 0, 0, kRvTimerEnabled);
}

void timer_irq_clear()
{
  rv_timer_irq_clear(&timer, 0, 0);
}


void timer_arm_start(uint32_t threshold)
{
    rv_timer_arm(&timer, 0, 0, threshold);
    rv_timer_counter_set_enabled(&timer, 0, kRvTimerEnabled);
}

void timer_arm_stop()
{
    rv_timer_counter_set_enabled(&timer, 0, kRvTimerDisabled);
}

void hw_timer_start()
{
    hw_timer_value = -hw_timer_get_cycles();
}

uint32_t hw_timer_stop()
{
    hw_timer_value += hw_timer_get_cycles();
    return hw_timer_value;
}

// Initialize the timer
void timer_cycles_init()
{
    // Get current Frequency
    soc_ctrl_t soc_ctrl;
    soc_ctrl.base_addr = mmio_region_from_addr((uintptr_t)SOC_CTRL_START_ADDRESS);
    uint32_t freq_hz = soc_ctrl_get_frequency(&soc_ctrl);

    // Initialize the timer
    rv_timer_init(timer_base, timer_cfg, &timer);
    rv_timer_approximate_tick_params(freq_hz, freq_hz, &tick_params);
    rv_timer_set_tick_params(&timer, 0, tick_params);
    rv_timer_counter_set_enabled(&timer, 0, kRvTimerEnabled);
}

// Initialize the timer
void  timer_microseconds_init()
{
    // Get current Frequency
    soc_ctrl_t soc_ctrl;
    soc_ctrl.base_addr = mmio_region_from_addr((uintptr_t)SOC_CTRL_START_ADDRESS);
    uint32_t freq_hz = soc_ctrl_get_frequency(&soc_ctrl);

    // Initialize the timer
    rv_timer_init(timer_base, timer_cfg, &timer);
    rv_timer_approximate_tick_params(freq_hz, FREQ_1MHz, &tick_params);
    rv_timer_set_tick_params(&timer, 0, tick_params);
    rv_timer_counter_set_enabled(&timer, 0, kRvTimerEnabled);
}

// Initialize the timer
void timer_wait_ms(uint32_t ms)
{
     timer_microseconds_init();
    timer_irq_enable();
    timer_arm_start(ms*1000);
    timer_start();              
    asm volatile ("wfi");
    timer_irq_clear();
    return;
}

void enable_timer_interrupt()
{
    //enable timer interrupt
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    // Set mie.MEIE bit to one to enable machine-level timer interrupts
    const uint32_t mask = 1 << 7;
    CSR_SET_BITS(CSR_REG_MIE, mask);
}