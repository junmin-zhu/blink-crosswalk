/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 */

#include "config.h"
#include "core/css/resolver/AnimatedStyleBuilder.h"

#include "core/animation/AnimatableClipPathOperation.h"
#include "core/animation/AnimatableColor.h"
#include "core/animation/AnimatableDouble.h"
#include "core/animation/AnimatableFilterOperations.h"
#include "core/animation/AnimatableImage.h"
#include "core/animation/AnimatableLength.h"
#include "core/animation/AnimatableLengthBox.h"
#include "core/animation/AnimatableLengthBoxAndBool.h"
#include "core/animation/AnimatableLengthPoint.h"
#include "core/animation/AnimatableLengthSize.h"
#include "core/animation/AnimatableRepeatable.h"
#include "core/animation/AnimatableSVGLength.h"
#include "core/animation/AnimatableSVGPaint.h"
#include "core/animation/AnimatableShadow.h"
#include "core/animation/AnimatableShapeValue.h"
#include "core/animation/AnimatableStrokeDasharrayList.h"
#include "core/animation/AnimatableTransform.h"
#include "core/animation/AnimatableUnknown.h"
#include "core/animation/AnimatableValue.h"
#include "core/animation/AnimatableVisibility.h"
#include "core/animation/css/CSSAnimations.h"
#include "core/css/CSSPrimitiveValueMappings.h"
#include "core/css/resolver/StyleBuilder.h"
#include "core/css/resolver/StyleResolverState.h"
#include "core/rendering/style/RenderStyle.h"
#include "wtf/MathExtras.h"
#include "wtf/TypeTraits.h"

