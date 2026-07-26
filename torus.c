#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#define H 30
#define W 60
#define R 6.
#define r 2.

typedef float f;
typedef int i;
typedef char c;

char screen[W * H];
float zbuf[W * H];

char light[] = " .:-=+*#%@";

void print_screen() {
    /* 1. clear screen */
    printf("\033[2J\033[H");
    /* 2. print content of screen */
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            putchar(screen[i * W + j]);
        }
        putchar('\n');
    }
    printf("\033[H");
    fflush(stdout);
}

int main() {

    /* rotation angles (y and z axis) */
    f phi = 0.0f, theta = 0.0f;
    
    /* light vector */
    f lx = 0, ly = 0, lz = 1;

    for (;;) {

        /* for each point of the torus compute its
         * position and light */
        for (int i = 0; i < H * W; i++)
            zbuf[i] = -INFINITY;
        memset(screen,' ', sizeof screen);
        for (f u = 0.0f; u <= 2 * M_PI; u += M_PI / 32.) {
            for (f v = 0.0f; v <= 2 * M_PI; v += M_PI / 32.) {
                /*
                 * x = ( R + r cosv ) cos u
                 * y = ( R + r cosv ) sin u
                 * z = r rinv
                 * */
                f cu = cosf(u),
                  cv = cosf(v),
                  su = sinf(u),
                  sv = sinf(v),
                  cp = cosf(phi), sp = sinf(phi),
                  ct = cosf(theta), st = sinf(theta);

                /* compute point */
                f x = (R + r * cv ) * cu,
                  y = ( R + r * cv ) * su,
                  z = r * sv;

                f x0 = x,
                  y0 = y,
                  z0 = z;

                x = x0 * ct + z0 * st; x *= 2;
                y = x0 * sp * st + y0 * cp - z0 * sp * ct;
                z = -x0 * cp * st + y0 * sp + z0 * cp * ct;

                /* compute normal*/
                f nx = cv * cu,
                  ny = cv * su,
                  nz = sv;

                nx = nx * ct + nz * st;
                ny = nx * sp * st + ny * cp - nz * sp * ct;
                nz = -nx * cp * st + ny * sp + nz * cp * ct;

                f l = nx * lx + ny * ly + nz * lz;
                if (l < 0)
                    continue;

                c p = light[ (i)(l * 10 - 0.01f) % 10 ];

                /* convert x, y, z to screen coordinates */
                i xi = x + ((f)W / 2),
                  yi = y + ((f)H / 2),
                  zi = (i)z;

                /* take only pixel that is visible */
                if (xi < 0 || xi >= W || yi < 0 || yi >= H || z < zbuf[yi * W + xi])
                    continue;

                zbuf[yi * W + xi] = z;

                /* write pixel on screen */
                screen[yi * W + xi] = p;
            }
        }


        print_screen();
        usleep(33000);

        /* increase rotation */
        phi += M_PI / 32.;
        theta += M_PI / 32.;
    }

    return 0;
}
