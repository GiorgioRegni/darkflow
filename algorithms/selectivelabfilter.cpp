#include "selectivelabfilter.h"
#include "hdr.h"
#include "cielab.h"
#include "ports.h"
#include "console.h"

SelectiveLabFilter::SelectiveLabFilter(int hue,
                                       int coverage,
                                       qreal saturation,
                                       bool strict,
                                       qreal exposure,
                                       bool insideSelection,
                                       bool exposureStrict,
                                       QObject *parent) :
    Algorithm(false,parent),
    m_hue(hue),
    m_coverage(coverage),
    m_saturation(saturation),
    m_strict(strict),
    m_exposure(exposure),
    m_insideSelection(insideSelection),
    m_exposureStrict(exposureStrict)

{
}

void SelectiveLabFilter::applyOnImage(Magick::Image &image, bool hdr)
{
    int     h = image.rows(),
            w = image.columns();
    Magick::Image srcImage(image);
    ResetImage(image);
    Magick::Pixels src_cache(srcImage);
    Magick::Pixels pixel_cache(image);

    double saturation = m_saturation;
    double value = m_exposure;
    double bias_sat = m_strict?0:1;
    double bias_val = m_exposureStrict?0:1;
    bool inv_val = !m_insideSelection;
    bool inv_sat=false;
    if ( m_coverage < 0 ) {
        m_coverage = -m_coverage;
        inv_sat = true;
        inv_val = !inv_val;
    }

    if ( inv_sat ) {
        if ( inv_val ) {
            if ( m_coverage == 0 || m_coverage == 360)
                value = 1;
        }
    }
    else {
        if ( inv_val ) {
            if ( m_coverage == 360 )
                value = 1;
        }
        else {
            if ( m_coverage == 0 )
                value = 1;
        }
    }

    //calcul de la puissance de largeur
    double theta = M_PI - M_PI * (double(m_coverage)/2) / 180.;
    double puissance = 0;
    if ( m_coverage != 0 ) {
        puissance = (log(-M_LN2/(log(-(cos(theta)-1.)/2.)))+2.*M_LN2)/(M_LN2);
        puissance = pow(2.,puissance);
    }
    //calcul de l'angle d'application
    theta = M_PI * double((360+m_hue)%360)/180.;

    bool error = false;
#pragma omp parallel for dfl_threads(4, srcImage, image)
    for ( int y = 0 ; y < h ; ++y ) {
        const Magick::PixelPacket *src = src_cache.getConst(0,y,w,1);
        Magick::PixelPacket *pixels = pixel_cache.get(0,y,w,1);
        if ( error || !src || !pixels ) {
            if ( !error )
                dflError(DF_NULL_PIXELS);
            error = true;
            continue;
        }
        for ( int x = 0 ; x < w ; ++x ) {

            double rgb[3];
            if (hdr) {
                rgb[0] = fromHDR(src[x].red);
                rgb[1] = fromHDR(src[x].green);
                rgb[2] = fromHDR(src[x].blue);
            }
            else {
                rgb[0] = src[x].red;
                rgb[1] = src[x].green;
                rgb[2] = src[x].blue;
            }

            double lab[3];

            RGB_to_LinearLab(rgb,lab);

            double module = sqrt(lab[1]*lab[1]+lab[2]*lab[2]);
            double arg;
            if ( lab[2] > 0 )
                arg = M_PI/2-atan(-lab[1]/lab[2]);
            else
                arg = -M_PI/2-atan(-lab[1]/lab[2]);

            double mul= pow((1.-cos(arg+theta))/2.,puissance);
            double mul_sat = mul;
            double mul_val = mul;
            if ( inv_sat ) {
                mul_sat=1.-mul_sat;
            }
            if ( inv_val ) {
                mul_val=1.-mul_val;
            }

            double v = lab_linearize(lab[0]);
            if ( bias_val == 0 )
                v = v * mul_val * value;
            else {
                /*
                 * correction handles low saturation zone by enlarging selection
                 * to prevent discontinuity
                 */
                double correction = clamp<double>(pow(module/DF_MAX_AB_MODULE,.15), 0, 1);
                v = v * mul_val * value + v * (1-mul_val) * pow(value,1-correction);
            }

            lab[0] = lab_gammaize(v);

            if ( bias_sat == 0 )
                module = module * mul_sat * saturation;
            else
                module = module * mul_sat * saturation + module * (1-mul_sat);

            lab[1]= -module * cos(arg);
            lab[2]=  module * sin(arg);

            LinearLab_to_RGB(lab,rgb);

            if ( hdr) {
                pixels[x].red=toHDR(rgb[0]);
                pixels[x].green=toHDR(rgb[1]);
                pixels[x].blue=toHDR(rgb[2]);
            }
            else {
                pixels[x].red=rgb[0];
                pixels[x].green=rgb[1];
                pixels[x].blue=rgb[2];
            }
        }
        pixel_cache.sync();
    }
}
