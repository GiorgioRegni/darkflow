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
#include "opflatfieldcorrection.h"
#include "operatorworker.h"
#include "operatorinput.h"
#include "operatoroutput.h"
#include "operatorparameterdropdown.h"
#include "photo.h"
#include <Magick++.h>
#include "algorithm.h"
#include "hdr.h"
#include "console.h"

using Magick::Quantum;

typedef float real;

class WorkerFlatField : public OperatorWorker {
public:
    WorkerFlatField(bool outputHDR, QThread *thread, Operator *op) :
        OperatorWorker(thread, op),
        m_outputHDR(outputHDR),
        m_max()
    {}
    void play_analyseSources() {
        Q_ASSERT(m_inputs.count() == 2);
        foreach(Photo photo, m_inputs[1]) {
            bool hdr = photo.getScale() == Photo::HDR;
            try  {
                Magick::Image& image = photo.image();
                Ordinary::Pixels pixels_cache(image);
                int w = image.columns();
                int h = image.rows();
                Triplet<real> max;
                for ( int y = 0 ; y < h ; ++ y) {
                    const Magick::PixelPacket * pixels = pixels_cache.getConst(0, y, w, 1);
                    for ( int x = 0 ; x < w ; ++x ) {
                        if ( pixels[x].red > max.red )
                            max.red = pixels[x].red;
                        if ( pixels[x].green > max.green )
                            max.green = pixels[x].green;
                        if ( pixels[x].blue > max.blue )
                            max.blue = pixels[x].blue;
                    }
                }
                if ( hdr ) {
                    max.red = fromHDR(max.red);
                    max.green = fromHDR(max.green);
                    max.blue = fromHDR(max.blue);
                }
                m_max.push_back(max);
            }
            catch (std::exception &e) {
                setError(photo, e.what());
                emitFailure();
            }
        }
    }
    template<typename PIXEL>
    void correct(Magick::Image &image, bool imageIsHDR,
                 Magick::Image& flatfield, bool flatfieldIsHDR,
                 Magick::Image& overflow,
                 Triplet<real> & max,
                 int p,
                 int c) {
        int w = image.columns();
        int h = image.rows();
        int f_w = flatfield.columns();
        int f_h = flatfield.rows();
        if ( w != f_w || h != f_h ) {
            dflError("size mismatch");
            return;
        }
        Magick::Image srcImage(image);
        ResetImage(image);
        ResetImage(overflow);
        std::shared_ptr<Ordinary::Pixels> src_cache(new Ordinary::Pixels(srcImage));
        std::shared_ptr<Ordinary::Pixels> image_cache(new Ordinary::Pixels(image));
        std::shared_ptr<Ordinary::Pixels> flatfield_cache(new Ordinary::Pixels(flatfield));
        std::shared_ptr<Ordinary::Pixels> overflow_cache(new Ordinary::Pixels(overflow));
        dfl_block int line=0;
        dfl_parallel_for(y, 0, h, 4, (srcImage, image, flatfield, overflow), {
            Magick::PixelPacket *image_pixels = image_cache->get(0, y, w, 1);
            Magick::PixelPacket *overflow_pixels = overflow_cache->get(0, y, w, 1);
            const Magick::PixelPacket *flatfield_pixels = flatfield_cache->getConst(0, y, w, 1);
            const Magick::PixelPacket *src = src_cache->getConst(0, y, w, 1);
            if ( m_error || !image_pixels || !overflow_pixels || !flatfield_pixels || !src ) {
                if ( !m_error )
                    dflError(DF_NULL_PIXELS);
                continue;
            }

            for ( int x = 0 ; x < w ; ++x ) {
                Triplet<PIXEL> ff;
                bool singularity = false;
                if ( flatfield_pixels[x].red )
                    ff.red = flatfield_pixels[x].red;
                else {
                    //continue;
                    ff.red = 1;
                    singularity = true;
                }
                if ( flatfield_pixels[x].green )
                    ff.green = flatfield_pixels[x].green;
                else {
                    //continue;
                    ff.green = 1;
                    singularity = true;
                }
                if ( flatfield_pixels[x].blue )
                    ff.blue = flatfield_pixels[x].blue;
                else {
                    //continue;
                    ff.blue = 1;
                    singularity = true;
                }
                if ( flatfieldIsHDR ) {
                    ff.red = fromHDR(ff.red);
                    ff.green = fromHDR(ff.green);
                    ff.blue = fromHDR(ff.blue);
                }
                PIXEL r = src[x].red;
                PIXEL g = src[x].green;
                PIXEL b = src[x].blue;
                if ( imageIsHDR ) {
                    r = fromHDR(r);
                    g = fromHDR(g);
                    b = fromHDR(b);
                }
                r = r * max.red  / ff.red;
                g = g * max.green / ff.green;
                b = b * max.blue / ff.blue;
                overflow_pixels[x].red = overflow_pixels[x].green = overflow_pixels[x].blue =
                        ( singularity || r > QuantumRange || g > QuantumRange || b > QuantumRange )
                        ? QuantumRange
                        : 0;
                if ( m_outputHDR ) {
                    image_pixels[x].red =
                            clamp<quantum_t>(toHDR(r));
                    image_pixels[x].green =
                            clamp<quantum_t>(toHDR(g));
                    image_pixels[x].blue =
                            clamp<quantum_t>(toHDR(b));
                }
                else {
                    image_pixels[x].red =
                            clamp<quantum_t>(r);
                    image_pixels[x].green =
                            clamp<quantum_t>(g);
                    image_pixels[x].blue =
                            clamp<quantum_t>(b);
                }
            }
            dfl_critical_section(
            {
                if ( line % 100 == 0 )
                    emitProgress(p, c, line, h);
                ++line;
            });
            image_cache->sync();
            overflow_cache->sync();
        });
    }

