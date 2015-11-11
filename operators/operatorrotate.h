#ifndef OPERATORROTATE_H
#define OPERATORROTATE_H

#include "operator.h"
#include <QObject>

class OperatorParameterDropDown;

class OperatorRotate : public Operator
{
    Q_OBJECT
public:
    OperatorRotate(Process *parent);
    ~OperatorRotate();

    OperatorRotate *newInstance();
    OperatorWorker *newWorker();
    qreal angle() const;

private slots:
    void set0();
    void set90();
    void set180();
    void set270();

private:
    OperatorParameterDropDown *m_dropdown;
    qreal m_angle;
};

#endif // OPERATORROTATE_H
