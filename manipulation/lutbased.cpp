#include "lutbased.h"
#include <Magick++.h>

using Magick::Quantum;

LutBased::LutBased(QObject *parent) :
    Manipulation(parent),
    m_lut(new quantum_t[QuantumRange+1])
{

}

LutBased::~LutBased()
{
    delete[] m_lut;
}

void LutBased::applyOn(Photo &photo)
{
    Magick::Image &image =  *photo.image();
    image.modifyImage();

    int h = image.rows(),
            w = image.columns();
    Magick::Pixels pixel_cache(image);

#pragma omp parallel for
    for ( int y = 0 ; y < h ; ++y ) {
        Magick::PixelPacket *pixels = pixel_cache.get(0,y,w,1);
        if ( !pixels ) continue;
        for ( int x = 0 ; x < w ; ++x ) {
            pixels[x].red=m_lut[pixels[x].red];
            pixels[x].green=m_lut[pixels[x].green];
            pixels[x].blue=m_lut[pixels[x].blue];
        }
    }
#pragma omp barrier
    pixel_cache.sync();
}