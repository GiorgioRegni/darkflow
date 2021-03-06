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
#include "opairydisk.h"
#include "operatoroutput.h"
#include "operatorworker.h"
#include "photo.h"
#include "console.h"
#include "algorithm.h"
#include "operatorparameterslider.h"
#include "operatorparameterdropdown.h"
#include "hdr.h"

using Magick::Quantum;
#define lred  612e-9
#define lgreen  549e-9
#define lblue  450e-9

static const double lambda[] = { lred, lgreen, lblue };

class AiryDisk : public OperatorWorker
{
    double m_diam;
    double m_f;
    double m_pixel_sz;
    double m_offset;
    int m_w;
    int m_precision;
    bool m_outputHDR;
    double m_I0;
public:
    AiryDisk(double diam,
             double f,
             double pixel_sz,
             double offset,
             double w,
             double precision,
             bool outputHDR,
             QThread *thread, OpAiryDisk *op) :
        OperatorWorker(thread, op),
        m_diam(diam),
        m_f(f),
        m_pixel_sz(pixel_sz),
        m_offset(offset),
        m_w(w),
        m_precision(precision),
        m_outputHDR(outputHDR),
        m_I0(QuantumRange)
    {
    }
private slots:
    Photo process(const Photo &, int, int) { throw 0; }
    void play() {
        Photo photo(Photo::Linear);
        photo.setIdentity(m_operator->uuid());
        photo.createImage(m_w, m_w);
        if (photo.isComplete()) {
            Magick::Image& image = photo.image();
            std::shared_ptr<Ordinary::Pixels> cache(new Ordinary::Pixels(image));
            int w = image.columns();
            int h = image.rows();
            double *pixels = new double[w*h*3];
            dfl_block double max = 0;
            int sq_p = m_precision*m_precision;
            dfl_parallel_for(y, 0, h, 4, (), {
                if (aborted())
                    continue;
                emit progress(y/2, h);
                for (int x = 0 ; x < w ; ++x ) {
                    double rgb[3] = {0, 0, 0};
                    for ( int jy = 0 ; jy < m_precision ; ++jy ) {
                        for ( int jx = 0 ; jx < m_precision ; ++jx ) {
                            double r = m_pixel_sz *
                                    sqrt(pow(double(x+double(jx)/m_precision+m_offset)-double(w)/2.,2) +
                                         pow(double(y+double(jy)/m_precision+m_offset)-double(h)/2., 2));
                            for ( int c = 0 ; c < 3 ; ++c ) {
                                double sin_theta = r/m_f;
                                double xx = m_diam * sin_theta / lambda[c];
                                double res = QuantumRange;
                                if ( xx != 0 )
                                    res = m_I0 * pow(2*j1(M_PI*xx)/(M_PI*xx),2);
                                rgb[c] += res;
                            }
                        }
                    }
                    rgb[0]/=sq_p;
                    rgb[1]/=sq_p;
                    rgb[2]/=sq_p;
                    pixels[y*w*3+x*3+0] = rgb[0];
                    pixels[y*w*3+x*3+1] = rgb[1];
                    pixels[y*w*3+x*3+2] = rgb[2];
                    max = qMax(double(max),qMax(rgb[0],qMax(rgb[1], rgb[2])));
                }

            });
            double cor = double(QuantumRange)/max;
            dfl_parallel_for(y, 0, h, 4, (image), {
                if (aborted())
                    continue;
                emit progress(h/2+y/2, h);
                Magick::PixelPacket *pxl = cache->get(0,y,w,1);
                if ( m_error || !pxl ) {
                    if ( !m_error )
                        dflError(DF_NULL_PIXELS);
                    m_error = true;
                    continue;
                }
                for (int x = 0 ; x < w ; ++x ) {
                    if (m_outputHDR) {
                        pxl[x].red = toHDR(cor * pixels[y*w*3+x*3+0]);
                        pxl[x].green = toHDR(cor * pixels[y*w*3+x*3+1]);
                        pxl[x].blue = toHDR(cor * pixels[y*w*3+x*3+2]);
                    }
                    else {
                        pxl[x].red = clamp<quantum_t>(cor * pixels[y*w*3+x*3+0]);
                        pxl[x].green = clamp<quantum_t>(cor * pixels[y*w*3+x*3+1]);
                        pxl[x].blue = clamp<quantum_t>(cor * pixels[y*w*3+x*3+2]);
                    }
                }
                cache->sync();
            });
            delete[] pixels;
            if (aborted()) {
                emitFailure();
            }
            else {
                photo.setTag(TAG_NAME, "Airy Disk");
                if (m_outputHDR)
                    photo.setScale(Photo::HDR);
                outputPush(0, photo);
                emitSuccess();
            }
        }
    }
};