namespace WebCore {

namespace {

Length animatableValueToLength(const AnimatableValue* value, const StyleResolverState& state, NumberRange range = AllValues)
{
    const RenderStyle* style = state.style();
    if (value->isLength())
        return toAnimatableLength(value)->toLength(style, state.rootElementStyle(), style->effectiveZoom(), range);
    RefPtr<CSSValue> cssValue = toAnimatableUnknown(value)->toCSSValue();
    CSSPrimitiveValue* cssPrimitiveValue = toCSSPrimitiveValue(cssValue.get());
    return cssPrimitiveValue->convertToLength<AnyConversion>(style, state.rootElementStyle(), style->effectiveZoom());
}

template<typename T> T animatableValueRoundClampTo(const AnimatableValue* value, T min = defaultMinimumForClamp<T>(), T max = defaultMaximumForClamp<T>())
{
    COMPILE_ASSERT(WTF::IsInteger<T>::value, ShouldUseIntegralTypeTWhenRoundingValues);
    return clampTo<T>(round(toAnimatableDouble(value)->toDouble()), min, max);
}

LengthBox animatableValueToLengthBox(const AnimatableValue* value, const StyleResolverState& state, NumberRange range = AllValues)
{
    const AnimatableLengthBox* animatableLengthBox = toAnimatableLengthBox(value);
    return LengthBox(
        animatableValueToLength(animatableLengthBox->top(), state, range),
        animatableValueToLength(animatableLengthBox->right(), state, range),
        animatableValueToLength(animatableLengthBox->bottom(), state, range),
        animatableValueToLength(animatableLengthBox->left(), state, range));
}

LengthPoint animatableValueToLengthPoint(const AnimatableValue* value, const StyleResolverState& state, NumberRange range = AllValues)
{
    const AnimatableLengthPoint* animatableLengthPoint = toAnimatableLengthPoint(value);
    return LengthPoint(
        animatableValueToLength(animatableLengthPoint->x(), state, range),
        animatableValueToLength(animatableLengthPoint->y(), state, range));
}

LengthSize animatableValueToLengthSize(const AnimatableValue* value, const StyleResolverState& state, NumberRange range)
{
    const AnimatableLengthSize* animatableLengthSize = toAnimatableLengthSize(value);
    return LengthSize(
        animatableValueToLength(animatableLengthSize->width(), state, range),
        animatableValueToLength(animatableLengthSize->height(), state, range));
}

template <CSSPropertyID property>
void setFillSize(FillLayer* fillLayer, const AnimatableValue* value, const StyleResolverState& state)
{
    if (value->isLengthSize())
        fillLayer->setSize(FillSize(SizeLength, animatableValueToLengthSize(value, state, NonNegativeValues)));
    else
        state.styleMap().mapFillSize(property, fillLayer, toAnimatableUnknown(value)->toCSSValue().get());
}

SVGLength animatableValueToNonNegativeSVGLength(const AnimatableValue* value)
{
    SVGLength length = toAnimatableSVGLength(value)->toSVGLength();
    if (length.valueInSpecifiedUnits() < 0)
        length.setValueInSpecifiedUnits(0);
    return length;
}

template <CSSPropertyID property>
void setOnFillLayers(FillLayer* fillLayer, const AnimatableValue* value, StyleResolverState& state)
{
    const Vector<RefPtr<AnimatableValue> >& values = toAnimatableRepeatable(value)->values();
    ASSERT(!values.isEmpty());
    FillLayer* prev = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        if (!fillLayer) {
            switch (property) {
            case CSSPropertyBackgroundImage:
            case CSSPropertyBackgroundPositionX:
            case CSSPropertyBackgroundPositionY:
            case CSSPropertyBackgroundSize:
            case CSSPropertyWebkitBackgroundSize:
                fillLayer = new FillLayer(BackgroundFillLayer);
                break;
            case CSSPropertyWebkitMaskImage:
            case CSSPropertyWebkitMaskPositionX:
            case CSSPropertyWebkitMaskPositionY:
            case CSSPropertyWebkitMaskSize:
                fillLayer = new FillLayer(MaskFillLayer);
                break;
            default:
                ASSERT_NOT_REACHED();
            }
            prev->setNext(fillLayer);
        }
        const AnimatableValue* layerValue = values[i].get();
        switch (property) {
        case CSSPropertyBackgroundImage:
        case CSSPropertyWebkitMaskImage:
            if (layerValue->isImage()) {
                fillLayer->setImage(toAnimatableImage(layerValue)->toStyleImage());
            } else {
                ASSERT(toAnimatableUnknown(layerValue)->toCSSValueID() == CSSValueNone);
                fillLayer->setImage(0);
            }
            break;
        case CSSPropertyBackgroundPositionX:
        case CSSPropertyWebkitMaskPositionX:
            fillLayer->setXPosition(animatableValueToLength(layerValue, state));
            break;
        case CSSPropertyBackgroundPositionY:
        case CSSPropertyWebkitMaskPositionY:
            fillLayer->setYPosition(animatableValueToLength(layerValue, state));
            break;
        case CSSPropertyBackgroundSize:
        case CSSPropertyWebkitBackgroundSize:
        case CSSPropertyWebkitMaskSize:
            setFillSize<property>(fillLayer, layerValue, state);
            break;
        default:
            ASSERT_NOT_REACHED();
        }
        prev = fillLayer;
        fillLayer = fillLayer->next();
    }
    while (fillLayer) {
        switch (property) {
        case CSSPropertyBackgroundImage:
        case CSSPropertyWebkitMaskImage:
            fillLayer->clearImage();
            break;
        case CSSPropertyBackgroundPositionX:
        case CSSPropertyWebkitMaskPositionX:
            fillLayer->clearXPosition();
            break;
        case CSSPropertyBackgroundPositionY:
        case CSSPropertyWebkitMaskPositionY:
            fillLayer->clearYPosition();
            break;
        case CSSPropertyBackgroundSize:
        case CSSPropertyWebkitBackgroundSize:
        case CSSPropertyWebkitMaskSize:
            fillLayer->clearSize();
            break;
        default:
            ASSERT_NOT_REACHED();
        }
        fillLayer = fillLayer->next();
    }
}

} // namespace

