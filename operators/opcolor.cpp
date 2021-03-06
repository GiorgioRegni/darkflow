/*
 * Copyright (c) 2006-2016, Guillaume Gimenez <guillaume@blackmilk.fr>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of G.Gimenez nor the names of its contributors may
 *       be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL G.Gimenez BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *     * Guillaume Gimenez <guillaume@blackmilk.fr>
 *
 */
#include "opcolor.h"
#include "operatorworker.h"
#include "operatorinput.h"
#include "operatorparameterslider.h"
#include "operatoroutput.h"
#include <Magick++.h>
using Magick::Quantum;

class WorkerColor : public OperatorWorker {
public:
    WorkerColor(qreal r, qreal g, qreal b, QThread *thread, Operator *op) :
        OperatorWorker(thread, op),
        m_r(r),
        m_g(g),
        m_b(b)
    {}
    Photo process(const Photo &, int, int) { throw 0; }

    void play() {
        Photo photo;
        photo.setIdentity(m_operator->uuid());
        photo.createImage(1,1);
        quantum_t r = DF_ROUND((m_r-(1./(1<<16)))*(1<<16));
        quantum_t g = DF_ROUND((m_g-(1./(1<<16)))*(1<<16));
        quantum_t b = DF_ROUND((m_b-(1./(1<<16)))*(1<<16));
        photo.setTag(TAG_NAME, "Color");
        photo.image().pixelColor(0, 0, Magick::Color(r,g,b));
        outputPush(0, photo);
        emitSuccess();
    }

private:
    qreal m_r;
    qreal m_g;
    qreal m_b;
};
OpColor::OpColor(Process *parent) :
    Operator(OP_SECTION_COLOR, QT_TRANSLATE_NOOP("Operator", "Color"), Operator::NA, parent),
    m_r(new OperatorParameterSlider("red", tr("Red"), tr("Color Red Component"), Slider::ExposureValue, Slider::Logarithmic, Slider::Real, 1./(1<<16), 1, 1, 1./(1<<16), 1, Slider::FilterExposureFromOne, this)),
    m_g(new OperatorParameterSlider("green", tr("Green"), tr("Color Green Component"), Slider::ExposureValue, Slider::Logarithmic, Slider::Real, 1./(1<<16), 1, 1, 1./(1<<16), 1, Slider::FilterExposureFromOne, this)),
    m_b(new OperatorParameterSlider("blue", tr("Blue"), tr("Color Blue Component"), Slider::ExposureValue, Slider::Logarithmic, Slider::Real, 1./(1<<16), 1, 1, 1./(1<<16), 1, Slider::FilterExposureFromOne, this))
{
    addOutput(new OperatorOutput(tr("Color"), this));
    addParameter(m_r);
    addParameter(m_g);
    addParameter(m_b);
}

OpColor *OpColor::newInstance()
{
    return new OpColor(m_process);
}

OperatorWorker *OpColor::newWorker()
{
    return new WorkerColor(m_r->value(), m_g->value(), m_b->value(), m_thread, this);
}