OpAiryDisk::OpAiryDisk(Process *parent) :
    Operator(OP_SECTION_FREQUENCY_DOMAIN, QT_TRANSLATE_NOOP("Operator", "Airy Disk"), Operator::NA, parent),
    m_diameter(new OperatorParameterSlider("diameter",tr("Aperture"), tr("Airy disk - Aperture in mm"), Slider::Value, Slider::Logarithmic, Slider::Real, 20, 1000, 200, 1, 10000, Slider::FilterPixels, this)),
    m_focal(new OperatorParameterSlider("focal",tr("Focal length"), tr("Airy disk - Focal length in mm"), Slider::Value, Slider::Logarithmic, Slider::Real, 200, 5000, 2000, 1, 100000, Slider::FilterPixels, this)),
    m_pixel_sz(new OperatorParameterSlider("pixelSize",tr("Pixel size"), tr("Airy disk - Pixel size in µm"), Slider::Value, Slider::Logarithmic, Slider::Real, 1, 32, 8.45, 0.001, 100, Slider::FilterPixels, this)),
    m_offset(new OperatorParameterSlider("offset",tr("Offset"), tr("Airy disk - Offset in fraction of pixels"), Slider::Value, Slider::Linear, Slider::Real, 0, 1, 0.5, 0, 1, Slider::FilterPixels, this)),
    m_width(new OperatorParameterSlider("width",tr("Width"), tr("Airy disk - Image width"), Slider::Value, Slider::Linear, Slider::Integer, 0, 1024, 512, 0, 65535, Slider::FilterPixels, this)),
    m_precision(new OperatorParameterSlider("precision",tr("Precision"), tr("Airy disk - Precision in subdivisions"), Slider::Value, Slider::Linear, Slider::Integer, 1, 10, 5, 1, 200, Slider::FilterPixels, this)),
    m_outputHDR(new OperatorParameterDropDown("outputHDR", tr("Output HDR"), this, SLOT(setOutputHDR(int)))),
    m_outputHDRValue(true)
{
    addOutput(new OperatorOutput(tr("Airy Disk"), this));

    m_outputHDR->addOption(DF_TR_AND_C("No"), false);
    m_outputHDR->addOption(DF_TR_AND_C("Yes"), true, true);

    addParameter(m_diameter);
    addParameter(m_focal);
    addParameter(m_pixel_sz);
    addParameter(m_offset);
    addParameter(m_width);
    addParameter(m_precision);
    addParameter(m_outputHDR);
}

OpAiryDisk::~OpAiryDisk()
{

}

OpAiryDisk *OpAiryDisk::newInstance()
{
    return new OpAiryDisk(m_process);
}

OperatorWorker *OpAiryDisk::newWorker()
{
    return new AiryDisk(m_diameter->value()/1000.,
                        m_focal->value()/1000.,
                        m_pixel_sz->value()/1000000.,
                        m_offset->value(),
                        m_width->value(),
                        m_precision->value(),
                        m_outputHDRValue,
                        m_thread, this);
}

void OpAiryDisk::setOutputHDR(int type)
{
    if ( m_outputHDRValue != !!type ) {
        m_outputHDRValue = !!type;
        setOutOfDate();
    }
}