// FIXME: Generate this function.
void AnimatedStyleBuilder::applyProperty(CSSPropertyID property, StyleResolverState& state, const AnimatableValue* value)
{
    if (value->isUnknown()) {
        StyleBuilder::applyProperty(property, state, toAnimatableUnknown(value)->toCSSValue().get());
        return;
    }
    RenderStyle* style = state.style();
    switch (property) {
    case CSSPropertyBackgroundColor:
        style->setBackgroundColor(toAnimatableColor(value)->color());
        style->setVisitedLinkBackgroundColor(toAnimatableColor(value)->visitedLinkColor());
        return;
    case CSSPropertyBackgroundImage:
        setOnFillLayers<CSSPropertyBackgroundImage>(style->accessBackgroundLayers(), value, state);
        return;
    case CSSPropertyBackgroundPositionX:
        setOnFillLayers<CSSPropertyBackgroundPositionX>(style->accessBackgroundLayers(), value, state);
        return;
    case CSSPropertyBackgroundPositionY:
        setOnFillLayers<CSSPropertyBackgroundPositionY>(style->accessBackgroundLayers(), value, state);
        return;
    case CSSPropertyBackgroundSize:
        setOnFillLayers<CSSPropertyBackgroundSize>(style->accessBackgroundLayers(), value, state);
        return;
    case CSSPropertyBaselineShift:
        style->setBaselineShiftValue(toAnimatableSVGLength(value)->toSVGLength());
        return;
    case CSSPropertyBorderBottomColor:
        style->setBorderBottomColor(toAnimatableColor(value)->color());
        style->setVisitedLinkBorderBottomColor(toAnimatableColor(value)->visitedLinkColor());
        return;
    case CSSPropertyBorderBottomLeftRadius:
        style->setBorderBottomLeftRadius(animatableValueToLengthSize(value, state, NonNegativeValues));
        return;
    case CSSPropertyBorderBottomRightRadius:
        style->setBorderBottomRightRadius(animatableValueToLengthSize(value, state, NonNegativeValues));
        return;
    case CSSPropertyBorderBottomWidth:
        style->setBorderBottomWidth(animatableValueRoundClampTo<unsigned>(value));
        return;
    case CSSPropertyBorderImageOutset:
        style->setBorderImageOutset(animatableValueToLengthBox(value, state, NonNegativeValues));
        return;
    case CSSPropertyBorderImageSlice:
        style->setBorderImageSlices(animatableValueToLengthBox(value, state, NonNegativeValues));
        return;
    case CSSPropertyBorderImageSource:
        style->setBorderImageSource(toAnimatableImage(value)->toStyleImage());
        return;
    case CSSPropertyBorderImageWidth:
        style->setBorderImageWidth(animatableValueToLengthBox(value, state, NonNegativeValues));
        return;
    case CSSPropertyBorderLeftColor:
        style->setBorderLeftColor(toAnimatableColor(value)->color());
        style->setVisitedLinkBorderLeftColor(toAnimatableColor(value)->visitedLinkColor());
        return;
    case CSSPropertyBorderLeftWidth:
        style->setBorderLeftWidth(animatableValueRoundClampTo<unsigned>(value));
        return;
    case CSSPropertyBorderRightColor:
        style->setBorderRightColor(toAnimatableColor(value)->color());
        style->setVisitedLinkBorderRightColor(toAnimatableColor(value)->visitedLinkColor());
        return;
    case CSSPropertyBorderRightWidth:
        style->setBorderRightWidth(animatableValueRoundClampTo<unsigned>(value));
        return;
    case CSSPropertyBorderTopColor:
        style->setBorderTopColor(toAnimatableColor(value)->color());
        style->setVisitedLinkBorderTopColor(toAnimatableColor(value)->visitedLinkColor());
        return;
    case CSSPropertyBorderTopLeftRadius:
        style->setBorderTopLeftRadius(animatableValueToLengthSize(value, state, NonNegativeValues));
        return;
    case CSSPropertyBorderTopRightRadius:
        style->setBorderTopRightRadius(animatableValueToLengthSize(value, state, NonNegativeValues));
        return;
    case CSSPropertyBorderTopWidth:
        style->setBorderTopWidth(animatableValueRoundClampTo<unsigned>(value));
        return;
    case CSSPropertyBottom:
        style->setBottom(animatableValueToLength(value, state));
        return;
    case CSSPropertyBoxShadow:
    case CSSPropertyWebkitBoxShadow:
        style->setBoxShadow(toAnimatableShadow(value)->shadowList());
        return;
    case CSSPropertyClip:
        style->setClip(animatableValueToLengthBox(value, state));
        style->setHasClip(true);
        return;
    case CSSPropertyColor:
        style->setColor(toAnimatableColor(value)->color());
        style->setVisitedLinkColor(toAnimatableColor(value)->visitedLinkColor());
        return;
    case CSSPropertyFillOpacity:
        style->setFillOpacity(clampTo<float>(toAnimatableDouble(value)->toDouble(), 0, 1));
        return;
    case CSSPropertyFill:
        {
            const AnimatableSVGPaint* svgPaint = toAnimatableSVGPaint(value);
            style->accessSVGStyle()->setFillPaint(svgPaint->paintType(), svgPaint->color(), svgPaint->uri());
        }
        return;
    case CSSPropertyFlexGrow:
        style->setFlexGrow(clampTo<float>(toAnimatableDouble(value)->toDouble(), 0));
        return;
    case CSSPropertyFlexShrink:
        style->setFlexShrink(clampTo<float>(toAnimatableDouble(value)->toDouble(), 0));
        return;
    case CSSPropertyFlexBasis:
        style->setFlexBasis(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyFloodColor:
        style->setFloodColor(toAnimatableColor(value)->color());
        return;
    case CSSPropertyFloodOpacity:
        style->setFloodOpacity(clampTo<float>(toAnimatableDouble(value)->toDouble(), 0, 1));
        return;
    case CSSPropertyFontSize:
        style->setFontSize(clampTo<float>(toAnimatableDouble(value)->toDouble(), 0));
        return;
    case CSSPropertyHeight:
        style->setHeight(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyKerning:
        style->setKerning(toAnimatableSVGLength(value)->toSVGLength());
        return;
    case CSSPropertyLeft:
        style->setLeft(animatableValueToLength(value, state));
        return;
    case CSSPropertyLightingColor:
        style->setLightingColor(toAnimatableColor(value)->color());
        return;
    case CSSPropertyLineHeight:
        if (value->isLength())
            style->setLineHeight(animatableValueToLength(value, state, NonNegativeValues));
        else
            style->setLineHeight(Length(clampTo<float>(toAnimatableDouble(value)->toDouble(), 0), Percent));
        return;
    case CSSPropertyListStyleImage:
        style->setListStyleImage(toAnimatableImage(value)->toStyleImage());
        return;
    case CSSPropertyLetterSpacing:
        style->setLetterSpacing(clampTo<float>(toAnimatableDouble(value)->toDouble()));
        return;
    case CSSPropertyMarginBottom:
        style->setMarginBottom(animatableValueToLength(value, state));
        return;
    case CSSPropertyMarginLeft:
        style->setMarginLeft(animatableValueToLength(value, state));
        return;
    case CSSPropertyMarginRight:
        style->setMarginRight(animatableValueToLength(value, state));
        return;
    case CSSPropertyMarginTop:
        style->setMarginTop(animatableValueToLength(value, state));
        return;
    case CSSPropertyMaxHeight:
        style->setMaxHeight(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyMaxWidth:
        style->setMaxWidth(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyMinHeight:
        style->setMinHeight(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyMinWidth:
        style->setMinWidth(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyObjectPosition:
        style->setObjectPosition(animatableValueToLengthPoint(value, state));
        return;
    case CSSPropertyOpacity:
        style->setOpacity(toAnimatableDouble(value)->toDouble());
        return;
    case CSSPropertyOrphans:
        style->setOrphans(animatableValueRoundClampTo<unsigned short>(value, 1));
        return;
    case CSSPropertyOutlineColor:
        style->setOutlineColor(toAnimatableColor(value)->color());
        style->setVisitedLinkOutlineColor(toAnimatableColor(value)->visitedLinkColor());
        return;
    case CSSPropertyOutlineOffset:
        style->setOutlineOffset(animatableValueRoundClampTo<int>(value));
        return;
    case CSSPropertyOutlineWidth:
        style->setOutlineWidth(animatableValueRoundClampTo<unsigned short>(value));
        return;
    case CSSPropertyPaddingBottom:
        style->setPaddingBottom(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyPaddingLeft:
        style->setPaddingLeft(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyPaddingRight:
        style->setPaddingRight(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyPaddingTop:
        style->setPaddingTop(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyRight:
        style->setRight(animatableValueToLength(value, state));
        return;
    case CSSPropertyStrokeWidth:
        style->setStrokeWidth(animatableValueToNonNegativeSVGLength(value));
        return;
    case CSSPropertyStopColor:
        style->setStopColor(toAnimatableColor(value)->color());
        return;
    case CSSPropertyStopOpacity:
        style->setStopOpacity(clampTo<float>(toAnimatableDouble(value)->toDouble(), 0, 1));
        return;
    case CSSPropertyStrokeDasharray:
        style->setStrokeDashArray(toAnimatableStrokeDasharrayList(value)->toSVGLengthVector());
        return;
    case CSSPropertyStrokeDashoffset:
        style->setStrokeDashOffset(toAnimatableSVGLength(value)->toSVGLength());
        return;
    case CSSPropertyStrokeMiterlimit:
        style->setStrokeMiterLimit(clampTo<float>(toAnimatableDouble(value)->toDouble(), 1));
        return;
    case CSSPropertyStrokeOpacity:
        style->setStrokeOpacity(clampTo<float>(toAnimatableDouble(value)->toDouble(), 0, 1));
        return;
    case CSSPropertyStroke:
        {
            const AnimatableSVGPaint* svgPaint = toAnimatableSVGPaint(value);
            style->accessSVGStyle()->setStrokePaint(svgPaint->paintType(), svgPaint->color(), svgPaint->uri());
        }
        return;
    case CSSPropertyTextDecorationColor:
        style->setTextDecorationColor(toAnimatableColor(value)->color());
        style->setVisitedLinkTextDecorationColor(toAnimatableColor(value)->visitedLinkColor());
        return;
    case CSSPropertyTextIndent:
        style->setTextIndent(animatableValueToLength(value, state));
        return;
    case CSSPropertyTextShadow:
        style->setTextShadow(toAnimatableShadow(value)->shadowList());
        return;
    case CSSPropertyTop:
        style->setTop(animatableValueToLength(value, state));
        return;
    case CSSPropertyWebkitBackgroundSize:
        setOnFillLayers<CSSPropertyWebkitBackgroundSize>(style->accessBackgroundLayers(), value, state);
        return;
    case CSSPropertyWebkitBorderHorizontalSpacing:
        style->setHorizontalBorderSpacing(animatableValueRoundClampTo<unsigned short>(value));
        return;
    case CSSPropertyWebkitBorderVerticalSpacing:
        style->setVerticalBorderSpacing(animatableValueRoundClampTo<unsigned short>(value));
        return;
    case CSSPropertyWebkitClipPath:
        style->setClipPath(toAnimatableClipPathOperation(value)->clipPathOperation());
        return;
    case CSSPropertyWebkitColumnCount:
        style->setColumnCount(animatableValueRoundClampTo<unsigned short>(value, 1));
        return;
    case CSSPropertyWebkitColumnGap:
        style->setColumnGap(clampTo(toAnimatableDouble(value)->toDouble(), 0));
        return;
    case CSSPropertyWebkitColumnRuleColor:
        style->setColumnRuleColor(toAnimatableColor(value)->color());
        style->setVisitedLinkColumnRuleColor(toAnimatableColor(value)->visitedLinkColor());
        return;
    case CSSPropertyWebkitColumnWidth:
        style->setColumnWidth(clampTo(toAnimatableDouble(value)->toDouble(), std::numeric_limits<float>::epsilon()));
        return;
    case CSSPropertyWebkitColumnRuleWidth:
        style->setColumnRuleWidth(animatableValueRoundClampTo<unsigned short>(value));
        return;
    case CSSPropertyWebkitFilter:
        style->setFilter(toAnimatableFilterOperations(value)->operations());
        return;
    case CSSPropertyWebkitMaskBoxImageOutset:
        style->setMaskBoxImageOutset(animatableValueToLengthBox(value, state, NonNegativeValues));
        return;
    case CSSPropertyWebkitMaskBoxImageSlice:
        style->setMaskBoxImageSlices(animatableValueToLengthBox(toAnimatableLengthBoxAndBool(value)->box(), state, NonNegativeValues));
        style->setMaskBoxImageSlicesFill(toAnimatableLengthBoxAndBool(value)->flag());
        return;
    case CSSPropertyWebkitMaskBoxImageSource:
        style->setMaskBoxImageSource(toAnimatableImage(value)->toStyleImage());
        return;
    case CSSPropertyWebkitMaskBoxImageWidth:
        style->setMaskBoxImageWidth(animatableValueToLengthBox(value, state, NonNegativeValues));
        return;
    case CSSPropertyWebkitMaskImage:
        setOnFillLayers<CSSPropertyWebkitMaskImage>(style->accessMaskLayers(), value, state);
        return;
    case CSSPropertyWebkitMaskPositionX:
        setOnFillLayers<CSSPropertyWebkitMaskPositionX>(style->accessMaskLayers(), value, state);
        return;
    case CSSPropertyWebkitMaskPositionY:
        setOnFillLayers<CSSPropertyWebkitMaskPositionY>(style->accessMaskLayers(), value, state);
        return;
    case CSSPropertyWebkitMaskSize:
        setOnFillLayers<CSSPropertyWebkitMaskSize>(style->accessMaskLayers(), value, state);
        return;
    case CSSPropertyWebkitPerspective:
        style->setPerspective(clampTo<float>(toAnimatableDouble(value)->toDouble()));
        return;
    case CSSPropertyWebkitPerspectiveOriginX:
        style->setPerspectiveOriginX(animatableValueToLength(value, state));
        return;
    case CSSPropertyWebkitPerspectiveOriginY:
        style->setPerspectiveOriginY(animatableValueToLength(value, state));
        return;
    case CSSPropertyShapeInside:
        style->setShapeInside(toAnimatableShapeValue(value)->shapeValue());
        return;
    case CSSPropertyShapeOutside:
        style->setShapeOutside(toAnimatableShapeValue(value)->shapeValue());
        return;
    case CSSPropertyShapeMargin:
        style->setShapeMargin(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyWebkitTextStrokeColor:
        style->setTextStrokeColor(toAnimatableColor(value)->color());
        style->setVisitedLinkTextStrokeColor(toAnimatableColor(value)->visitedLinkColor());
        return;
    case CSSPropertyWebkitTransform:
        style->setTransform(toAnimatableTransform(value)->transformOperations());
        return;
    case CSSPropertyWebkitTransformOriginX:
        style->setTransformOriginX(animatableValueToLength(value, state));
        return;
    case CSSPropertyWebkitTransformOriginY:
        style->setTransformOriginY(animatableValueToLength(value, state));
        return;
    case CSSPropertyWebkitTransformOriginZ:
        style->setTransformOriginZ(toAnimatableDouble(value)->toDouble());
        return;
    case CSSPropertyWidows:
        style->setWidows(animatableValueRoundClampTo<unsigned short>(value, 1));
        return;
    case CSSPropertyWidth:
        style->setWidth(animatableValueToLength(value, state, NonNegativeValues));
        return;
    case CSSPropertyWordSpacing:
        style->setWordSpacing(clampTo<float>(toAnimatableDouble(value)->toDouble()));
        return;
    case CSSPropertyVisibility:
        style->setVisibility(toAnimatableVisibility(value)->visibility());
        return;
    case CSSPropertyZIndex:
        style->setZIndex(animatableValueRoundClampTo<int>(value));
        return;
    case CSSPropertyZoom:
        style->setZoom(clampTo<float>(toAnimatableDouble(value)->toDouble(), std::numeric_limits<float>::denorm_min()));
        return;
    default:
        RELEASE_ASSERT_WITH_MESSAGE(!CSSAnimations::isAnimatableProperty(property), "Web Animations not yet implemented: Unable to apply AnimatableValue to RenderStyle: %s", getPropertyNameString(property).utf8().data());
        ASSERT_NOT_REACHED();
    }
}

} // namespace WebCore
