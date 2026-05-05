#include "CookingStation/Math/Geometry.h"
#include <cmath> // Dla std::abs

void Plane::Normalize() {
    float mag = glm::length(normal);

    // Zabezpieczenie przed dzieleniem przez zero (Epsilon test)
    // Zapobiega korupcji danych i powstawaniu wartoœci NaN w macierzach
    if (mag > 0.000001f) {
        normal /= mag;
        distance /= mag;
    }
}

Frustum ExtractFrustum(const glm::mat4& viewProj) {
    Frustum frustum;

    // Ekstrakcja Gribb-Hartmanna - wyci¹ganie p³aszczyzn bezpoœrednio z rzêdów macierzy View-Projection

    // Lewa p³aszczyzna (Left)
    frustum.leftFace.normal.x = viewProj[0][3] + viewProj[0][0];
    frustum.leftFace.normal.y = viewProj[1][3] + viewProj[1][0];
    frustum.leftFace.normal.z = viewProj[2][3] + viewProj[2][0];
    frustum.leftFace.distance = viewProj[3][3] + viewProj[3][0];

    // Prawa p³aszczyzna (Right)
    frustum.rightFace.normal.x = viewProj[0][3] - viewProj[0][0];
    frustum.rightFace.normal.y = viewProj[1][3] - viewProj[1][0];
    frustum.rightFace.normal.z = viewProj[2][3] - viewProj[2][0];
    frustum.rightFace.distance = viewProj[3][3] - viewProj[3][0];

    // Dolna p³aszczyzna (Bottom)
    frustum.bottomFace.normal.x = viewProj[0][3] + viewProj[0][1];
    frustum.bottomFace.normal.y = viewProj[1][3] + viewProj[1][1];
    frustum.bottomFace.normal.z = viewProj[2][3] + viewProj[2][1];
    frustum.bottomFace.distance = viewProj[3][3] + viewProj[3][1];

    // Górna p³aszczyzna (Top)
    frustum.topFace.normal.x = viewProj[0][3] - viewProj[0][1];
    frustum.topFace.normal.y = viewProj[1][3] - viewProj[1][1];
    frustum.topFace.normal.z = viewProj[2][3] - viewProj[2][1];
    frustum.topFace.distance = viewProj[3][3] - viewProj[3][1];

    // Bliska p³aszczyzna (Near)
    frustum.nearFace.normal.x = viewProj[0][3] + viewProj[0][2];
    frustum.nearFace.normal.y = viewProj[1][3] + viewProj[1][2];
    frustum.nearFace.normal.z = viewProj[2][3] + viewProj[2][2];
    frustum.nearFace.distance = viewProj[3][3] + viewProj[3][2];

    // Daleka p³aszczyzna (Far)
    frustum.farFace.normal.x = viewProj[0][3] - viewProj[0][2];
    frustum.farFace.normal.y = viewProj[1][3] - viewProj[1][2];
    frustum.farFace.normal.z = viewProj[2][3] - viewProj[2][2];
    frustum.farFace.distance = viewProj[3][3] - viewProj[3][2];

    // Obowi¹zkowa normalizacja wektorów, aby algorytm z rzutowaniem promienia (r) dzia³a³ poprawnie
    frustum.leftFace.Normalize();
    frustum.rightFace.Normalize();
    frustum.bottomFace.Normalize();
    frustum.topFace.Normalize();
    frustum.nearFace.Normalize();
    frustum.farFace.Normalize();

    return frustum;
}

bool IsOnFrustum(const Frustum& camFrustum, const AABB& aabb) {
    // Pakowanie p³aszczyzn do tablicy eliminuje ryzyko C++ Undefined Behavior 
    // wynikaj¹ce z mo¿liwego paddingu w strukturze Frustum.
    const Plane planes[6] = {
        camFrustum.topFace,
        camFrustum.bottomFace,
        camFrustum.rightFace,
        camFrustum.leftFace,
        camFrustum.farFace,
        camFrustum.nearFace
    };

    for (int i = 0; i < 6; ++i) {
        // Efektywny test promienia (Effective Radius Test)
        // Rzutujemy extents z AABB na wektor normalny aktualnie sprawdzanej p³aszczyzny
        const float r = aabb.extents.x * std::abs(planes[i].normal.x) +
            aabb.extents.y * std::abs(planes[i].normal.y) +
            aabb.extents.z * std::abs(planes[i].normal.z);

        // Znakowany dystans od œrodka AABB do p³aszczyzny
        const float d = glm::dot(planes[i].normal, aabb.center) + planes[i].distance;

        // Jeœli dystans jest mocniej na minusie ni¿ promieñ rzutu,
        // to oznacza, ¿e ca³y obiekt AABB le¿y po negatywnej ("niewidzialnej") stronie p³aszczyzny.
        // Mo¿emy natychmiast odrzuciæ obiekt (tzw. Early-Out).
        if (d < -r) {
            return false;
        }
    }

    // Jeœli przeszliœmy przez wszystkie 6 p³aszczyzn i ¿adna z nich nie wykluczy³a obiektu,
    // oznacza to, ¿e obiekt na pewno przetina siê z bry³¹ widzenia (lub jest ca³kowicie wewn¹trz).
    return true;
}
