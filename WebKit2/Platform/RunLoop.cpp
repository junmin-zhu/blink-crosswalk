/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "RunLoop.h"

#include "WorkItem.h"
#include <wtf/StdLibExtras.h>

static RunLoop* s_mainRunLoop;

void RunLoop::initializeMainRunLoop()
{
    if (s_mainRunLoop)
        return;
    s_mainRunLoop = RunLoop::current();
}

RunLoop* RunLoop::current()
{
    DEFINE_STATIC_LOCAL(WTF::ThreadSpecific<RunLoop>, runLoopData, ());
    return &*runLoopData;
}

RunLoop* RunLoop::main()
{
    ASSERT(s_mainRunLoop);
    return s_mainRunLoop;
}

void RunLoop::performWork()
{
    Vector<WorkItem*> workItemQueue;
    {
        MutexLocker locker(m_workItemQueueLock);
        m_workItemQueue.swap(workItemQueue);
    }

    for (size_t i = 0; i < workItemQueue.size(); ++i) {
        std::auto_ptr<WorkItem> item(workItemQueue[i]);
        item->execute();
    }
}

void RunLoop::scheduleWork(std::auto_ptr<WorkItem> item)
{
    MutexLocker locker(m_workItemQueueLock);
    m_workItemQueue.append(item.release());

    wakeUp();
}
