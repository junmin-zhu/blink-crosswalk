/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "modules/indexeddb/IDBVersionChangeEvent.h"

#include "core/events/ThreadLocalEventNames.h"
#include "modules/indexeddb/IDBAny.h"

namespace WebCore {

PassRefPtr<IDBVersionChangeEvent> IDBVersionChangeEvent::create(PassRefPtr<IDBAny> oldVersion, PassRefPtr<IDBAny> newVersion, const AtomicString& eventType, WebKit::WebIDBCallbacks::DataLoss dataLoss, const String& dataLossMessage)
{
    return adoptRef(new IDBVersionChangeEvent(oldVersion, newVersion, eventType, dataLoss, dataLossMessage));
}

IDBVersionChangeEvent::IDBVersionChangeEvent(PassRefPtr<IDBAny> oldVersion, PassRefPtr<IDBAny> newVersion, const AtomicString& eventType, WebKit::WebIDBCallbacks::DataLoss dataLoss, const String& dataLossMessage)
    : Event(eventType, false /*canBubble*/, false /*cancelable*/)
    , m_oldVersion(oldVersion)
    , m_newVersion(newVersion)
    , m_dataLoss(dataLoss)
    , m_dataLossMessage(dataLossMessage)
{
    ScriptWrappable::init(this);
}

IDBVersionChangeEvent::~IDBVersionChangeEvent()
{
}

const AtomicString& IDBVersionChangeEvent::dataLoss()
{
    DEFINE_STATIC_LOCAL(AtomicString, total, ("total", AtomicString::ConstructFromLiteral));
    if (m_dataLoss == WebKit::WebIDBCallbacks::DataLossTotal)
        return total;
    DEFINE_STATIC_LOCAL(AtomicString, none, ("none", AtomicString::ConstructFromLiteral));
    return none;
}

const AtomicString& IDBVersionChangeEvent::interfaceName() const
{
    return EventNames::IDBVersionChangeEvent;
}

} // namespace WebCore
