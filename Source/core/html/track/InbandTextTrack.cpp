/*
 * Copyright (C) 2012 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/html/track/InbandTextTrack.h"

#include "bindings/v8/ExceptionStatePlaceholder.h"
#include "core/html/track/TextTrackCue.h"
#include "core/platform/graphics/InbandTextTrackPrivate.h"
#include "platform/Logging.h"
#include "wtf/UnusedParam.h"
#include <math.h>

namespace WebCore {

PassRefPtr<InbandTextTrack> InbandTextTrack::create(Document& document, TextTrackClient* client, PassRefPtr<InbandTextTrackPrivate> playerPrivate)
{
    return adoptRef(new InbandTextTrack(document, client, playerPrivate));
}

InbandTextTrack::InbandTextTrack(Document& document, TextTrackClient* client, PassRefPtr<InbandTextTrackPrivate> tracksPrivate)
    : TextTrack(document, client, emptyString(), tracksPrivate->label(), tracksPrivate->language(), InBand)
    , m_private(tracksPrivate)
{
    m_private->setClient(this);

    switch (m_private->kind()) {
    case InbandTextTrackPrivate::Subtitles:
        setKind(TextTrack::subtitlesKeyword());
        break;
    case InbandTextTrackPrivate::Captions:
        setKind(TextTrack::captionsKeyword());
        break;
    case InbandTextTrackPrivate::Descriptions:
        setKind(TextTrack::descriptionsKeyword());
        break;
    case InbandTextTrackPrivate::Chapters:
        setKind(TextTrack::chaptersKeyword());
        break;
    case InbandTextTrackPrivate::Metadata:
        setKind(TextTrack::metadataKeyword());
        break;
    case InbandTextTrackPrivate::None:
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

InbandTextTrack::~InbandTextTrack()
{
    // Make sure m_private was cleared by trackRemoved() before destruction.
    ASSERT(!m_private);
}

void InbandTextTrack::setMode(const AtomicString& mode)
{
    TextTrack::setMode(mode);

    if (!m_private)
        return;

    if (mode == TextTrack::disabledKeyword())
        m_private->setMode(InbandTextTrackPrivate::Disabled);
    else if (mode == TextTrack::hiddenKeyword())
        m_private->setMode(InbandTextTrackPrivate::Hidden);
    else if (mode == TextTrack::showingKeyword())
        m_private->setMode(InbandTextTrackPrivate::Showing);
    else
        ASSERT_NOT_REACHED();
}

bool InbandTextTrack::isClosedCaptions() const
{
    if (!m_private)
        return false;

    return m_private->isClosedCaptions();
}

bool InbandTextTrack::containsOnlyForcedSubtitles() const
{
    if (!m_private)
        return false;

    return m_private->containsOnlyForcedSubtitles();
}

bool InbandTextTrack::isMainProgramContent() const
{
    if (!m_private)
        return false;

    return m_private->isMainProgramContent();
}

bool InbandTextTrack::isEasyToRead() const
{
    if (!m_private)
        return false;

    return m_private->isEasyToRead();
}

size_t InbandTextTrack::inbandTrackIndex()
{
    ASSERT(m_private);
    return m_private->textTrackIndex();
}

void InbandTextTrack::trackRemoved()
{
    ASSERT(m_private);
    m_private->setClient(0);
    m_private = 0;
    clearClient();
}

void InbandTextTrack::addWebVTTCue(InbandTextTrackPrivate* trackPrivate, double start, double end, const String& id, const String& content, const String& settings)
{
    ASSERT_UNUSED(trackPrivate, trackPrivate == m_private);

    RefPtr<TextTrackCue> cue = TextTrackCue::create(document(), start, end, content);
    cue->setId(id);
    cue->setCueSettings(settings);

    if (hasCue(cue.get()))
        return;

    addCue(cue);
}

} // namespace WebCore
