#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>
#include "indoor_localization.h"
#include <QDebug>
using namespace std;


float trilateration::norm (point p)
{
        return pow(pow(p.x,2)+pow(p.y,2),.5);
}


void trilateration::Trilateration(void)
{
        //point coordinate;
        //unit vector in a direction from point1 to point 2
        double p2p1Distance = pow(pow(point2.x - point1.x,2) + pow(point2.y - point1.y, 2),0.5);
        point ex = {(point2.x-point1.x)/(float)p2p1Distance, (point2.y-point1.y)/(float)p2p1Distance};
        point aux = {point3.x-point1.x,point3.y-point1.y};
        //signed magnitude of the x component
        float i = ex.x * aux.x + ex.y * aux.y;
        //the unit vector in the y direction.
        point aux2 = { point3.x-point1.x-i*ex.x, point3.y-point1.y-i*ex.y};
        point ey = { aux2.x / norm (aux2), aux2.y / norm (aux2) };
        //the signed magnitude of the y component
        double j = ey.x * aux.x + ey.y * aux.y;
        //coordinates
        double x = (pow(this->r1,2) - pow(r2,2) + pow(p2p1Distance,2))/ (2 * p2p1Distance);
        double y = (pow(r1,2) - pow(r3,2) + pow(i,2) + pow(j,2))/(2*j) - i*x/j;
        //result coordinates
        double finalX = point1.x+ x*ex.x + y*ey.x;
        double finalY = point1.y+ x*ex.y + y*ey.y;
        coordinate.x = finalX;
        coordinate.y = finalY;

}


void trilateration::modify_points(point a1 , point b2, point c3)
{
        point1.x = a1.x ;
        point2.x = b2.x ;
        point3.x = c3.x ;

        point1.y = a1.y ;
        point2.y = b2.y ;
        point3.y = c3.y ;
}


void trilateration::modify_distance(float a1 , float b2, float c3)
{
         r1 = a1 ;
         r2 = b2 ;
         r3 = c3 ;
}


void trilateration::display_cordinates (void)
{
        cout<<"X:::  "<< coordinate.x  <<endl;
        cout<<"Y:::  "<< coordinate.y <<endl;
}

void trilateration::distanceConversion (float * distanceList)
{
    for( int index = 0; index < 3; index++ )
    {
    //   qDebug() << "before conersion" << distanceList[index];
       distanceList[index] = sqrt( pow(distanceList[index], 2 ) - 2.25  );
     //  qDebug() << "after conersion" << distanceList[index];
    }
}




