/*
 * Copyright (C) 2018 Vlad Zagorodniy <vladzzag@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// Own
#include "YetAnotherMagicLampEffect.h"
#include "Model.h"

// Auto-generated
#include "YetAnotherMagicLampConfig.h"

// std
#include <cmath>

enum ShapeCurve {
    Linear = 0,
    Quad = 1,
    Cubic = 2,
    Quart = 3,
    Quint = 4,
    Sine = 5,
    Circ = 6,
    Bounce = 7,
    Bezier = 8
};

YetAnotherMagicLampEffect::YetAnotherMagicLampEffect()
    : m_lastPresentTime(std::chrono::milliseconds::zero())
{
    reconfigure(ReconfigureAll);

    connect(KWin::effects, &KWin::EffectsHandler::windowMinimized,
        this, &YetAnotherMagicLampEffect::slotWindowMinimized);
    connect(KWin::effects, &KWin::EffectsHandler::windowUnminimized,
        this, &YetAnotherMagicLampEffect::slotWindowUnminimized);
    connect(KWin::effects, &KWin::EffectsHandler::windowDeleted,
        this, &YetAnotherMagicLampEffect::slotWindowDeleted);
    connect(KWin::effects, &KWin::EffectsHandler::activeFullScreenEffectChanged,
        this, &YetAnotherMagicLampEffect::slotActiveFullScreenEffectChanged);
}

YetAnotherMagicLampEffect::~YetAnotherMagicLampEffect()
{
}

void YetAnotherMagicLampEffect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags)

    YetAnotherMagicLampConfig::self()->read();

    QEasingCurve curve;
    const auto shapeCurve = static_cast<ShapeCurve>(YetAnotherMagicLampConfig::shapeCurve());
    switch (shapeCurve) {
    case ShapeCurve::Linear:
        curve.setType(QEasingCurve::Linear);
        break;

    case ShapeCurve::Quad:
        curve.setType(QEasingCurve::InOutQuad);
        break;

    case ShapeCurve::Cubic:
        curve.setType(QEasingCurve::InOutCubic);
        break;

    case ShapeCurve::Quart:
        curve.setType(QEasingCurve::InOutQuart);
        break;

    case ShapeCurve::Quint:
        curve.setType(QEasingCurve::InOutQuint);
        break;

    case ShapeCurve::Sine:
        curve.setType(QEasingCurve::InOutSine);
        break;

    case ShapeCurve::Circ:
        curve.setType(QEasingCurve::InOutCirc);
        break;

    case ShapeCurve::Bounce:
        curve.setType(QEasingCurve::InOutBounce);
        break;

    case ShapeCurve::Bezier:
        // With the cubic bezier curve, "0" corresponds to the furtherst edge
        // of a window, "1" corresponds to the closest edge.
        curve.setType(QEasingCurve::BezierSpline);
        curve.addCubicBezierSegment(
            QPointF(0.3, 0.0),
            QPointF(0.7, 1.0),
            QPointF(1.0, 1.0));
        break;
    default:
        // Fallback to the sine curve.
        curve.setType(QEasingCurve::InOutSine);
        break;
    }
    m_modelParameters.shapeCurve = curve;

    const int baseDuration = animationTime<YetAnotherMagicLampConfig>(300);
    m_modelParameters.squashDuration = std::chrono::milliseconds(baseDuration);
    m_modelParameters.stretchDuration = std::chrono::milliseconds(qMax(qRound(baseDuration * 0.7), 1));
    m_modelParameters.bumpDuration = std::chrono::milliseconds(baseDuration);
    m_modelParameters.shapeFactor = YetAnotherMagicLampConfig::initialShapeFactor();
    m_modelParameters.bumpDistance = YetAnotherMagicLampConfig::maxBumpDistance();

    m_gridResolution = YetAnotherMagicLampConfig::gridResolution();
}

void YetAnotherMagicLampEffect::prePaintScreen(KWin::ScreenPrePaintData& data, std::chrono::milliseconds presentTime)
{
    std::chrono::milliseconds delta = std::chrono::milliseconds::zero();
    if (m_lastPresentTime.count())
        delta = presentTime - m_lastPresentTime;
    m_lastPresentTime = presentTime;

    for (Model& model : m_models) {
        model.step(delta);
    }

    data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;

    KWin::effects->prePaintScreen(data, presentTime);
}

void YetAnotherMagicLampEffect::postPaintScreen()
{
    auto modelIt = m_models.begin();
    while (modelIt != m_models.end()) {
        if ((*modelIt).done()) {
            unredirect(modelIt.key());
            modelIt = m_models.erase(modelIt);
        } else {
            ++modelIt;
        }
    }

    if (m_models.isEmpty())
        m_lastPresentTime = std::chrono::milliseconds::zero();

    KWin::effects->addRepaintFull();
    KWin::effects->postPaintScreen();
}

void YetAnotherMagicLampEffect::prePaintWindow(KWin::EffectWindow* w, KWin::WindowPrePaintData& data, std::chrono::milliseconds presentTime)
{
    auto modelIt = m_models.constFind(w);
    if (modelIt != m_models.constEnd()) {
        w->enablePainting(KWin::EffectWindow::PAINT_DISABLED_BY_MINIMIZE);
    }

    KWin::effects->prePaintWindow(w, data, presentTime);
}

void YetAnotherMagicLampEffect::paintWindow(KWin::EffectWindow* w, int mask, QRegion region, KWin::WindowPaintData& data)
{
    QRegion clip = region;

    auto modelIt = m_models.constFind(w);
    if (modelIt != m_models.constEnd()) {
        if ((*modelIt).needsClip()) {
            clip = (*modelIt).clipRegion();
        }
    }

    KWin::effects->paintWindow(w, mask, clip, data);
}

void YetAnotherMagicLampEffect::deform(KWin::EffectWindow *window, int mask, KWin::WindowPaintData &data, KWin::WindowQuadList &quads)
{
    Q_UNUSED(mask)
    Q_UNUSED(data)

    auto modelIt = m_models.constFind(window);
    if (modelIt == m_models.constEnd()) {
        return;
    }

    quads = quads.makeGrid(m_gridResolution);
    (*modelIt).apply(quads);
}

bool YetAnotherMagicLampEffect::isActive() const
{
    return !m_models.isEmpty();
}

bool YetAnotherMagicLampEffect::supported()
{
    if (!KWin::effects->animationsSupported()) {
        return false;
    }
    if (KWin::effects->isOpenGLCompositing()) {
        return true;
    }
    return false;
}

void YetAnotherMagicLampEffect::slotWindowMinimized(KWin::EffectWindow* w)
{
    if (KWin::effects->activeFullScreenEffect()) {
        return;
    }

    const QRect iconRect = w->iconGeometry();
    if (!iconRect.isValid()) {
        return;
    }

    Model& model = m_models[w];
    model.setWindow(w);
    model.setParameters(m_modelParameters);
    model.start(Model::AnimationKind::Minimize);

    redirect(w);

    KWin::effects->addRepaintFull();
}

void YetAnotherMagicLampEffect::slotWindowUnminimized(KWin::EffectWindow* w)
{
    if (KWin::effects->activeFullScreenEffect()) {
        return;
    }

    const QRect iconRect = w->iconGeometry();
    if (!iconRect.isValid()) {
        return;
    }

    Model& model = m_models[w];
    model.setWindow(w);
    model.setParameters(m_modelParameters);
    model.start(Model::AnimationKind::Unminimize);

    redirect(w);

    KWin::effects->addRepaintFull();
}

void YetAnotherMagicLampEffect::slotWindowDeleted(KWin::EffectWindow* w)
{
    m_models.remove(w);
}

void YetAnotherMagicLampEffect::slotActiveFullScreenEffectChanged()
{
    if (KWin::effects->activeFullScreenEffect() != nullptr) {
        for (auto it = m_models.constBegin(); it != m_models.constEnd(); ++it) {
            unredirect(it.key());
        }
        m_models.clear();
    }
}
