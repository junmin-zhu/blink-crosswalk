/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>
                  2005 Alexander Kellett <lypanov@kde.org>

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"
#ifdef SVG_SUPPORT
#include "SVGMaskElement.h"

#include "GraphicsContext.h"
#include "KCanvasImage.h"
#include "KRenderingDevice.h"
#include "RenderSVGContainer.h"
#include "SVGHelper.h"
#include "SVGLength.h"
#include "SVGNames.h"
#include "cssstyleselector.h"
#include <wtf/OwnPtr.h>
#include <math.h>

namespace WebCore {

SVGMaskElement::SVGMaskElement(const QualifiedName& tagName, Document* doc)
    : SVGStyledLocatableElement(tagName, doc)
    , SVGURIReference()
    , SVGTests()
    , SVGLangSpace()
    , SVGExternalResourcesRequired()
    , m_x(new SVGLength(this, LM_WIDTH, viewportElement()))
    , m_y(new SVGLength(this, LM_HEIGHT, viewportElement()))
    , m_width(new SVGLength(this, LM_WIDTH, viewportElement()))
    , m_height(new SVGLength(this, LM_HEIGHT, viewportElement()))
    , m_masker(0)
    , m_dirty(true)
{
}

SVGMaskElement::~SVGMaskElement()
{
    delete m_masker;
}

ANIMATED_PROPERTY_DEFINITIONS(SVGMaskElement, SVGLength*, Length, length, X, x, SVGNames::xAttr.localName(), m_x.get())
ANIMATED_PROPERTY_DEFINITIONS(SVGMaskElement, SVGLength*, Length, length, Y, y, SVGNames::yAttr.localName(), m_y.get())
ANIMATED_PROPERTY_DEFINITIONS(SVGMaskElement, SVGLength*, Length, length, Width, width, SVGNames::widthAttr.localName(), m_width.get())
ANIMATED_PROPERTY_DEFINITIONS(SVGMaskElement, SVGLength*, Length, length, Height, height, SVGNames::heightAttr.localName(), m_height.get())

void SVGMaskElement::attributeChanged(Attribute* attr, bool preserveDecls)
{
    IntSize newSize = IntSize(lroundf(width()->value()), lroundf(height()->value()));
    if (!m_masker || !m_masker->mask() || (m_masker->mask()->size() != newSize))
        m_dirty = true;
    SVGStyledLocatableElement::attributeChanged(attr, preserveDecls);
}

void SVGMaskElement::childrenChanged()
{
    m_dirty = true;
    SVGStyledLocatableElement::childrenChanged();
}

void SVGMaskElement::parseMappedAttribute(MappedAttribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::xAttr)
        xBaseValue()->setValueAsString(value);
    else if (attr->name() == SVGNames::yAttr)
        yBaseValue()->setValueAsString(value);
    else if (attr->name() == SVGNames::widthAttr)
        widthBaseValue()->setValueAsString(value);
    else if (attr->name() == SVGNames::heightAttr)
        heightBaseValue()->setValueAsString(value);
    else {
        if (SVGURIReference::parseMappedAttribute(attr))
            return;
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        SVGStyledElement::parseMappedAttribute(attr);
    }
}

KCanvasImage* SVGMaskElement::drawMaskerContent()
{
    KRenderingDevice* device = renderingDevice();
    if (!device->currentContext()) // FIXME: hack for now until Image::lockFocus exists
        return 0;
    if (!renderer())
        return 0;
    KCanvasImage* maskImage = static_cast<KCanvasImage*>(device->createResource(RS_IMAGE));

    IntSize size = IntSize(lroundf(width()->value()), lroundf(height()->value()));
    maskImage->init(size);

    KRenderingDeviceContext* patternContext = device->contextForImage(maskImage);
    device->pushContext(patternContext);

    OwnPtr<GraphicsContext> context(patternContext->createGraphicsContext());

    RenderSVGContainer* maskContainer = static_cast<RenderSVGContainer*>(renderer());
    RenderObject::PaintInfo info(context.get(), IntRect(), PaintPhaseForeground, 0, 0, 0);
    maskContainer->setDrawsContents(true);
    maskContainer->paint(info, 0, 0);
    maskContainer->setDrawsContents(false);
    
    device->popContext();
    delete patternContext;

    return maskImage;
}

RenderObject* SVGMaskElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    RenderSVGContainer* maskContainer = new (arena) RenderSVGContainer(this);
    maskContainer->setDrawsContents(false);
    return maskContainer;
}

KCanvasMasker* SVGMaskElement::canvasResource()
{
    if (!m_masker) {
        m_masker = static_cast<KCanvasMasker*>(renderingDevice()->createResource(RS_MASKER));
        m_dirty = true;
    }
    if (m_dirty) {
        KCanvasImage* newMaskImage = drawMaskerContent();
        m_masker->setMask(newMaskImage);
        m_dirty = (newMaskImage != 0);
    }
    return m_masker;
}

}

// vim:ts=4:noet
#endif // SVG_SUPPORT

