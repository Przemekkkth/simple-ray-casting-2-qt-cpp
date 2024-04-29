#ifndef SOBJECT_H
#define SOBJECT_H
#include <QImage>
struct sObject
{
    float x;
    float y;
    float vx;
    float vy;
    bool bRemove;
    QImage sprite;
    sObject(float _x, float _y, float _vx, float _vy, bool _bRemove, QImage _sprite)
        : x(_x), y(_y), vx(_vx), vy(_vy), bRemove(_bRemove), sprite(_sprite)
    {

    }
};

#endif // SOBJECT_H
