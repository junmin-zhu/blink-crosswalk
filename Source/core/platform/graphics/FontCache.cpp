/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Nicholas Shanks <webkit@nickshanks.com>
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include "core/platform/graphics/FontCache.h"

#include "FontFamilyNames.h"

#include "RuntimeEnabledFeatures.h"
#include "core/platform/graphics/Font.h"
#include "core/platform/graphics/FontFallbackList.h"
#include "core/platform/graphics/FontPlatformData.h"
#include "core/platform/graphics/opentype/OpenTypeVerticalData.h"
#include "platform/fonts/AlternateFontFamily.h"
#include "platform/fonts/FontCacheKey.h"
#include "platform/fonts/FontDescription.h"
#include "platform/fonts/FontSelector.h"
#include "platform/fonts/FontSmoothingMode.h"
#include "platform/fonts/TextRenderingMode.h"
#include "wtf/HashMap.h"
#include "wtf/ListHashSet.h"
#include "wtf/StdLibExtras.h"
#include "wtf/text/AtomicStringHash.h"
#include "wtf/text/StringHash.h"

using namespace WTF;

namespace WebCore {

FontCache* fontCache()
{
    DEFINE_STATIC_LOCAL(FontCache, globalFontCache, ());
    return &globalFontCache;
}

#if !OS(WIN) || ENABLE(GDI_FONTS_ON_WINDOWS)
FontCache::FontCache()
    : m_purgePreventCount(0)
{
}
#endif // !OS(WIN) || ENABLE(GDI_FONTS_ON_WINDOWS)

typedef HashMap<FontCacheKey, OwnPtr<FontPlatformData>, FontCacheKeyHash, FontCacheKeyTraits> FontPlatformDataCache;

static FontPlatformDataCache* gFontPlatformDataCache = 0;

FontPlatformData* FontCache::getFontResourcePlatformData(const FontDescription& fontDescription,
    const AtomicString& passedFamilyName, bool checkingAlternateName)
{
#if OS(WIN) && ENABLE(OPENTYPE_VERTICAL)
    // Leading "@" in the font name enables Windows vertical flow flag for the font.
    // Because we do vertical flow by ourselves, we don't want to use the Windows feature.
    // IE disregards "@" regardless of the orientatoin, so we follow the behavior.
    const AtomicString& familyName = (passedFamilyName.isEmpty() || passedFamilyName[0] != '@') ?
        passedFamilyName : AtomicString(passedFamilyName.impl()->substring(1));
#else
    const AtomicString& familyName = passedFamilyName;
#endif

    if (!gFontPlatformDataCache) {
        gFontPlatformDataCache = new FontPlatformDataCache;
        platformInit();
    }

    FontCacheKey key = fontDescription.cacheKey(familyName);
    FontPlatformData* result = 0;
    bool foundResult;
    FontPlatformDataCache::iterator it = gFontPlatformDataCache->find(key);
    if (it == gFontPlatformDataCache->end()) {
        result = createFontPlatformData(fontDescription, familyName, fontDescription.effectiveFontSize());
        gFontPlatformDataCache->set(key, adoptPtr(result));
        foundResult = result;
    } else {
        result = it->value.get();
        foundResult = true;
    }

    if (!foundResult && !checkingAlternateName) {
        // We were unable to find a font. We have a small set of fonts that we alias to other names,
        // e.g., Arial/Helvetica, Courier/Courier New, etc. Try looking up the font under the aliased name.
        const AtomicString& alternateName = alternateFamilyName(familyName);
        if (!alternateName.isEmpty())
            result = getFontResourcePlatformData(fontDescription, alternateName, true);
        if (result)
            gFontPlatformDataCache->set(key, adoptPtr(new FontPlatformData(*result))); // Cache the result under the old name.
    }

    return result;
}

#if ENABLE(OPENTYPE_VERTICAL)
typedef HashMap<FontCache::FontFileKey, RefPtr<OpenTypeVerticalData>, WTF::IntHash<FontCache::FontFileKey>, WTF::UnsignedWithZeroKeyHashTraits<FontCache::FontFileKey> > FontVerticalDataCache;

FontVerticalDataCache& fontVerticalDataCacheInstance()
{
    DEFINE_STATIC_LOCAL(FontVerticalDataCache, fontVerticalDataCache, ());
    return fontVerticalDataCache;
}

PassRefPtr<OpenTypeVerticalData> FontCache::getVerticalData(const FontFileKey& key, const FontPlatformData& platformData)
{
    FontVerticalDataCache& fontVerticalDataCache = fontVerticalDataCacheInstance();
    FontVerticalDataCache::iterator result = fontVerticalDataCache.find(key);
    if (result != fontVerticalDataCache.end())
        return result.get()->value;

    RefPtr<OpenTypeVerticalData> verticalData = OpenTypeVerticalData::create(platformData);
    if (!verticalData->isOpenType())
        verticalData.clear();
    fontVerticalDataCache.set(key, verticalData);
    return verticalData;
}
#endif

struct FontDataCacheKeyHash {
    static unsigned hash(const FontPlatformData& platformData)
    {
        return platformData.hash();
    }