    void play() {
        Q_ASSERT( m_inputs.count() == 2 );
        if ( m_inputs[1].count() == 0 )
            return OperatorWorker::play();
        play_analyseSources();
        int n_photos = m_inputs[0].count() * m_inputs[1].count();
        int n = 0;
        int source_flatfield_idx = 0;
        foreach(Photo flatfield, m_inputs[1]) {
            foreach(Photo photo, m_inputs[0]) {
                if (aborted())
                    continue;
                try {
                    Photo overflow(photo);
                    if (photo.getScale() == Photo::HDR ||
                        flatfield.getScale() == Photo::HDR )
                        correct<real>(photo.image(), photo.getScale() == Photo::HDR,
                                      flatfield.image(), flatfield.getScale() == Photo::HDR,
                                      overflow.image(),
                                      m_max[source_flatfield_idx],
                                      n, n_photos);
                    else
                        correct<quantum_t>(photo.image(), photo.getScale() == Photo::HDR,
                                           flatfield.image(), flatfield.getScale() == Photo::HDR,
                                           overflow.image(),
                                           m_max[source_flatfield_idx],
                                           n, n_photos);
                    if ( m_outputHDR )
                        photo.setScale(Photo::HDR);
                    else if ( photo.getScale() == Photo::HDR )
                        photo.setScale(Photo::Linear);
                    overflow.setScale(Photo::Linear);
                    outputPush(0, photo);
                    outputPush(1, overflow);
                    ++n;
                    emit progress(n, n_photos);
                }
                catch (std::exception &e) {
                    setError(flatfield, e.what());
                    setError(photo, e.what());
                    emitFailure();
                    return;
                }
            }
            ++source_flatfield_idx;
        }
        if ( aborted() )
            emitFailure();
        else
            emitSuccess();
    }

    Photo process(const Photo &photo, int, int) { return Photo(photo); }
private:
    bool m_outputHDR;
    QVector<Triplet<real> > m_max;
};

OpFlatFieldCorrection::OpFlatFieldCorrection(Process *parent) :
    Operator(OP_SECTION_BLEND, QT_TRANSLATE_NOOP("Operator", "Flat-Field Correction"), Operator::All, parent),
    m_outputHDR(new OperatorParameterDropDown("outputHDR", tr("Output HDR"), this, SLOT(setOutputHDR(int)))),
    m_outputHDRValue(false)
{
    m_outputHDR->addOption(DF_TR_AND_C("No"), false, true);
    m_outputHDR->addOption(DF_TR_AND_C("Yes"), true);

    addInput(new OperatorInput(tr("Uneven images"), OperatorInput::Set, this));
    addInput(new OperatorInput(tr("Flat-field"), OperatorInput::Set, this));
    addOutput(new OperatorOutput(tr("Flattened"), this));
    addOutput(new OperatorOutput(tr("Overflow"), this));
    addParameter(m_outputHDR);
}

OpFlatFieldCorrection *OpFlatFieldCorrection::newInstance()
{
    return new OpFlatFieldCorrection(m_process);
}

OperatorWorker *OpFlatFieldCorrection::newWorker()
{
    return new WorkerFlatField(m_outputHDRValue, m_thread, this);
}

void OpFlatFieldCorrection::setOutputHDR(int type)
{
    if ( m_outputHDRValue != !!type ) {
        m_outputHDRValue = !!type;
        setOutOfDate();
    }
}
