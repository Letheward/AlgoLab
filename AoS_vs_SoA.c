typedef struct {
    char*  name; // where to put this makes HUGE difference!!! this is the BAD place, put this to the end will be more efficient
    double x;
    double y;
    double z;
} Point3D;

Point3D points_AoS[8192];

typedef struct {
    double x[8192];
    double y[8192];
    double z[8192];
    char*  names[8192];
} Point3DArray;

Point3DArray points_SoA;

int main() {

    #pragma omp parallel for
    for (int i = 0; i < 8192; i++) {
        for (int j = 0; j < 8192; j++) {
            points_AoS[j].x = points_AoS[j].y + points_AoS[j].z;
            points_SoA.x[j] = points_SoA.y[j] + points_SoA.z[j];
        }
    }
    return 0;
}

/* 

results


serial

loops 0.210s
AoS 0.589s
SoA 0.298s


parallel

loops 0.072s
AoS 0.339s
SoA 0.145s


*/
