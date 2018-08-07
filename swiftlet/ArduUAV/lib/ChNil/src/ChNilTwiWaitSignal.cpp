/* Arduino NilRTOS Library
 * Copyright (C) 2013 by William Greiman
 *
 * This file is part of the Arduino NilRTOS Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino NilRTOS Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "ChNil.h"
static SEMAPHORE_DECL(twiSem, 0);

void twiMstrSignal() {
  CH_IRQ_PROLOGUE();
  chSemSignalI(&twiSem);
  CH_IRQ_EPILOGUE();
}

void twiMstrWait() {
  if (chIsIdleThread()) {
    // Can't sleep in idle thread.
    while (chSemGetCounterI(&twiSem) <= 0) {}/////////////////////// lock?/////////////////////////////////
  }
  chSemWait(&twiSem);
}