/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef SYNCHRONIZATION_BARRIER_H
#define SYNCHRONIZATION_BARRIER_H

#include <trident_config.h>
#include <trident/layer1/layer1_types.h>
#include <trident/util.h>

ERRNO sender_barrier_synchronize(MACHINESTATE* machinestate);

ERRNO receiver_barrier_synchronize(MACHINESTATE* machinestate);

#endif // SYNCHRONIZATION_BARRIER_H
