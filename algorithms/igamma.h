#ifndef IGAMMA_H
#define IGAMMA_H

#include "lutbased.h"

#define SRGB_G 2.4L
#define SRGB_N 0.00304L


class iGamma : public LutBased
{
    Q_OBJECT
public:
    explicit iGamma(qreal gamma, qreal x0, bool invert = false, QObject *parent = 0);
    static iGamma& sRGB();
    static iGamma& BT709();
    static iGamma& Lab();
    static iGamma& reverse_sRGB();
    static iGamma& reverse_BT709();
    static iGamma& reverse_Lab();

    void applyOn(Photo &photo);

private:
    qreal m_gamma;
    qreal m_x0;
    bool m_invert;
};

#endif // IGAMMA_H
