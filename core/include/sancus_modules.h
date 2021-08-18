#ifndef SANCUS_RIOT_MODULES_H
#define SANCUS_RIOT_MODULES_H


#include <sancus/sm_support.h>
#include <sancus_support/sm_io.h>

#ifdef __cplusplus
 extern "C" {
#endif
/**
 * We can use one common Riot vendor ID for all Sancus modules
 * */
#define SANCUS_RIOT_ID 0x1234

// extern struct SancusModule sancus_sm_timer;

/**
 * The Timer module uses two sancus modules. One MMIO module and one for handling this mmio module.
 * */



extern struct SancusModule sancus_mmio_timer;
extern struct SancusModule sancus_sm_timer;


#ifdef __cplusplus
}
#endif

#endif