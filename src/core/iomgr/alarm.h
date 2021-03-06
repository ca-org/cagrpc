/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef GRPC_INTERNAL_CORE_IOMGR_ALARM_H
#define GRPC_INTERNAL_CORE_IOMGR_ALARM_H

#include "src/core/iomgr/iomgr.h"
#include <grpc/support/port_platform.h>
#include <grpc/support/grpc_time.h>

typedef struct grpc_alarm {
  gpr_timespec deadline;
  gpr_uint32 heap_index; /* INVALID_HEAP_INDEX if not in heap */
  int triggered;
  struct grpc_alarm *next;
  struct grpc_alarm *prev;
  grpc_iomgr_cb_func cb;
  void *cb_arg;
} grpc_alarm;

/* Initialize *alarm. When expired or canceled, alarm_cb will be called with
   *alarm_cb_arg and status to indicate if it expired (SUCCESS) or was
   canceled (CANCELLED). alarm_cb is guaranteed to be called exactly once,
   and application code should check the status to determine how it was
   invoked. The application callback is also responsible for maintaining
   information about when to free up any user-level state. */
void grpc_alarm_init(grpc_alarm *alarm, gpr_timespec deadline,
                     grpc_iomgr_cb_func alarm_cb, void *alarm_cb_arg,
                     gpr_timespec now);

/* Note that there is no alarm destroy function. This is because the
   alarm is a one-time occurrence with a guarantee that the callback will
   be called exactly once, either at expiration or cancellation. Thus, all
   the internal alarm event management state is destroyed just before
   that callback is invoked. If the user has additional state associated with
   the alarm, the user is responsible for determining when it is safe to
   destroy that state. */

/* Cancel an *alarm.
   There are three cases:
   1. We normally cancel the alarm
   2. The alarm has already run
   3. We can't cancel the alarm because it is "in flight".

   In all of these cases, the cancellation is still considered successful.
   They are essentially distinguished in that the alarm_cb will be run
   exactly once from either the cancellation (with status CANCELLED)
   or from the activation (with status SUCCESS)

   Note carefully that the callback function MAY occur in the same callstack
   as grpc_alarm_cancel. It's expected that most alarms will be cancelled (their
   primary use is to implement deadlines), and so this code is optimized such
   that cancellation costs as little as possible. Making callbacks run inline
   matches this aim.

   Requires:  cancel() must happen after add() on a given alarm */
void grpc_alarm_cancel(grpc_alarm *alarm);

#endif /* GRPC_INTERNAL_CORE_IOMGR_ALARM_H */
