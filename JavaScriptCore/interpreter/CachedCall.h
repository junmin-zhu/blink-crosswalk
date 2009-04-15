/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef CachedCall_h
#define CachedCall_h

#include "Interpreter.h"

namespace JSC {
    class CachedCall : Noncopyable {
    public:
        CachedCall(CallFrame* callFrame, JSFunction* function, int argCount, JSValuePtr* exception)
            : m_valid(false)
            , m_interpreter(callFrame->interpreter())
            , m_exception(exception)
            , m_globalObjectScope(callFrame, callFrame->globalData().dynamicGlobalObject ? callFrame->globalData().dynamicGlobalObject : function->scope().node()->globalObject())
        {
            m_closure = m_interpreter->prepareForRepeatCall(function->body(), callFrame, function, argCount, function->scope().node(), exception);
            m_valid = !exception;
        }
        
        JSValuePtr call()
        { 
            return m_interpreter->execute(m_closure, m_exception);
        }
        void setThis(JSValuePtr v) { m_closure.setArgument(0, v); }
        void setArgument(int n, JSValuePtr v) { m_closure.setArgument(n + 1, v); }
        
        ~CachedCall()
        {
            if (m_valid)
                m_interpreter->endRepeatCall(m_closure);
        }
        
    private:
        bool m_valid;
        Interpreter* m_interpreter;
        JSValuePtr* m_exception;
        DynamicGlobalObjectScope m_globalObjectScope;
        Interpreter::CallFrameClosure m_closure;
    };
}

#endif
