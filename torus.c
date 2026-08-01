#include <_time.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define H 128
#define W 128
#define R 42.
#define r 18.
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

typedef uint8_t b;
typedef float f;
typedef int i;
typedef char c;

/*
 * 4 different channels because
 * every RGB (3 bytes) point produces
 * 4 chars.
 *
 * This is retro-compatible with
 * the first approach because we can just use
 * the first channel as a single BW image
 * */
static c screen[W * H * 4];
static f zbuf[W * H];
c light[] = " .:-=+*#%@";
struct timespec t0;
struct timespec t1;
f fps = 0;
long elapsed = 0;

void update_fps(){
    clock_gettime(CLOCK_MONOTONIC, &t1);

    elapsed =
        (t1.tv_sec - t0.tv_sec) * 1000000L +
        (t1.tv_nsec - t0.tv_nsec) / 1000L;

    t0 = t1;

    if (elapsed > 0) {
        f instant_fps = 1000000.0f / (elapsed < 30000 ? 30000 : elapsed);
        fps = 0.5f * fps + 0.5f * instant_fps;
    }
}

/*
 * base64 map
 * */
c map[] = {
     'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 
     'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 
     'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 
     'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 
     'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 
     'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 
     'w', 'x', 'y', 'z', '0', '1', '2', '3', 
     '4', '5', '6', '7', '8', '9', '+', '/'
};

/*
 * simple implementation
 * of base-64 encoding given 3 bytes.
 * */
static inline void encode_base64(b red, b green, b blue,char *a, char *b, char *c, char *d) {
    *a = map[red >> 2];
    *b = map[((red & 0x03) << 4) | (green >> 4)];
    *c = map[((green & 0x0F) << 2) | (blue >> 6)];
    *d = map[blue & 0x3F];
}

/*
 * function that prints chunked
 * base64 encoded RGB image
 * */
static void print_kitty(void) {
    const i size = H * W * 4;

    printf("\033[2J\033[H");
    for (i i = 0; i < size; i += 4096) {
        int chunk_size = min(4096, size - i);
        int more = i + chunk_size < size;

        if (i == 0)
            printf("\033_Ga=T,f=24,s=%d,v=%d,c=%d,r=%d,m=%d;", W, H, 50, 25, more);
        else
            printf("\033_Gm=%d;", more);

        fwrite(screen + i, 1, chunk_size, stdout);
        printf("\033\\");
    }
}

void print_screen() {
    /* 1. clear screen */
    printf("\033[2J\033[H");
    /* 2. print content of screen */
    for (i i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            putchar(screen[i * W + j]);
        }
        putchar('\n');
    }
}

int main() {

    /* rotation angles (y and z axis) */
    f phi = 0.0f, theta = 0.0f;
    
    /*
     * ligth1 + color1
     * */
    f lx1 = sqrt(4), ly1 = sqrt(4), lz1 = sqrt(4);
    i rl1 = 255, gl1 = 0, bl1 = 0;

    /*
     * ligth1 + color1
     * */
    f lx2 = 0, ly2 = sqrt(2), lz2 = sqrt(2);
    i rl2 = 0, gl2 = 0, bl2 = 255;

    for (;;) {

        clock_gettime(CLOCK_MONOTONIC, &t0);

        /* for each point of the torus compute its
         * position and light */
        for (i i = 0; i < H * W; i++)
            zbuf[i] = -INFINITY;
#ifdef _PRINT_KITTY
        memset(screen,'A', sizeof screen);
#else
        memset(screen,' ', sizeof screen / 4);
#endif
        for (f u = 0.0f; u <= 2 * M_PI; u += M_PI / 256.) {
            for (f v = 0.0f; v <= 2 * M_PI; v += M_PI / 256.) {
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

                x = x0 * ct + z0 * st; 
#ifndef _PRINT_KITTY
                x *= 2;
#endif
                y = x0 * sp * st + y0 * cp - z0 * sp * ct;
                z = -x0 * cp * st + y0 * sp + z0 * cp * ct;

                /* compute normal*/
                f nx0 = cv * cu,
                  ny0 = cv * su,
                  nz0 = sv;

                f nx = nx0 * ct + nz0 * st,
                  ny = nx0 * sp * st + ny0 * cp - nz0 * sp * ct,
                  nz = -nx0 * cp * st + ny0 * sp + nz0 * cp * ct;

                f l1 = nx * lx1 + ny * ly1 + nz * lz1;
                if (l1 < 0)
                    l1 = 0;

                f l2 = nx * lx2 + ny * ly2 + nz * lz2;
                if (l2 < 0)
                    l2 = 0;


                /* convert x, y, z to screen coordinates */
                i xi = x + ((f)W / 2),
                  yi = y + ((f)H / 2),
                  zi = (i)z;

                /* take only pixel that is visible */
                if (xi < 0 || xi >= W || yi < 0 || yi >= H || z < zbuf[yi * W + xi])
                    continue;

                zbuf[yi * W + xi] = z;
#ifdef _PRINT_KITTY
                i index = (yi * W + xi) * 4;
                i red = min(255,rl1 * l1 + rl2 * l2);
                i green = min(255,gl1 * l1 + gl2 * l2);
                i blue = min(255,bl1 * l1 + bl2 * l2);
                encode_base64(red, green, blue, screen + index, screen + index + 1, screen + index + 2, screen + index + 3);
#else
                /*
                 * compute light for the single point.
                 * */
                c p = light[ min(9,(i)(l2 * 10)) ];
                /* 
                 * write pixel on screen
                 * */
                screen[yi * W + xi] = p;
#endif

            }
        }

#ifdef _PRINT_KITTY
        print_kitty();
#else
        print_screen();
#endif
        
        update_fps();
        printf("\nFPS: [%.2f]\n", fps);

        // usleep(30000);
        if (elapsed < 30000)
            usleep(30000 - elapsed);

        fflush(stdout);


        /* increase rotation */
        phi += M_PI / 48.;
        theta += M_PI / 32.;
    }

    return 0;
}
