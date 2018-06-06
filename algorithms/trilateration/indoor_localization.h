#ifndef INDOOR_LOCALIZATION_H
#define INDOOR_LOCALIZATION_H
#include "point.h"
#include <QList>

class trilateration
{
    public:

    point point1, point2, point3 , coordinate;

    float r1, r2, r3;

    float norm (point p);

    void Trilateration(void);

    void modify_points(point a1 , point b2, point c3);

    void modify_distance(float a1 , float b2, float c3);

    void display_cordinates (void);

    void distanceConversion (float *distanceList);
};

#endif // INDOOR_LOCALIZATION_H