    static bool equal(const FontPlatformData& a, const FontPlatformData& b)
    {
        return a == b;
    }

    static const bool safeToCompareToEmptyOrDeleted = true;
};

struct FontDataCacheKeyTraits : WTF::GenericHashTraits<FontPlatformData> {
    static const bool emptyValueIsZero = true;
    static const bool needsDestruction = true;
    static const FontPlatformData& emptyValue()
    {
        DEFINE_STATIC_LOCAL(FontPlatformData, key, (0.f, false, false));
        return key;
    }
    static void constructDeletedValue(FontPlatformData& slot)
    {
        new (NotNull, &slot) FontPlatformData(HashTableDeletedValue);
    }
    static bool isDeletedValue(const FontPlatformData& value)
    {
        return value.isHashTableDeletedValue();
    }
};

typedef HashMap<FontPlatformData, pair<RefPtr<SimpleFontData>, unsigned>, FontDataCacheKeyHash, FontDataCacheKeyTraits> FontDataCache;

static FontDataCache* gFontDataCache = 0;

#if !OS(ANDROID)
const int cMaxInactiveFontData = 250;
const int cTargetInactiveFontData = 200;
#else
const int cMaxInactiveFontData = 225;
const int cTargetInactiveFontData = 200;
#endif
static ListHashSet<RefPtr<SimpleFontData> >* gInactiveFontData = 0;

PassRefPtr<SimpleFontData> FontCache::getFontResourceData(const FontDescription& fontDescription, const AtomicString& family, bool checkingAlternateName, ShouldRetain shouldRetain)
{
    FontPlatformData* platformData;

    const AtomicString& preferedAlternateName = alternateFamilyNameAvoidingBitmapFonts(family);
    if (!preferedAlternateName.isEmpty())
        platformData = getFontResourcePlatformData(fontDescription, preferedAlternateName, checkingAlternateName);
    else
        platformData = getFontResourcePlatformData(fontDescription, family, checkingAlternateName);

    if (!platformData)
        return 0;

    return getFontResourceData(platformData, shouldRetain);
}

PassRefPtr<SimpleFontData> FontCache::getFontResourceData(const FontPlatformData* platformData, ShouldRetain shouldRetain)
{
    if (!platformData)
        return 0;

#if !ASSERT_DISABLED
    if (shouldRetain == DoNotRetain)
        ASSERT(m_purgePreventCount);
#endif

    if (!gFontDataCache) {
        gFontDataCache = new FontDataCache;
        gInactiveFontData = new ListHashSet<RefPtr<SimpleFontData> >;
    }

    FontDataCache::iterator result = gFontDataCache->find(*platformData);
    if (result == gFontDataCache->end()) {
        pair<RefPtr<SimpleFontData>, unsigned> newValue(SimpleFontData::create(*platformData), shouldRetain == Retain ? 1 : 0);
        gFontDataCache->set(*platformData, newValue);
        if (shouldRetain == DoNotRetain)
            gInactiveFontData->add(newValue.first);
        return newValue.first.release();
    }

    if (!result.get()->value.second) {
        ASSERT(gInactiveFontData->contains(result.get()->value.first));
        gInactiveFontData->remove(result.get()->value.first);
    }

    if (shouldRetain == Retain) {
        result.get()->value.second++;
    } else if (!result.get()->value.second) {
        // If shouldRetain is DoNotRetain and count is 0, we want to remove the fontData from
        // gInactiveFontData (above) and re-add here to update LRU position.
        gInactiveFontData->add(result.get()->value.first);
    }

    return result.get()->value.first;
}

bool FontCache::isPlatformFontAvailable(const FontDescription& fontDescription, const AtomicString& family, bool checkingAlternateName)
{
    return getFontResourcePlatformData(fontDescription, family, checkingAlternateName);
}

SimpleFontData* FontCache::getNonRetainedLastResortFallbackFont(const FontDescription& fontDescription)
{
    return getLastResortFallbackFont(fontDescription, DoNotRetain).leakRef();
}

void FontCache::releaseFontData(const SimpleFontData* fontData)
{
    ASSERT(gFontDataCache);
    ASSERT(!fontData->isCustomFont());

    FontDataCache::iterator it = gFontDataCache->find(fontData->platformData());
    ASSERT(it != gFontDataCache->end());
    if (it == gFontDataCache->end())
        return;

    ASSERT(it->value.second);
    if (!--it->value.second)
        gInactiveFontData->add(it->value.first);
}

void FontCache::purgeInactiveFontDataIfNeeded()
{
    if (gInactiveFontData && !m_purgePreventCount && gInactiveFontData->size() > cMaxInactiveFontData)
        purgeInactiveFontData(gInactiveFontData->size() - cTargetInactiveFontData);
}

void FontCache::purgeInactiveFontData(int count)
{
    if (!gInactiveFontData || m_purgePreventCount)
        return;

    static bool isPurging; // Guard against reentry when e.g. a deleted FontData releases its small caps FontData.
    if (isPurging)
        return;

    isPurging = true;

    Vector<RefPtr<SimpleFontData>, 20> fontDataToDelete;
    ListHashSet<RefPtr<SimpleFontData> >::iterator end = gInactiveFontData->end();
    ListHashSet<RefPtr<SimpleFontData> >::iterator it = gInactiveFontData->begin();
    for (int i = 0; i < count && it != end; ++it, ++i) {
        RefPtr<SimpleFontData>& fontData = *it.get();
        gFontDataCache->remove(fontData->platformData());
        // We should not delete SimpleFontData here because deletion can modify gInactiveFontData. See http://trac.webkit.org/changeset/44011
        fontDataToDelete.append(fontData);
    }

    if (it == end) {
        // Removed everything
        gInactiveFontData->clear();
    } else {
        for (int i = 0; i < count; ++i)
            gInactiveFontData->remove(gInactiveFontData->begin());
    }

    fontDataToDelete.clear();

    if (gFontPlatformDataCache) {
        Vector<FontCacheKey> keysToRemove;
        keysToRemove.reserveInitialCapacity(gFontPlatformDataCache->size());
        FontPlatformDataCache::iterator platformDataEnd = gFontPlatformDataCache->end();
        for (FontPlatformDataCache::iterator platformData = gFontPlatformDataCache->begin(); platformData != platformDataEnd; ++platformData) {
            if (platformData->value && !gFontDataCache->contains(*platformData->value.get()))
                keysToRemove.append(platformData->key);
        }

        size_t keysToRemoveCount = keysToRemove.size();
        for (size_t i = 0; i < keysToRemoveCount; ++i)
            gFontPlatformDataCache->remove(keysToRemove[i]);
    }

#if ENABLE(OPENTYPE_VERTICAL)
    FontVerticalDataCache& fontVerticalDataCache = fontVerticalDataCacheInstance();
    if (!fontVerticalDataCache.isEmpty()) {
        // Mark & sweep unused verticalData
        FontVerticalDataCache::iterator verticalDataEnd = fontVerticalDataCache.end();
        for (FontVerticalDataCache::iterator verticalData = fontVerticalDataCache.begin(); verticalData != verticalDataEnd; ++verticalData) {
            if (verticalData->value)
                verticalData->value->m_inFontCache = false;
        }
        FontDataCache::iterator fontDataEnd = gFontDataCache->end();
        for (FontDataCache::iterator fontData = gFontDataCache->begin(); fontData != fontDataEnd; ++fontData) {
            OpenTypeVerticalData* verticalData = const_cast<OpenTypeVerticalData*>(fontData->value.first->verticalData());
            if (verticalData)
                verticalData->m_inFontCache = true;
        }
        Vector<FontFileKey> keysToRemove;
        keysToRemove.reserveInitialCapacity(fontVerticalDataCache.size());
        for (FontVerticalDataCache::iterator verticalData = fontVerticalDataCache.begin(); verticalData != verticalDataEnd; ++verticalData) {
            if (!verticalData->value || !verticalData->value->m_inFontCache)
                keysToRemove.append(verticalData->key);
        }
        for (size_t i = 0, count = keysToRemove.size(); i < count; ++i)
            fontVerticalDataCache.take(keysToRemove[i]);
    }
#endif

    isPurging = false;
}

size_t FontCache::fontDataCount()
{
    if (gFontDataCache)
        return gFontDataCache->size();
    return 0;
}

size_t FontCache::inactiveFontDataCount()
{
    if (gInactiveFontData)
        return gInactiveFontData->size();
    return 0;
}

PassRefPtr<FontData> FontCache::getFontData(const Font& font, int& familyIndex, FontSelector* fontSelector)
{
    RefPtr<FontData> result;

    int startIndex = familyIndex;
    const FontFamily* startFamily = &font.fontDescription().family();
    for (int i = 0; startFamily && i < startIndex; i++)
        startFamily = startFamily->next();
    const FontFamily* currFamily = startFamily;
    while (currFamily && !result) {
        familyIndex++;
        if (currFamily->family().length()) {
            if (fontSelector)
                result = fontSelector->getFontData(font.fontDescription(), currFamily->family());

            if (!result)
                result = getFontResourceData(font.fontDescription(), currFamily->family());
        }
        currFamily = currFamily->next();
    }

    if (!currFamily)
        familyIndex = cAllFamiliesScanned;

    if (!result) {
        // We didn't find a font. Try to find a similar font using our own specific knowledge about our platform.
        // For example on OS X, we know to map any families containing the words Arabic, Pashto, or Urdu to the
        // Geeza Pro font.
        result = getSimilarFontPlatformData(font);
    }

    if (!result && !startIndex) {
        // If it's the primary font that we couldn't find, we try the following. In all other cases, we will
        // just use per-character system fallback.

        if (fontSelector) {
            // Try the user's preferred standard font.
            if (RefPtr<FontData> data = fontSelector->getFontData(font.fontDescription(), FontFamilyNames::webkit_standard))
                return data.release();
        }

        // Still no result. Hand back our last resort fallback font.
        result = getLastResortFallbackFont(font.fontDescription());
    }
    return result.release();
}

static HashSet<FontSelector*>* gClients;

void FontCache::addClient(FontSelector* client)
{
    if (!gClients)
        gClients = new HashSet<FontSelector*>;

    ASSERT(!gClients->contains(client));
    gClients->add(client);
}

void FontCache::removeClient(FontSelector* client)
{
    ASSERT(gClients);
    ASSERT(gClients->contains(client));

    gClients->remove(client);
}

static unsigned short gGeneration = 0;

unsigned short FontCache::generation()
{
    return gGeneration;
}

void FontCache::invalidate()
{
    if (!gClients) {
        ASSERT(!gFontPlatformDataCache);
        return;
    }

    if (gFontPlatformDataCache) {
        delete gFontPlatformDataCache;
        gFontPlatformDataCache = new FontPlatformDataCache;
    }

    gGeneration++;

    Vector<RefPtr<FontSelector> > clients;
    size_t numClients = gClients->size();
    clients.reserveInitialCapacity(numClients);
    HashSet<FontSelector*>::iterator end = gClients->end();
    for (HashSet<FontSelector*>::iterator it = gClients->begin(); it != end; ++it)
        clients.append(*it);

    ASSERT(numClients == clients.size());
    for (size_t i = 0; i < numClients; ++i)
        clients[i]->fontCacheInvalidated();

    purgeInactiveFontData();
}

} // namespace WebCore
