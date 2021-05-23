/*
 * Copyright 2017 Andrei Pangin
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef __x86_64__

#include "stackFrame.h"


#ifdef __APPLE__
#  define REG(l, m)  _ucontext->uc_mcontext->__ss.m
#else
#  define REG(l, m)  _ucontext->uc_mcontext.gregs[l]
#endif


uintptr_t& StackFrame::pc() {
    return (uintptr_t&)REG(REG_RIP, __rip);
}

uintptr_t& StackFrame::sp() {
    return (uintptr_t&)REG(REG_RSP, __rsp);
}

uintptr_t& StackFrame::fp() {
    return (uintptr_t&)REG(REG_RBP, __rbp);
}

uintptr_t StackFrame::arg0() {
    return (uintptr_t)REG(REG_RDI, __rdi);
}

uintptr_t StackFrame::arg1() {
    return (uintptr_t)REG(REG_RSI, __rsi);
}

uintptr_t StackFrame::arg2() {
    return (uintptr_t)REG(REG_RDX, __rdx);
}

uintptr_t StackFrame::arg3() {
    return (uintptr_t)REG(REG_RCX, __rcx);
}

void StackFrame::ret() {
    pc() = stackAt(0);
    sp() += 8;
}


static inline bool isFramePrologueEpilogue(uintptr_t pc) {
    if (pc & 0xfff) {
        // Make sure we are not at the page boundary, so that reading [pc - 1] is safe
        unsigned int opcode = *(unsigned int*)(pc - 1);
        if (opcode == 0xec834855) {
            // push rbp
            // sub  rsp, $const
            return true;
        } else if (opcode == 0xec8b4855) {
            // push rbp
            // mov  rbp, rsp
            return true;
        }
    }

    if (*(unsigned char*)pc == 0x5d && *(unsigned short*)(pc + 1) == 0x0585) {
        // pop  rbp
        // test [polling_page], eax
        return true;
    }

    return false;
}

bool StackFrame::pop(bool trust_frame_pointer) {
    if (trust_frame_pointer && withinCurrentStack(fp())) {
        sp() = fp() + 16;
        fp() = stackAt(-2);
        pc() = stackAt(-1);
        return true;
    } else if (fp() == sp() || withinCurrentStack(stackAt(0)) || isFramePrologueEpilogue(pc())) {
        fp() = stackAt(0);
        pc() = stackAt(1);
        sp() += 16;
        return true;
    }
    return false;
}

int StackFrame::callerLookupSlots() {
    return 7;
}

bool StackFrame::isReturnAddress(instruction_t* pc) {
    if (pc[-5] == 0xe8) {
        // call rel32
        return true;
    } else if (pc[-2] == 0xff && ((pc[-1] & 0xf0) == 0xd0 || (pc[-1] & 0xf0) == 0x10)) {
        // call reg or call [reg]
        return true;
    }
    return false;
}

#endif // __x86_64__
