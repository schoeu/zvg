/*
 * Copyright (c) 2024 - 2026 ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"
#include <thorvg.h>.7476027996952986632:20d4500cb2bffcabcf5ba56c1a7e95bd_69800572fa3185c9a0748b95.6980a98afa3185c9a074be0a.6980a98a4457231bb6219ad4:Trae.T(2026/2/2 21:41:30)
#ifdef THORVG_LOTTIE_LOADER_SUPPORT
#include <thorvg_lottie.h>
#endif
#include <fstream>
#include <cstring>
#include <vector>
#include <cmath>
#include "src/loaders/lottie/tvgLottieModel.h"
#include "src/loaders/lottie/tvgLottieParser.h"
#include "src/loaders/lottie/tvgLottieFlatBufferParser.h"
#include "catch.hpp"

using namespace tvg;
using namespace std;

#ifdef THORVG_LOTTIE_LOADER_SUPPORT

static bool eqString(const char* a, const char* b)
{
    if (!a || !b) return a == b;
    return strcmp(a, b) == 0;
}

static bool eqFloat(float a, float b, float eps = 0.0001f)
{
    return fabsf(a - b) <= eps;
}

static bool eqPoint(const Point& a, const Point& b)
{
    return eqFloat(a.x, b.x) && eqFloat(a.y, b.y);
}

static bool eqRGB(const RGB32& a, const RGB32& b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

static bool eqColorStop(const Fill::ColorStop& a, const Fill::ColorStop& b)
{
    return eqFloat(a.offset, b.offset) && a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static bool eqInterpolator(const LottieInterpolator* a, const LottieInterpolator* b)
{
    if (!a || !b) return a == b;
    if (!eqPoint(a->inTangent, b->inTangent)) return false;
    if (!eqPoint(a->outTangent, b->outTangent)) return false;
    return eqString(a->key, b->key);
}

static bool eqExpression(const LottieExpression* a, const LottieExpression* b)
{
    if (!a || !b) return a == b;
    if (!eqString(a->code, b->code)) return false;
    if (a->disabled != b->disabled) return false;
    if (a->writables.count != b->writables.count) return false;
    for (uint32_t i = 0; i < a->writables.count; ++i) {
        if (!eqString(a->writables[i].var, b->writables[i].var)) return false;
        if (!eqFloat(a->writables[i].val, b->writables[i].val)) return false;
    }
    return true;
}

static bool eqTextDocument(const TextDocument& a, const TextDocument& b)
{
    if (!eqString(a.text, b.text)) return false;
    if (!eqFloat(a.height, b.height)) return false;
    if (!eqFloat(a.shift, b.shift)) return false;
    if (!eqRGB(a.color, b.color)) return false;
    if (!eqPoint(a.bbox.pos, b.bbox.pos)) return false;
    if (!eqPoint(a.bbox.size, b.bbox.size)) return false;
    if (!eqRGB(a.stroke.color, b.stroke.color)) return false;
    if (!eqFloat(a.stroke.width, b.stroke.width)) return false;
    if (a.stroke.below != b.stroke.below) return false;
    if (!eqString(a.name, b.name)) return false;
    if (!eqFloat(a.size, b.size)) return false;
    if (!eqFloat(a.tracking, b.tracking)) return false;
    if (!eqFloat(a.justify, b.justify)) return false;
    if (a.caps != b.caps) return false;
    return true;
}

static bool eqPathSet(const PathSet& a, const PathSet& b)
{
    if (a.ptsCnt != b.ptsCnt) return false;
    if (a.cmdsCnt != b.cmdsCnt) return false;
    for (uint16_t i = 0; i < a.cmdsCnt; ++i) {
        if (a.cmds[i] != b.cmds[i]) return false;
    }
    for (uint16_t i = 0; i < a.ptsCnt; ++i) {
        if (!eqPoint(a.pts[i], b.pts[i])) return false;
    }
    return true;
}

template<typename T>
static bool eqScalarFrame(const LottieScalarFrame<T>& a, const LottieScalarFrame<T>& b)
{
    if (!eqFloat(a.no, b.no)) return false;
    if (a.hold != b.hold) return false;
    if (!eqInterpolator(a.interpolator, b.interpolator)) return false;
    return true;
}

static bool eqScalarFrameValue(const LottieScalarFrame<float>& a, const LottieScalarFrame<float>& b)
{
    if (!eqScalarFrame(a, b)) return false;
    return eqFloat(a.value, b.value);
}

static bool eqScalarFrameValue(const LottieScalarFrame<int8_t>& a, const LottieScalarFrame<int8_t>& b)
{
    if (!eqScalarFrame(a, b)) return false;
    return a.value == b.value;
}

static bool eqScalarFrameValue(const LottieScalarFrame<uint8_t>& a, const LottieScalarFrame<uint8_t>& b)
{
    if (!eqScalarFrame(a, b)) return false;
    return a.value == b.value;
}

static bool eqScalarFrameValue(const LottieScalarFrame<RGB32>& a, const LottieScalarFrame<RGB32>& b)
{
    if (!eqScalarFrame(a, b)) return false;
    return eqRGB(a.value, b.value);
}

static bool eqScalarFrameValue(const LottieScalarFrame<Point>& a, const LottieScalarFrame<Point>& b)
{
    if (!eqScalarFrame(a, b)) return false;
    return eqPoint(a.value, b.value);
}

static bool eqVectorFrameValue(const LottieVectorFrame<Point>& a, const LottieVectorFrame<Point>& b)
{
    if (!eqFloat(a.no, b.no)) return false;
    if (a.hold != b.hold) return false;
    if (!eqInterpolator(a.interpolator, b.interpolator)) return false;
    if (!eqPoint(a.value, b.value)) return false;
    if (!eqPoint(a.outTangent, b.outTangent)) return false;
    if (!eqPoint(a.inTangent, b.inTangent)) return false;
    if (a.hasTangent != b.hasTangent) return false;
    if (!eqFloat(a.length, b.length)) return false;
    return true;
}

template<typename Frame, typename Value, LottieProperty::Type PType, bool Scalar>
static bool eqGenericProperty(const LottieGenericProperty<Frame, Value, PType, Scalar>& a, const LottieGenericProperty<Frame, Value, PType, Scalar>& b)
{
    if (a.ix != b.ix) return false;
    if (!eqExpression(a.exp, b.exp)) return false;
    if (!a.frames || !b.frames) {
        if (a.frames || b.frames) return false;
    } else {
        if (a.frames->count != b.frames->count) return false;
    }
    return true;
}

static bool eqLottieFloat(const LottieFloat& a, const LottieFloat& b)
{
    if (!eqGenericProperty(a, b)) return false;
    if (!a.frames) return eqFloat(a.value, b.value);
    for (uint32_t i = 0; i < a.frames->count; ++i) {
        if (!eqScalarFrameValue((*a.frames)[i], (*b.frames)[i])) return false;
    }
    return true;
}

static bool eqLottieInteger(const LottieInteger& a, const LottieInteger& b)
{
    if (!eqGenericProperty(a, b)) return false;
    if (!a.frames) return a.value == b.value;
    for (uint32_t i = 0; i < a.frames->count; ++i) {
        if (!eqScalarFrameValue((*a.frames)[i], (*b.frames)[i])) return false;
    }
    return true;
}

static bool eqLottieScalar(const LottieScalar& a, const LottieScalar& b)
{
    if (!eqGenericProperty(a, b)) return false;
    if (!a.frames) return eqPoint(a.value, b.value);
    for (uint32_t i = 0; i < a.frames->count; ++i) {
        if (!eqScalarFrameValue((*a.frames)[i], (*b.frames)[i])) return false;
    }
    return true;
}

static bool eqLottieVector(const LottieVector& a, const LottieVector& b)
{
    if (!eqGenericProperty(a, b)) return false;
    if (!a.frames) return eqPoint(a.value, b.value);
    for (uint32_t i = 0; i < a.frames->count; ++i) {
        if (!eqVectorFrameValue((*a.frames)[i], (*b.frames)[i])) return false;
    }
    return true;
}

static bool eqLottieColor(const LottieColor& a, const LottieColor& b)
{
    if (!eqGenericProperty(a, b)) return false;
    if (!a.frames) return eqRGB(a.value, b.value);
    for (uint32_t i = 0; i < a.frames->count; ++i) {
        if (!eqScalarFrameValue((*a.frames)[i], (*b.frames)[i])) return false;
    }
    return true;
}

static bool eqLottieOpacity(const LottieOpacity& a, const LottieOpacity& b)
{
    if (!eqGenericProperty(a, b)) return false;
    if (!a.frames) return a.value == b.value;
    for (uint32_t i = 0; i < a.frames->count; ++i) {
        if (!eqScalarFrameValue((*a.frames)[i], (*b.frames)[i])) return false;
    }
    return true;
}

static bool eqPathFrameValue(const LottieScalarFrame<PathSet>& a, const LottieScalarFrame<PathSet>& b)
{
    if (!eqScalarFrame(a, b)) return false;
    return eqPathSet(a.value, b.value);
}

static bool eqColorStopData(const ColorStop& a, const ColorStop& b, uint16_t count)
{
    if (!a.data || !b.data) return a.data == b.data;
    for (uint16_t i = 0; i < count; ++i) {
        if (!eqColorStop(a.data[i], b.data[i])) return false;
    }
    if (!a.input || !b.input) return a.input == b.input;
    if (a.input->count != b.input->count) return false;
    for (uint32_t i = 0; i < a.input->count; ++i) {
        if (!eqFloat((*a.input)[i], (*b.input)[i])) return false;
    }
    return true;
}

static bool eqColorStopFrameValue(const LottieScalarFrame<ColorStop>& a, const LottieScalarFrame<ColorStop>& b, uint16_t count)
{
    if (!eqScalarFrame(a, b)) return false;
    return eqColorStopData(a.value, b.value, count);
}

static bool eqLottiePathSet(const LottiePathSet& a, const LottiePathSet& b)
{
    if (a.ix != b.ix) return false;
    if (!eqExpression(a.exp, b.exp)) return false;
    if (!eqPathSet(a.value, b.value)) return false;
    if (!a.frames || !b.frames) return a.frames == b.frames;
    if (a.frames->count != b.frames->count) return false;
    for (uint32_t i = 0; i < a.frames->count; ++i) {
        if (!eqPathFrameValue((*a.frames)[i], (*b.frames)[i])) return false;
    }
    return true;
}

static bool eqLottieColorStop(const LottieColorStop& a, const LottieColorStop& b)
{
    if (a.ix != b.ix) return false;
    if (!eqExpression(a.exp, b.exp)) return false;
    if (a.count != b.count) return false;
    if (a.populated != b.populated) return false;
    if (!eqColorStopData(a.value, b.value, a.count)) return false;
    if (!a.frames || !b.frames) return a.frames == b.frames;
    if (a.frames->count != b.frames->count) return false;
    for (uint32_t i = 0; i < a.frames->count; ++i) {
        if (!eqColorStopFrameValue((*a.frames)[i], (*b.frames)[i], a.count)) return false;
    }
    return true;
}

static bool eqLottieTextDoc(const LottieTextDoc& a, const LottieTextDoc& b)
{
    if (a.ix != b.ix) return false;
    if (!eqExpression(a.exp, b.exp)) return false;
    if (!a.frames || !b.frames) {
        if (a.frames || b.frames) return false;
        return eqTextDocument(a.value, b.value);
    }
    if (a.frames->count != b.frames->count) return false;
    for (uint32_t i = 0; i < a.frames->count; ++i) {
        if (!eqScalarFrame(a.frames->data[i], b.frames->data[i])) return false;
        if (!eqTextDocument(a.frames->data[i].value, b.frames->data[i].value)) return false;
    }
    return true;
}

static bool eqLottieBitmap(const LottieBitmap& a, const LottieBitmap& b)
{
    if (a.ix != b.ix) return false;
    if (!eqExpression(a.exp, b.exp)) return false;
    if (a.size != b.size) return false;
    if (!eqFloat(a.width, b.width)) return false;
    if (!eqFloat(a.height, b.height)) return false;
    if (!eqString(a.mimeType, b.mimeType)) return false;
    if ((a.picture == nullptr) != (b.picture == nullptr)) return false;
    if (a.size > 0) {
        if (!a.data || !b.data) return a.data == b.data;
        return memcmp(a.data, b.data, a.size) == 0;
    }
    return eqString(a.data, b.data);
}

static bool eqProperty(const LottieProperty* a, const LottieProperty* b)
{
    if (!a || !b) return a == b;
    if (a->type != b->type) return false;
    if (a->ix != b->ix) return false;
    switch (a->type) {
        case LottieProperty::Type::Float:
            return eqLottieFloat(*static_cast<const LottieFloat*>(a), *static_cast<const LottieFloat*>(b));
        case LottieProperty::Type::Integer:
            return eqLottieInteger(*static_cast<const LottieInteger*>(a), *static_cast<const LottieInteger*>(b));
        case LottieProperty::Type::Scalar:
            return eqLottieScalar(*static_cast<const LottieScalar*>(a), *static_cast<const LottieScalar*>(b));
        case LottieProperty::Type::Vector:
            return eqLottieVector(*static_cast<const LottieVector*>(a), *static_cast<const LottieVector*>(b));
        case LottieProperty::Type::Color:
            return eqLottieColor(*static_cast<const LottieColor*>(a), *static_cast<const LottieColor*>(b));
        case LottieProperty::Type::Opacity:
            return eqLottieOpacity(*static_cast<const LottieOpacity*>(a), *static_cast<const LottieOpacity*>(b));
        case LottieProperty::Type::PathSet:
            return eqLottiePathSet(*static_cast<const LottiePathSet*>(a), *static_cast<const LottiePathSet*>(b));
        case LottieProperty::Type::ColorStop:
            return eqLottieColorStop(*static_cast<const LottieColorStop*>(a), *static_cast<const LottieColorStop*>(b));
        case LottieProperty::Type::TextDoc:
            return eqLottieTextDoc(*static_cast<const LottieTextDoc*>(a), *static_cast<const LottieTextDoc*>(b));
        case LottieProperty::Type::Image:
            return eqLottieBitmap(*static_cast<const LottieBitmap*>(a), *static_cast<const LottieBitmap*>(b));
        default:
            return false;
    }
}

static bool eqDashAttr(const LottieStroke::DashAttr* a, const LottieStroke::DashAttr* b)
{
    if (!a || !b) return a == b;
    if (a->size != b->size) return false;
    if (a->allocated != b->allocated) return false;
    if (!eqLottieFloat(a->offset, b->offset)) return false;
    for (uint8_t i = 0; i < a->size; ++i) {
        if (!eqLottieFloat(a->values[i], b->values[i])) return false;
    }
    return true;
}

static bool eqStroke(const LottieStroke* a, const LottieStroke* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieFloat(a->width, b->width)) return false;
    if (!eqDashAttr(a->dashattr, b->dashattr)) return false;
    if (!eqFloat(a->miterLimit, b->miterLimit)) return false;
    if (a->cap != b->cap) return false;
    if (a->join != b->join) return false;
    return true;
}

static bool eqMask(const LottieMask* a, const LottieMask* b)
{
    if (!a || !b) return a == b;
    if (!eqLottiePathSet(a->pathset, b->pathset)) return false;
    if (!eqLottieFloat(a->expand, b->expand)) return false;
    if (!eqLottieOpacity(a->opacity, b->opacity)) return false;
    if (a->method != b->method) return false;
    if (a->inverse != b->inverse) return false;
    return true;
}

static bool eqEffect(const LottieEffect* a, const LottieEffect* b);

static bool eqFxCustom(const LottieFxCustom* a, const LottieFxCustom* b)
{
    if (a->props.count != b->props.count) return false;
    if (!eqString(a->name, b->name)) return false;
    for (uint32_t i = 0; i < a->props.count; ++i) {
        if (a->props[i].nm != b->props[i].nm) return false;
        if (a->props[i].mn != b->props[i].mn) return false;
        if (!eqProperty(a->props[i].property, b->props[i].property)) return false;
    }
    return true;
}

static bool eqFxFill(const LottieFxFill* a, const LottieFxFill* b)
{
    return eqLottieColor(a->color, b->color) && eqLottieFloat(a->opacity, b->opacity);
}

static bool eqFxStroke(const LottieFxStroke* a, const LottieFxStroke* b)
{
    return eqLottieInteger(a->mask, b->mask) && eqLottieInteger(a->allMask, b->allMask) && eqLottieColor(a->color, b->color) && eqLottieFloat(a->size, b->size) && eqLottieFloat(a->opacity, b->opacity) && eqLottieFloat(a->begin, b->begin) && eqLottieFloat(a->end, b->end);
}

static bool eqFxTint(const LottieFxTint* a, const LottieFxTint* b)
{
    return eqLottieColor(a->black, b->black) && eqLottieColor(a->white, b->white) && eqLottieFloat(a->intensity, b->intensity);
}

static bool eqFxTritone(const LottieFxTritone* a, const LottieFxTritone* b)
{
    return eqLottieColor(a->bright, b->bright) && eqLottieColor(a->midtone, b->midtone) && eqLottieColor(a->dark, b->dark) && eqLottieOpacity(a->blend, b->blend);
}

static bool eqFxDropShadow(const LottieFxDropShadow* a, const LottieFxDropShadow* b)
{
    return eqLottieColor(a->color, b->color) && eqLottieFloat(a->opacity, b->opacity) && eqLottieFloat(a->angle, b->angle) && eqLottieFloat(a->distance, b->distance) && eqLottieFloat(a->blurness, b->blurness);
}

static bool eqFxGaussianBlur(const LottieFxGaussianBlur* a, const LottieFxGaussianBlur* b)
{
    return eqLottieFloat(a->blurness, b->blurness) && eqLottieInteger(a->direction, b->direction) && eqLottieInteger(a->wrap, b->wrap);
}

static bool eqEffect(const LottieEffect* a, const LottieEffect* b)
{
    if (!a || !b) return a == b;
    if (a->type != b->type) return false;
    if (a->ix != b->ix) return false;
    if (a->nm != b->nm) return false;
    if (a->mn != b->mn) return false;
    if (a->enable != b->enable) return false;
    switch (a->type) {
        case LottieEffect::Custom:
            return eqFxCustom(static_cast<const LottieFxCustom*>(a), static_cast<const LottieFxCustom*>(b));
        case LottieEffect::Tint:
            return eqFxTint(static_cast<const LottieFxTint*>(a), static_cast<const LottieFxTint*>(b));
        case LottieEffect::Fill:
            return eqFxFill(static_cast<const LottieFxFill*>(a), static_cast<const LottieFxFill*>(b));
        case LottieEffect::Stroke:
            return eqFxStroke(static_cast<const LottieFxStroke*>(a), static_cast<const LottieFxStroke*>(b));
        case LottieEffect::Tritone:
            return eqFxTritone(static_cast<const LottieFxTritone*>(a), static_cast<const LottieFxTritone*>(b));
        case LottieEffect::DropShadow:
            return eqFxDropShadow(static_cast<const LottieFxDropShadow*>(a), static_cast<const LottieFxDropShadow*>(b));
        case LottieEffect::GaussianBlur:
            return eqFxGaussianBlur(static_cast<const LottieFxGaussianBlur*>(a), static_cast<const LottieFxGaussianBlur*>(b));
        default:
            return false;
    }
}

static bool eqGlyph(const LottieGlyph* a, const LottieGlyph* b);
static bool eqObject(const LottieObject* a, const LottieObject* b);

static bool eqGlyph(const LottieGlyph* a, const LottieGlyph* b)
{
    if (!a || !b) return a == b;
    if (!eqString(a->code, b->code)) return false;
    if (!eqString(a->family, b->family)) return false;
    if (!eqString(a->style, b->style)) return false;
    if (a->size != b->size) return false;
    if (!eqFloat(a->width, b->width)) return false;
    if (a->len != b->len) return false;
    if (a->children.count != b->children.count) return false;
    for (uint32_t i = 0; i < a->children.count; ++i) {
        if (!eqObject(a->children[i], b->children[i])) return false;
    }
    return true;
}

static bool eqFont(const LottieFont* a, const LottieFont* b)
{
    if (!a || !b) return a == b;
    if (!eqString(a->name, b->name)) return false;
    if (!eqString(a->family, b->family)) return false;
    if (!eqString(a->style, b->style)) return false;
    if (!eqString(a->b64src, b->b64src)) return false;
    if (a->size != b->size) return false;
    if (!eqFloat(a->ascent, b->ascent)) return false;
    if (a->origin != b->origin) return false;
    if (a->chars.count != b->chars.count) return false;
    for (uint32_t i = 0; i < a->chars.count; ++i) {
        if (!eqGlyph(a->chars[i], b->chars[i])) return false;
    }
    return true;
}

static bool eqTextRange(const LottieTextRange* a, const LottieTextRange* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieColor(a->style.fillColor, b->style.fillColor)) return false;
    if (!eqLottieColor(a->style.strokeColor, b->style.strokeColor)) return false;
    if (!eqLottieVector(a->style.position, b->style.position)) return false;
    if (!eqLottieScalar(a->style.scale, b->style.scale)) return false;
    if (!eqLottieFloat(a->style.letterSpace, b->style.letterSpace)) return false;
    if (!eqLottieFloat(a->style.lineSpace, b->style.lineSpace)) return false;
    if (!eqLottieFloat(a->style.strokeWidth, b->style.strokeWidth)) return false;
    if (!eqLottieFloat(a->style.rotation, b->style.rotation)) return false;
    if (!eqLottieOpacity(a->style.fillOpacity, b->style.fillOpacity)) return false;
    if (!eqLottieOpacity(a->style.strokeOpacity, b->style.strokeOpacity)) return false;
    if (!eqLottieOpacity(a->style.opacity, b->style.opacity)) return false;
    if (a->style.flags.fillColor != b->style.flags.fillColor) return false;
    if (a->style.flags.strokeColor != b->style.flags.strokeColor) return false;
    if (a->style.flags.strokeWidth != b->style.flags.strokeWidth) return false;
    if (!eqLottieFloat(a->offset, b->offset)) return false;
    if (!eqLottieFloat(a->maxEase, b->maxEase)) return false;
    if (!eqLottieFloat(a->minEase, b->minEase)) return false;
    if (!eqLottieFloat(a->maxAmount, b->maxAmount)) return false;
    if (!eqLottieFloat(a->smoothness, b->smoothness)) return false;
    if (!eqLottieFloat(a->start, b->start)) return false;
    if (!eqLottieFloat(a->end, b->end)) return false;
    if (!eqInterpolator(a->interpolator, b->interpolator)) return false;
    if (a->based != b->based) return false;
    if (a->shape != b->shape) return false;
    if (a->rangeUnit != b->rangeUnit) return false;
    if (a->random != b->random) return false;
    if (a->expressible != b->expressible) return false;
    return true;
}

static bool eqTextFollowPath(const LottieTextFollowPath* a, const LottieTextFollowPath* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieFloat(a->firstMargin, b->firstMargin)) return false;
    if (a->maskIdx != b->maskIdx) return false;
    return true;
}

static bool eqGroup(const LottieGroup* a, const LottieGroup* b)
{
    if (!a || !b) return a == b;
    if (a->blendMethod != b->blendMethod) return false;
    if (a->reqFragment != b->reqFragment) return false;
    if (a->buildDone != b->buildDone) return false;
    if (a->trimpath != b->trimpath) return false;
    if (a->visible != b->visible) return false;
    if (a->allowMerge != b->allowMerge) return false;
    if (a->children.count != b->children.count) return false;
    for (uint32_t i = 0; i < a->children.count; ++i) {
        if (!eqObject(a->children[i], b->children[i])) return false;
    }
    return true;
}

static bool eqTransform(const LottieTransform* a, const LottieTransform* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieVector(a->position, b->position)) return false;
    if (!eqLottieFloat(a->rotation, b->rotation)) return false;
    if (!eqLottieScalar(a->scale, b->scale)) return false;
    if (!eqLottieScalar(a->anchor, b->anchor)) return false;
    if (!eqLottieOpacity(a->opacity, b->opacity)) return false;
    if (!eqLottieFloat(a->skewAngle, b->skewAngle)) return false;
    if (!eqLottieFloat(a->skewAxis, b->skewAxis)) return false;
    if (!a->coords || !b->coords) {
        if (a->coords || b->coords) return false;
    } else {
        if (!eqLottieFloat(a->coords->x, b->coords->x)) return false;
        if (!eqLottieFloat(a->coords->y, b->coords->y)) return false;
    }
    if (!a->rotationEx || !b->rotationEx) {
        if (a->rotationEx || b->rotationEx) return false;
    } else {
        if (!eqLottieFloat(a->rotationEx->x, b->rotationEx->x)) return false;
        if (!eqLottieFloat(a->rotationEx->y, b->rotationEx->y)) return false;
    }
    return true;
}

static bool eqSolid(const LottieSolid* a, const LottieSolid* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieColor(a->color, b->color)) return false;
    if (!eqLottieOpacity(a->opacity, b->opacity)) return false;
    return true;
}

static bool eqSolidFill(const LottieSolidFill* a, const LottieSolidFill* b)
{
    if (!eqSolid(a, b)) return false;
    if (a->rule != b->rule) return false;
    return true;
}

static bool eqSolidStroke(const LottieSolidStroke* a, const LottieSolidStroke* b)
{
    if (!eqSolid(a, b)) return false;
    if (!eqStroke(a, b)) return false;
    return true;
}

static bool eqGradient(const LottieGradient* a, const LottieGradient* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieScalar(a->start, b->start)) return false;
    if (!eqLottieScalar(a->end, b->end)) return false;
    if (!eqLottieFloat(a->height, b->height)) return false;
    if (!eqLottieFloat(a->angle, b->angle)) return false;
    if (!eqLottieOpacity(a->opacity, b->opacity)) return false;
    if (!eqLottieColorStop(a->colorStops, b->colorStops)) return false;
    if (a->id != b->id) return false;
    if (a->opaque != b->opaque) return false;
    return true;
}

static bool eqGradientFill(const LottieGradientFill* a, const LottieGradientFill* b)
{
    if (!eqGradient(a, b)) return false;
    if (a->rule != b->rule) return false;
    return true;
}

static bool eqGradientStroke(const LottieGradientStroke* a, const LottieGradientStroke* b)
{
    if (!eqGradient(a, b)) return false;
    if (!eqStroke(a, b)) return false;
    return true;
}

static bool eqRect(const LottieRect* a, const LottieRect* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieVector(a->position, b->position)) return false;
    if (!eqLottieScalar(a->size, b->size)) return false;
    if (!eqLottieFloat(a->radius, b->radius)) return false;
    return true;
}

static bool eqEllipse(const LottieEllipse* a, const LottieEllipse* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieVector(a->position, b->position)) return false;
    if (!eqLottieScalar(a->size, b->size)) return false;
    return true;
}

static bool eqPath(const LottiePath* a, const LottiePath* b)
{
    if (!a || !b) return a == b;
    return eqLottiePathSet(a->pathset, b->pathset);
}

static bool eqPolystar(const LottiePolyStar* a, const LottiePolyStar* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieVector(a->position, b->position)) return false;
    if (!eqLottieFloat(a->innerRadius, b->innerRadius)) return false;
    if (!eqLottieFloat(a->outerRadius, b->outerRadius)) return false;
    if (!eqLottieFloat(a->innerRoundness, b->innerRoundness)) return false;
    if (!eqLottieFloat(a->outerRoundness, b->outerRoundness)) return false;
    if (!eqLottieFloat(a->rotation, b->rotation)) return false;
    if (!eqLottieFloat(a->ptsCnt, b->ptsCnt)) return false;
    if (a->type != b->type) return false;
    return true;
}

static bool eqImage(const LottieImage* a, const LottieImage* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieBitmap(a->bitmap, b->bitmap)) return false;
    if (a->resolved != b->resolved) return false;
    return true;
}

static bool eqTrimpath(const LottieTrimpath* a, const LottieTrimpath* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieFloat(a->start, b->start)) return false;
    if (!eqLottieFloat(a->end, b->end)) return false;
    if (!eqLottieFloat(a->offset, b->offset)) return false;
    if (a->type != b->type) return false;
    return true;
}

static bool eqRoundedCorner(const LottieRoundedCorner* a, const LottieRoundedCorner* b)
{
    if (!a || !b) return a == b;
    return eqLottieFloat(a->radius, b->radius);
}

static bool eqOffsetPath(const LottieOffsetPath* a, const LottieOffsetPath* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieFloat(a->offset, b->offset)) return false;
    if (!eqLottieFloat(a->miterLimit, b->miterLimit)) return false;
    if (a->join != b->join) return false;
    return true;
}

static bool eqRepeater(const LottieRepeater* a, const LottieRepeater* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieFloat(a->copies, b->copies)) return false;
    if (!eqLottieFloat(a->offset, b->offset)) return false;
    if (!eqLottieVector(a->position, b->position)) return false;
    if (!eqLottieFloat(a->rotation, b->rotation)) return false;
    if (!eqLottieScalar(a->scale, b->scale)) return false;
    if (!eqLottieScalar(a->anchor, b->anchor)) return false;
    if (!eqLottieOpacity(a->startOpacity, b->startOpacity)) return false;
    if (!eqLottieOpacity(a->endOpacity, b->endOpacity)) return false;
    if (a->inorder != b->inorder) return false;
    return true;
}

static bool eqText(const LottieText* a, const LottieText* b)
{
    if (!a || !b) return a == b;
    if (!eqLottieTextDoc(a->doc, b->doc)) return false;
    if (!eqFont(a->font, b->font)) return false;
    if (!eqTextFollowPath(a->follow, b->follow)) return false;
    if (a->ranges.count != b->ranges.count) return false;
    for (uint32_t i = 0; i < a->ranges.count; ++i) {
        if (!eqTextRange(a->ranges[i], b->ranges[i])) return false;
    }
    if (a->alignOp.group != b->alignOp.group) return false;
    if (!eqLottieScalar(a->alignOp.anchor, b->alignOp.anchor)) return false;
    return true;
}

static bool eqLayer(const LottieLayer* a, const LottieLayer* b)
{
    if (!a || !b) return a == b;
    if (!eqGroup(a, b)) return false;
    if (!eqString(a->name, b->name)) return false;
    if (!eqLottieFloat(a->timeRemap, b->timeRemap)) return false;
    if (!eqTransform(a->transform, b->transform)) return false;
    if (a->masks.count != b->masks.count) return false;
    for (uint32_t i = 0; i < a->masks.count; ++i) {
        if (!eqMask(a->masks[i], b->masks[i])) return false;
    }
    if (a->effects.count != b->effects.count) return false;
    for (uint32_t i = 0; i < a->effects.count; ++i) {
        if (!eqEffect(a->effects[i], b->effects[i])) return false;
    }
    if (!eqFloat(a->timeStretch, b->timeStretch)) return false;
    if (!eqFloat(a->w, b->w)) return false;
    if (!eqFloat(a->h, b->h)) return false;
    if (!eqFloat(a->inFrame, b->inFrame)) return false;
    if (!eqFloat(a->outFrame, b->outFrame)) return false;
    if (!eqFloat(a->startFrame, b->startFrame)) return false;
    if (a->rid != b->rid) return false;
    if (a->mix != b->mix) return false;
    if (a->pix != b->pix) return false;
    if (a->ix != b->ix) return false;
    if (a->matteType != b->matteType) return false;
    if (a->type != b->type) return false;
    if (a->autoOrient != b->autoOrient) return false;
    if (a->matteSrc != b->matteSrc) return false;
    return true;
}

static bool eqObject(const LottieObject* a, const LottieObject* b)
{
    if (!a || !b) return a == b;
    if (a->type != b->type) return false;
    if (a->id != b->id) return false;
    if (a->hidden != b->hidden) return false;
    switch (a->type) {
        case LottieObject::Composition:
            return eqGroup(static_cast<const LottieGroup*>(a), static_cast<const LottieGroup*>(b));
        case LottieObject::Layer:
            return eqLayer(static_cast<const LottieLayer*>(a), static_cast<const LottieLayer*>(b));
        case LottieObject::Group:
            return eqGroup(static_cast<const LottieGroup*>(a), static_cast<const LottieGroup*>(b));
        case LottieObject::Transform:
            return eqTransform(static_cast<const LottieTransform*>(a), static_cast<const LottieTransform*>(b));
        case LottieObject::SolidFill:
            return eqSolidFill(static_cast<const LottieSolidFill*>(a), static_cast<const LottieSolidFill*>(b));
        case LottieObject::SolidStroke:
            return eqSolidStroke(static_cast<const LottieSolidStroke*>(a), static_cast<const LottieSolidStroke*>(b));
        case LottieObject::GradientFill:
            return eqGradientFill(static_cast<const LottieGradientFill*>(a), static_cast<const LottieGradientFill*>(b));
        case LottieObject::GradientStroke:
            return eqGradientStroke(static_cast<const LottieGradientStroke*>(a), static_cast<const LottieGradientStroke*>(b));
        case LottieObject::Rect:
            return eqRect(static_cast<const LottieRect*>(a), static_cast<const LottieRect*>(b));
        case LottieObject::Ellipse:
            return eqEllipse(static_cast<const LottieEllipse*>(a), static_cast<const LottieEllipse*>(b));
        case LottieObject::Path:
            return eqPath(static_cast<const LottiePath*>(a), static_cast<const LottiePath*>(b));
        case LottieObject::Polystar:
            return eqPolystar(static_cast<const LottiePolyStar*>(a), static_cast<const LottiePolyStar*>(b));
        case LottieObject::Image:
            return eqImage(static_cast<const LottieImage*>(a), static_cast<const LottieImage*>(b));
        case LottieObject::Trimpath:
            return eqTrimpath(static_cast<const LottieTrimpath*>(a), static_cast<const LottieTrimpath*>(b));
        case LottieObject::Text:
            return eqText(static_cast<const LottieText*>(a), static_cast<const LottieText*>(b));
        case LottieObject::Repeater:
            return eqRepeater(static_cast<const LottieRepeater*>(a), static_cast<const LottieRepeater*>(b));
        case LottieObject::RoundedCorner:
            return eqRoundedCorner(static_cast<const LottieRoundedCorner*>(a), static_cast<const LottieRoundedCorner*>(b));
        case LottieObject::OffsetPath:
            return eqOffsetPath(static_cast<const LottieOffsetPath*>(a), static_cast<const LottieOffsetPath*>(b));
        default:
            return false;
    }
}

static bool eqMarker(const LottieMarker* a, const LottieMarker* b)
{
    if (!a || !b) return a == b;
    if (!eqString(a->name, b->name)) return false;
    if (!eqFloat(a->time, b->time)) return false;
    if (!eqFloat(a->duration, b->duration)) return false;
    return true;
}

static bool eqSlot(const LottieSlot* a, const LottieSlot* b)
{
    if (!a || !b) return a == b;
    if (a->sid != b->sid) return false;
    if (a->type != b->type) return false;
    if (a->overridden != b->overridden) return false;
    if (a->pairs.count != b->pairs.count) return false;
    for (uint32_t i = 0; i < a->pairs.count; ++i) {
        if (a->pairs[i].obj->id != b->pairs[i].obj->id) return false;
        if (!eqProperty(a->pairs[i].prop, b->pairs[i].prop)) return false;
    }
    return true;
}

static bool eqComposition(const LottieComposition* a, const LottieComposition* b)
{
    if (!a || !b) return a == b;
    if (!eqString(a->version, b->version)) return false;
    if (!eqString(a->name, b->name)) return false;
    if (!eqFloat(a->w, b->w)) return false;
    if (!eqFloat(a->h, b->h)) return false;
    if (!eqFloat(a->frameRate, b->frameRate)) return false;
    if (a->expressions != b->expressions) return false;
    if (a->initiated != b->initiated) return false;
    if (a->quality != b->quality) return false;
    if (a->assets.count != b->assets.count) return false;
    for (uint32_t i = 0; i < a->assets.count; ++i) {
        if (!eqObject(a->assets[i], b->assets[i])) return false;
    }
    if (a->fonts.count != b->fonts.count) return false;
    for (uint32_t i = 0; i < a->fonts.count; ++i) {
        if (!eqFont(a->fonts[i], b->fonts[i])) return false;
    }
    if (a->slots.count != b->slots.count) return false;
    for (uint32_t i = 0; i < a->slots.count; ++i) {
        if (!eqSlot(a->slots[i], b->slots[i])) return false;
    }
    if (a->markers.count != b->markers.count) return false;
    for (uint32_t i = 0; i < a->markers.count; ++i) {
        if (!eqMarker(a->markers[i], b->markers[i])) return false;
    }
    if (a->interpolators.count != b->interpolators.count) return false;
    for (uint32_t i = 0; i < a->interpolators.count; ++i) {
        if (!eqInterpolator(a->interpolators[i], b->interpolators[i])) return false;
    }
    return eqLayer(a->root, b->root);
}

static bool readFile(const char* path, std::vector<char>& out)
{
    ifstream file(path, ios::binary);
    if (!file) return false;
    file.seekg(0, ios::end);
    auto size = static_cast<size_t>(file.tellg());
    file.seekg(0, ios::beg);
    out.resize(size);
    file.read(out.data(), size);
    return file.good();
}

static string rootFromTestDir()
{
    string root = TEST_DIR;
    const string tail = "/zvg/test/resources";
    auto pos = root.rfind(tail);
    if (pos != string::npos) root.erase(pos);
    const string zvgTail = "/zvg";
    if (root.size() > zvgTail.size() && root.rfind(zvgTail) == root.size() - zvgTail.size()) {
        root.erase(root.size() - zvgTail.size());
    }
    return root;
}

TEST_CASE("Lottie Coverages", "[tvgLottie]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        #define TEST_CNT 10

        const char* names[TEST_CNT] = {
            "test3.lot",
            "test4.lot",
            "test5.lot",
            "test6.lot",
            "test7.lot",
            "test8.lot",
            "test9.lot",
            "test10.lot",
            "test11.lot",
            "test12.lot"
        };

        auto animation = unique_ptr<Animation>(Animation::gen());
        REQUIRE(animation);

        auto picture = animation->picture();

        for (int i = 0; i < TEST_CNT; ++i) {
            char buf[100];
            snprintf(buf, sizeof(buf), TEST_DIR"/%s", names[i]);
            REQUIRE(picture->load(buf) == Result::Success);
            REQUIRE(animation->frame(0.0f) == Result::InsufficientCondition);
            REQUIRE(animation->frame(animation->totalFrame() * 0.5f) == Result::Success);
            REQUIRE(animation->frame(animation->totalFrame()) == Result::Success);
        }
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Lottie Slot", "[tvgLottie]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto animation = unique_ptr<LottieAnimation>(LottieAnimation::gen());
        REQUIRE(animation);

        auto picture = animation->picture();

        //Slot Test 1
        const char* slotJson = R"({"gradient_fill":{"p":{"p":2,"k":{"a":0,"k":[0,0.1,0.1,0.2,1,1,0.1,0.2,0.1,1]}}}})";

        //Negative: slot generation before loaded
        REQUIRE(animation->gen(slotJson) == 0);

        REQUIRE(picture->load(TEST_DIR"/slot.lot") == Result::Success);

        auto id = animation->gen(slotJson);
        REQUIRE(id > 0);

        REQUIRE(animation->apply(0) == Result::Success);
        REQUIRE(animation->apply(id) == Result::Success);
        REQUIRE(animation->apply(0) == Result::Success);
        REQUIRE(animation->apply(id) == Result::Success);
        REQUIRE(animation->gen("") == 0);
        REQUIRE(animation->del(id) == Result::Success);

        //Slot Test 2
        const char* slotJson2 = R"({"lottie-icon-outline":{"p":{"a":0,"k":[1,1,0]}},"lottie-icon-solid":{"p":{"a":0,"k":[0,0,1]}}})";

        auto id2 = animation->gen(slotJson2);
        REQUIRE(id2 > 0);

        REQUIRE(animation->apply(id2) == Result::Success);
        REQUIRE(animation->apply(0) == Result::Success);
        REQUIRE(animation->apply(id2) == Result::Success);
        REQUIRE(animation->del(id2) == Result::Success);

        //Slot Test 3 (Transform)
        const char* positionSlot = R"({"transform_id":{"p":{"a":1,"k":[{"i":{"x":0.833,"y":0.833},"o":{"x":0.167,"y":0.167},"s":[100,100],"t":0},{"s":[200,300],"t":100}]}}})";
        auto id3 = animation->gen(positionSlot);
        REQUIRE(id3 > 0);
        REQUIRE(animation->apply(id3) == Result::Success);
        REQUIRE(animation->apply(0) == Result::Success);
        REQUIRE(animation->del(id3) == Result::Success);

        const char* scaleSlot = R"({"transform_id":{"p":{"a":1,"k":[{"i":{"x":0.833,"y":0.833},"o":{"x":0.167,"y":0.167},"s":[0,0],"t":0},{"s":[100,100],"t":100}]}}})";
        auto id4 = animation->gen(scaleSlot);
        REQUIRE(id4 > 0);
        REQUIRE(animation->apply(id4) == Result::Success);
        REQUIRE(animation->apply(0) == Result::Success);
        REQUIRE(animation->del(id4) == Result::Success);

        const char* rotationSlot = R"({"transform_id":{"p":{"a":1,"k":[{"i":{"x":0.833,"y":0.833},"o":{"x":0.167,"y":0.167},"s":[0],"t":0},{"s":[180],"t":100}]}}})";
        auto id5 = animation->gen(rotationSlot);
        REQUIRE(id5 > 0);
        REQUIRE(animation->apply(id5) == Result::Success);
        REQUIRE(animation->apply(0) == Result::Success);
        REQUIRE(animation->del(id5) == Result::Success);

        const char* opacitySlot = R"({"transform_id":{"p":{"a":1,"k":[{"i":{"x":0.833,"y":0.833},"o":{"x":0.167,"y":0.167},"s":[0],"t":0},{"s":[100],"t":100}]}}})";
        auto id6 = animation->gen(opacitySlot);
        REQUIRE(id6 > 0);
        REQUIRE(animation->apply(id6) == Result::Success);
        REQUIRE(animation->apply(0) == Result::Success);
        REQUIRE(animation->del(id6) == Result::Success);

        //Slot Test 4: Expression
        const char* expressionSlot = R"({"rect_rotation":{"p":{"x":"var $bm_rt = time * 360;"}},"rect_scale":{"p":{"x":"var $bm_rt = [];$bm_rt[0] = value[0] + Math.cos(2 * Math.PI * time) * 100;$bm_rt[1] = value[1];"}},"rect_position":{"p":{"x":"var $bm_rt = [];$bm_rt[0] = value[0] + Math.cos(2 * Math.PI * time) * 100;$bm_rt[1] = value[1];"}}})";
        auto id7 = animation->gen(expressionSlot);
        REQUIRE(id7 > 0);
        REQUIRE(animation->apply(id7) == Result::Success);
        REQUIRE(animation->apply(0) == Result::Success);
        REQUIRE(animation->del(id7) == Result::Success);

        //Slot Test 5: Text
        const char* textSlot = R"({"text_doc":{"p":{"k":[{"s":{"f":"Ubuntu Light Italic","t":"ThorVG!","j":0,"s":48,"fc":[1,1,1]},"t":0}]}}})";
        auto id8 = animation->gen(textSlot);
        REQUIRE(id8 > 0);
        REQUIRE(animation->apply(id8) == Result::Success);
        REQUIRE(animation->apply(0) == Result::Success);
        REQUIRE(animation->del(id8) == Result::Success);

        //Slot Test 6: Image
        const char* imageSlot = R"({"path_img":{"p":{"id":"image_0","w":200,"h":300,"u":"images/","p":"logo.png","e":0}}})";
        auto id9 = animation->gen(imageSlot);
        REQUIRE(id9 > 0);
        REQUIRE(animation->apply(id9) == Result::Success);
        REQUIRE(animation->apply(0) == Result::Success);
        REQUIRE(animation->del(id9) == Result::Success);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Lottie Marker", "[tvgLottie]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto animation = unique_ptr<LottieAnimation>(LottieAnimation::gen());
        REQUIRE(animation);

        auto picture = animation->picture();

        //Set marker name before loaded
        REQUIRE(animation->segment("sectionC") == Result::InsufficientCondition);

        //Animation load
        REQUIRE(picture->load(TEST_DIR"/segment.lot") == Result::Success);

        //Set marker
        REQUIRE(animation->segment("sectionA") == Result::Success);

        //Set marker by invalid name
        REQUIRE(animation->segment("") == Result::InvalidArguments);

        //Get marker count
        REQUIRE(animation->markersCnt() == 3);

        //Get marker name by index
        REQUIRE(!strcmp(animation->marker(1), "sectionB"));

        //Get marker name by invalid index
        REQUIRE(animation->marker(-1) == nullptr);

        REQUIRE(animation->segment(nullptr) == Result::Success);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Lottie Tween", "[tvgLottie]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto animation = unique_ptr<LottieAnimation>(LottieAnimation::gen());
        REQUIRE(animation);

        auto picture = animation->picture();

        REQUIRE(animation->tween(0.0f, 10.0f, 0.5f) == Result::InsufficientCondition);

        REQUIRE(picture->load(TEST_DIR"/test.lot") == Result::Success);

        //Set initial frame to avoid frame difference being too small
        REQUIRE(animation->frame(5.0f) == Result::Success);

        //Tween between frames with different progress values
        REQUIRE(animation->tween(0.0f, 10.0f, 0.5f) == Result::Success);
        REQUIRE(animation->tween(10.0f, 20.0f, 0.0f) == Result::Success);
        REQUIRE(animation->tween(20.0f, 30.0f, 1.0f) == Result::Success);

        //Tween with different frame ranges
        REQUIRE(animation->tween(10.0f, 50.0f, 0.25f) == Result::Success);
        REQUIRE(animation->tween(50.0f, 100.0f, 0.75f) == Result::Success);

        //Tween between distant frames
        REQUIRE(animation->tween(0.0f, 100.0f, 0.5f) == Result::Success);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Lottie Quality", "[tvgLottie]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto animation = unique_ptr<LottieAnimation>(LottieAnimation::gen());
        REQUIRE(animation);

        auto picture = animation->picture();

        REQUIRE(animation->quality(50) == Result::InsufficientCondition);

        REQUIRE(picture->load(TEST_DIR"/test.lot") == Result::Success);

        //Set quality with minimum value
        REQUIRE(animation->quality(0) == Result::Success);

        //Set quality with default value
        REQUIRE(animation->quality(50) == Result::Success);

        //Set quality with maximum value
        REQUIRE(animation->quality(100) == Result::Success);

        //Set quality with various values
        REQUIRE(animation->quality(25) == Result::Success);
        REQUIRE(animation->quality(75) == Result::Success);

        //Set quality with invalid value (> 100)
        REQUIRE(animation->quality(101) == Result::InvalidArguments);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Lottie Json Zfb Consistency", "[tvgLottie]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto root = rootFromTestDir();
        auto dir = root + "/example/lottiejson";
        auto jsonPath = dir + "/ok.json";
        auto zfbPath = dir + "/ok.zfb";

        vector<char> jsonBuf;
        REQUIRE(readFile(jsonPath.c_str(), jsonBuf));
        jsonBuf.push_back('\0');

        LottieParser jsonParser(jsonBuf.data(), dir.c_str(), false);
        REQUIRE(jsonParser.parse());
        REQUIRE(jsonParser.comp);

        vector<char> zfbBuf;
        REQUIRE(readFile(zfbPath.c_str(), zfbBuf));

        LottieFlatBufferParser zfbParser(zfbBuf.data(), zfbBuf.size(), dir.c_str());
        REQUIRE(zfbParser.parse());
        REQUIRE(zfbParser.comp);

        REQUIRE(eqComposition(jsonParser.comp, zfbParser.comp));
    }
    REQUIRE(Initializer::term() == Result::Success);
}

TEST_CASE("Lottie Asset Resolver", "[tvgLottie]")
{
    REQUIRE(Initializer::init() == Result::Success);
    {
        auto animation = unique_ptr<LottieAnimation>(LottieAnimation::gen());
        REQUIRE(animation);

        auto picture = animation->picture();

        auto resolver = [](Paint* p, const char* src, void* data) -> bool {
            if (p->type() == tvg::Type::Picture) {
                string resolvedPath = string(TEST_DIR) + "/image/test.png";
                auto ret = static_cast<Picture*>(p)->load(resolvedPath.c_str());
                return (ret == Result::Success);
            } else if (p->type() == tvg::Type::Text) {
                string fontPath = string(TEST_DIR) + "/font/Arial.ttf";
                if (Text::load(fontPath.c_str()) != Result::Success) return false;
                auto ret = static_cast<Text*>(p)->font("Arial");
                return (ret == Result::Success);
            }
            return false;
        };

        // Test unset resolver
        REQUIRE(picture->resolver(resolver, nullptr) == Result::Success);
        REQUIRE(picture->resolver(nullptr, nullptr) == Result::Success);

        //Resolver Test (Image and Font)
        REQUIRE(picture->resolver(resolver, nullptr) == Result::Success);
        REQUIRE(picture->load(TEST_DIR"/resolver.json") == Result::Success);
        REQUIRE(animation->frame(animation->totalFrame() * 0.5f) == Result::Success);

        //Test that setting/unsetting resolver after load
        REQUIRE(picture->resolver(resolver, nullptr) == Result::InsufficientCondition);
        REQUIRE(picture->resolver(nullptr, nullptr) == Result::InsufficientCondition);
    }
    REQUIRE(Initializer::term() == Result::Success);
}

#endif
