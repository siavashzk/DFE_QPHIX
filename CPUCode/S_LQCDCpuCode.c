#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"

#define TEST_MAIN
#include "su3.h"
#include "global.h"

void create_random_input(spinor* s, su3* u);
void create_random_spinor(spinor * s);
void create_random_su3vector(su3_vector *v);
void create_random_su3(su3 *s);
void create_random_complex(float complex *a);
void add_4d_halos_spinor(spinor* with_halos, spinor* orig, int halos, int eo);
void add_4d_halos_gauge(su3* with_halos, su3* orig, int halos, int eo) ;
int AreNotSameComplex(complex float a, float complex b);
int compare_spinor (spinor *a, spinor *b);
void read_spinor(char * filename, spinor *out);
void read_gauge(char * filename, su3 *s);
void reorganize_ueven (su3 *out, su3 *in);
void reorganize_back_ueven (su3 *out, su3 *in);
void devide_gauge_to_oddeven(su3 const * const in, su3 * const even, su3 * const odd, int ieo);
void add_1d_halos_spinor(spinor* with_halos, spinor* orig, int halos);
void add_1d_halos_gauge(su3* with_halos, su3* orig, int halos);
void print_spinors (spinor* s);
void print_gauges (su3* s);
int verify_results (spinor* dfe_out, spinor *expected_out, int V);

int main(void) {

	T = S_LQCD_T;
	LX = S_LQCD_LX;
	LY = S_LQCD_LY;
	LZ  = S_LQCD_LZ;
	VOLUME = LZ * LY * LX * T;
	int VOLUMEH1 = (LX/2) * (LY) * (LZ) * (T+4);
	int VOLUMEH2 = (LX/2) * (LY) * (LZ) * (T+2);

	spinor *in, *out, *out_dfe, *out_expected, *tmp;
	su3 *uodd0, *ueven0, *u0;
	su3 *uodd1, *ueven1, *u1;

	printf("Allocating memory for data ... \n");

	in = malloc(VOLUME * sizeof(spinor));
	out = &in[VOLUME / 2];
	out_dfe = malloc(VOLUMEH1 * sizeof(spinor));
	out_expected = malloc(VOLUME/2 * sizeof(spinor));


	tmp = malloc(VOLUME/2 * sizeof(spinor));

	ueven0 = malloc(VOLUME * 4 * sizeof(su3));
	uodd0 = &ueven0[VOLUME / 2 * 4];
	u0 = malloc (VOLUME / 2 * 8 * sizeof(su3));
	ueven1 = malloc(VOLUME * 4 * sizeof(su3));
	uodd1 = &ueven1[VOLUME / 2 * 4];
	u1 = malloc (VOLUME / 2 * 8 * sizeof(su3));

	spinor *in_halos  = malloc(VOLUMEH1 * sizeof(spinor));
	su3 *uodd0_halos   = malloc(VOLUMEH2 * 4 * sizeof(spinor));
	su3 *ueven0_halos  = malloc(VOLUMEH2 * 4 * sizeof(spinor));
	//spinor *out_halos = malloc(VOLUMEH2 * sizeof(spinor));

	su3 *uodd1_halos   = malloc(VOLUMEH1 * 4 * sizeof(spinor));
	su3 *ueven1_halos  = malloc(VOLUMEH1 * 4 * sizeof(spinor));

	printf("Done!\n");

	/*printf("Creating random spinor and gauge inputs ... \n");

	for (int i=0 ; i<VOLUME/2 ; i++ ) {
		create_random_spinor(in + i);
	}
	for (int i=0 ; i<VOLUME*2 ; i++ ) {
		create_random_su3(ueven + i);
		create_random_su3(uodd + i);
	}

	printf("Done!\n");*/

	printf("Reading spinor and gauge inputs ... \n");

	read_spinor("tmp_spinor.txt", tmp);
	read_spinor("in_spinor.txt", in);
	read_spinor("out_spinor.txt", out_expected);
	read_gauge("in_gauge0.txt", u0);
	read_gauge("in_gauge1.txt", u1);

	printf("Done!\n");
	printf("Data reordering and adding necessary halos ... \n");

	devide_gauge_to_oddeven(u0, ueven0, uodd0, 0);
	devide_gauge_to_oddeven(u1, ueven1, uodd1, 1);

	add_1d_halos_spinor(in_halos,   in,    2);
	add_1d_halos_gauge(uodd1_halos,  uodd1,  2);
	add_1d_halos_gauge(ueven1_halos, ueven1, 2);

	add_1d_halos_gauge(uodd0_halos,  uodd0,  1);
	add_1d_halos_gauge(ueven0_halos, ueven0, 1);

	printf("Done!\n");
	printf("Setting up DFE SLIC  ... \n");

	max_file_t *maxfile = S_LQCD_init();
	max_engine_t *engine = max_load(maxfile, "*");

	max_actions_t* act = max_actions_init(maxfile, "default");

	max_set_double(act, "times1kernel", "beta_s", -.5);
	max_set_double(act, "times1kernel", "beta_t_b", 0.3);
	max_set_double(act, "times1kernel", "beta_t_f", 0.3);

	max_set_double(act, "sub1kernel", "alpha", 4.1);
	max_set_double(act, "sub1kernel", "beta_s", -.5 / 16.4);
	max_set_double(act, "sub1kernel", "beta_t_b", 0.3 / 16.4);
	max_set_double(act, "sub1kernel", "beta_t_f", 0.3 / 16.4);

	max_queue_input(act, "times1kernel_spinor_in", in_halos, VOLUMEH1 * sizeof(spinor));
	max_queue_input(act, "times1kernel_gauge0", uodd1_halos, VOLUMEH1 * 4 * sizeof(su3));
	max_queue_input(act, "times1kernel_gauge1", ueven1_halos, VOLUMEH1 * 4 * sizeof(su3));

	max_queue_input(act, "sub1kernel_gauge0", uodd0_halos, VOLUMEH2 * 4 * sizeof(su3));
	max_queue_input(act, "sub1kernel_gauge1", ueven0_halos, VOLUMEH2 * 4 * sizeof(su3));

	max_queue_output(act, "sub1kernel_spinor_out", out_dfe,  VOLUME/2 * sizeof(spinor));

	printf("Done!\n");
	printf("Running LQCD on DFE ...\n");

	max_run(engine, act);
	max_unload(engine);

	printf("Done!\n");
	printf("Verifying LQCD output ...\n");

	return  verify_results(out_dfe, out_expected, VOLUME/2);
}

int verify_results (spinor* dfe_out, spinor *expected_out, int V) {
	for (int i=0 ; i<V ; i++ ) {
		int error = compare_spinor(expected_out+i , dfe_out+i);
		if (error) {
			printf("Wrong %d! %d\n", error,i);
			spinor *a = expected_out + i;
			spinor *b = dfe_out + i;
			printf("%f %f    %f %f ", creal(a->s0.c0), cimag(a->s0.c0),
					                  creal(b->s0.c0), cimag(b->s0.c0));
			return 1;
		}
	}
	printf("Good.\n");
	return 0;

}

void print_cmplx(float * a, int N) {
	for (int i = 0; i < N; i += 2) {
		printf("(%f,%f i)\n", a[i], a[i + 1]);
	}
}

void create_random_input(spinor* s, su3* u) {
	for (int i = 0; i < VOLUME / 2; i++) {
		create_random_spinor(s + i);
	}
	for (int i = 0; i < VOLUME * 4; i++) {
		create_random_su3(u + i);
	}
}

void create_random_spinor(spinor * s) {
	create_random_su3vector(&s->s0);
	create_random_su3vector(&s->s1);
	create_random_su3vector(&s->s2);
	create_random_su3vector(&s->s3);
}

void create_random_su3vector(su3_vector *v) {
	create_random_complex(&v->c0);
	create_random_complex(&v->c1);
	create_random_complex(&v->c2);
}

void create_random_su3(su3 *s) {
	create_random_complex(&s->c00);
	create_random_complex(&s->c01);
	create_random_complex(&s->c02);
	create_random_complex(&s->c10);
	create_random_complex(&s->c11);
	create_random_complex(&s->c12);
	create_random_complex(&s->c20);
	create_random_complex(&s->c21);
	create_random_complex(&s->c22);
}

void create_random_complex(float complex *a) {
	float r[2];
	for (int i = 0; i < 2; i++) {
		r[i] = (float) (rand()) / RAND_MAX * 2;
	}
	*a = r[0] + I * r[1];
}

void add_1d_halos_spinor(spinor* with_halos, spinor* orig, int halos) {

	for (int t = -halos ; t < T+halos ; t++ ) {
		for (int z = 0 ; z < LZ ; z++ ) {
			for (int y = 0 ; y < LY ; y++ ) {
				for (int x = 0 ; x < LX/2 ; x++ ) {
					int tt = (t+T)%T;

					with_halos[ ((( (t+halos) * LZ + z) * LY + y) * LX/2 ) + x] =
							orig[ (((tt*LZ + z) * LY + y) * LX/2 ) + x];
				}
			}
		}
	}

}

void add_1d_halos_gauge(su3* with_halos, su3* orig, int halos) {

	for (int t = -halos ; t < T+halos ; t++ ) {
		for (int z = 0 ; z < LZ ; z++ ) {
			for (int y = 0 ; y < LY ; y++ ) {
				for (int x = 0 ; x < LX/2 ; x++ ) {
					int tt = (t+T)%T;

					for (int i=0 ; i < 4 ; i++ ){
						with_halos[ (((( (t+halos) * LZ + z) * LY + y) * LX/2 ) + x) * 4 + i] =
								orig[ ((((tt*LZ + z) * LY + y) * LX/2 ) + x) * 4 + i];
					}
				}
			}
		}
	}

}


void add_4d_halos_spinor(spinor* with_halos, spinor* orig, int halos, int eo) {
	int lhx, lhy, lhz;
	lhx = LX+2*halos;
	lhy = LY+2*halos;
	lhz = LZ/2+halos;

	eo = eo ^ 1;

	for (int t = -halos ; t < T+halos ; t++ ) {
		for (int x = -halos ; x < LX+halos ; x++ ) {
			for (int y = -halos ; y < LY+halos ; y++ ) {
				for (int z = -(halos/2) ; z < LZ/2+(halos+1)/2 ; z++ ) {
					int tt = (t+T)%T;
					int xx = (x+LX)%LX;
					int yy = (y+LY)%LY;
					int isOddRow = (tt & 1) ^ (xx & 1) ^ (yy & 1) ^ eo;
					int zz;
					if (halos%2 == 0) {
						zz = (z+LZ/2)%(LZ/2);
					} else {
						zz = (z+LZ*2-isOddRow*halos)%(LZ/2);
					}

					with_halos[ (((( (t+halos) * lhx +
					                 (x+halos)) * lhy +
					                 (y+halos)) * lhz ) +
					                 (z+halos/2))] =
							orig[ (((tt*LX + xx) * LY + yy) * LZ/2 ) + zz];
				}
			}
		}
	}
}

void add_4d_halos_gauge(su3* with_halos, su3* orig, int halos, int eo) {
	int lhx, lhy, lhz;
	lhx = LX+2*halos;
	lhy = LY+2*halos;
	lhz = LZ/2+halos;

	eo = eo ^ 1;

	for (int t = -halos ; t < T+halos ; t++ ) {
		for (int x = -halos ; x < LX+halos ; x++ ) {
			for (int y = -halos ; y < LY+halos ; y++ ) {
				for (int z = -(halos/2) ; z < LZ/2+(halos+1)/2 ; z++ ) {
					int tt = (t+T)%T;
					int xx = (x+LX)%LX;
					int yy = (y+LY)%LY;
					int isOddRow = (tt & 1) ^ (xx & 1) ^ (yy & 1) ^ eo;
					int zz;
					if (halos%2 == 0) {
						zz = (z+LZ/2)%(LZ/2);
					} else {
						zz = (z+LZ*2-isOddRow*halos)%(LZ/2);
					}

					for (int i=0 ; i < 4 ; i++ ){
						with_halos[ ((((( (t+halos) * lhx +
				                          (x+halos)) * lhy +
				                          (y+halos)) * lhz ) +
				                          (z+halos/2)) * 4) + i] =
						      orig[ ((((tt*LX + xx) * LY + yy) * LZ/2 ) + zz) * 4 + i];
					}
				}
			}
		}
	}
}

int AreNotSameComplex(complex float a, float complex b)
{
    return ( fabs(creal(a) - creal(b)) > 0.001 ) ||
    	   ( cimag(creal(a) - cimag(b)) > 0.001 );
}

int compare_spinor (spinor *a, spinor *b) {
	if (AreNotSameComplex(a->s0.c0 ,b->s0.c0)) return 1;
	if (AreNotSameComplex(a->s0.c1 ,b->s0.c1)) return 2;
	if (AreNotSameComplex(a->s0.c2 ,b->s0.c2)) return 3;
	if (AreNotSameComplex(a->s1.c0 ,b->s1.c0)) return 4;
	if (AreNotSameComplex(a->s1.c1 ,b->s1.c1)) return 5;
	if (AreNotSameComplex(a->s1.c2 ,b->s1.c2)) return 6;
	if (AreNotSameComplex(a->s2.c0 ,b->s2.c0)) return 7;
	if (AreNotSameComplex(a->s2.c1 ,b->s2.c1)) return 8;
	if (AreNotSameComplex(a->s2.c2 ,b->s2.c2)) return 9;
	if (AreNotSameComplex(a->s3.c0 ,b->s3.c0)) return 10;
	if (AreNotSameComplex(a->s3.c1 ,b->s3.c1)) return 11;
	if (AreNotSameComplex(a->s3.c2 ,b->s3.c2)) return 12;
	return 0;
}

void read_spinor(char * filename, spinor *out) {
	FILE * fptr = fopen(filename, "r" );
	char temp[64];

	for (int t=0 ; t<T ; t++ ) {
		for (int z=0 ; z<LZ ; z++ ) {
			for (int y=0 ; y<LY ; y++ ) {
				for (int x=0 ; x<LX/2 ; x++ ) {

					/*int isOddRow = (t & 1) ^ (z & 1) ^ (y & 1);
					int tt = t;               // converting from checkerboarded
					int zz = z/2;             // coordinates of qphix along x-axis
					int yy = y;               // to tmLQCD checkerboarding along
					int xx = (2*x)+isOddRow;  // along y-axis*/

					spinor *s = &out [ (((((t*LZ)+z)*LY)+y)*LX/2)+x ];

					fgets (temp, 64, fptr);
					float a, b;

					fscanf(fptr, "%f %f", &a, &b);
					s->s0.c0 = a + I * b;
					fscanf(fptr, "%f\n%f\n", &a, &b);
					s->s0.c1 = a + I * b;
					fscanf(fptr, "%f\n%f\n", &a, &b);
					s->s0.c2 = a + I * b;

					fscanf(fptr, "%f\n%f\n", &a, &b);
					s->s1.c0 = a + I * b;
					fscanf(fptr, "%f\n%f\n", &a, &b);
					s->s1.c1 = a + I * b;
					fscanf(fptr, "%f\n%f\n", &a, &b);
					s->s1.c2 = a + I * b;

					fscanf(fptr, "%f\n%f\n", &a, &b);
					s->s2.c0 = a + I * b;
					fscanf(fptr, "%f\n%f\n", &a, &b);
					s->s2.c1 = a + I * b;
					fscanf(fptr, "%f\n%f\n", &a, &b);
					s->s2.c2 = a + I * b;

					fscanf(fptr, "%f\n%f\n", &a, &b);
					s->s3.c0 = a + I * b;
					fscanf(fptr, "%f\n%f\n", &a, &b);
					s->s3.c1 = a + I * b;
					fscanf(fptr, "%f\n%f\n", &a, &b);
					s->s3.c2 = a + I * b;

				}
			}
		}
	}
	fclose(fptr);
}

void read_gauge(char * filename, su3 *s) {
	FILE * fptr = fopen(filename, "r" );
	char temp[64];
	for (int i = 0 ; i < VOLUME/2 * 8 ; i++ ) {
		fgets (temp, 64, fptr);
		float a, b;

		fscanf(fptr, "%f\n%f\n", &a, &b);
		s->c00 = a + I * b;
		fscanf(fptr, "%f\n%f\n", &a, &b);
		s->c01 = a + I * b;
		fscanf(fptr, "%f\n%f\n", &a, &b);
		s->c02 = a + I * b;

		fscanf(fptr, "%f\n%f\n", &a, &b);
		s->c10 = a + I * b;
		fscanf(fptr, "%f\n%f\n", &a, &b);
		s->c11 = a + I * b;
		fscanf(fptr, "%f\n%f\n", &a, &b);
		s->c12 = a + I * b;

		fscanf(fptr, "%f\n%f\n", &a, &b);
		s->c20 = a + I * b;
		fscanf(fptr, "%f\n%f\n", &a, &b);
		s->c21 = a + I * b;
		fscanf(fptr, "%f\n%f\n", &a, &b);
		s->c22 = a + I * b;

		s++;
	}
	fclose(fptr);
}

void reorganize_ueven (su3 *out, su3 *in) {
	int i = 0;
	for (int t=0 ; t<T ; t++ ) {
		for (int x=0 ; x<LX ; x++ ) {
			for (int y=0 ; y<LY ; y++ ) {
				for (int z=0 ; z<LZ/2 ; z++ ) {
					int isOddRow = (t & 1) ^ (x & 1) ^ (y & 1);
					for (int mu=0; mu<4 ; mu++ ) {
						int tt = (mu==0)?t+1:t;
						int xx = (mu==1)?x+1:x;
						int yy = (mu==2)?y+1:y;
						int zz = (mu==3)?( (isOddRow)?(z):z+1 ):z;

						tt = (tt+T) % T;
						xx = (xx+LX) % LX;
						yy = (yy+LY) % LY;
						zz = (zz+LZ) % (LZ/2);

						out[i] = in [ ((((((tt*LX)+xx)*LY)+yy)*LZ/2)+zz)*4+mu ];
						i++;
					}
				}
			}
		}
	}
}

void reorganize_back_ueven (su3 *out, su3 *in) {
	int i = 0;
	for (int t=0 ; t<T ; t++ ) {
		for (int x=0 ; x<LX ; x++ ) {
			for (int y=0 ; y<LY ; y++ ) {
				for (int z=0 ; z<LZ/2 ; z++ ) {
					int isOddRow = (t & 1) ^ (x & 1) ^ (y & 1);
					for (int mu=0; mu<4 ; mu++ ) {
						int tt = (mu==0)?t+1:t;
						int xx = (mu==1)?x+1:x;
						int yy = (mu==2)?y+1:y;
						int zz = (mu==3)?( (isOddRow)?(z):z+1 ):z;

						tt = (tt+T) % T;
						xx = (xx+LX) % LX;
						yy = (yy+LY) % LY;
						zz = (zz+LZ) % (LZ/2);

						out[((((((tt*LX)+xx)*LY)+yy)*LZ/2)+zz)*4+mu ] = in [ i ];
						i++;
					}
				}
			}
		}
	}
}

/* Converting from qphix style gauge for the whole lattice, to even/odd separated
 * tmLQCD style gauge fields
 */
void devide_gauge_to_oddeven(su3 const * const in, su3 * const even, su3 * const odd, int ieo) {
	int i = 0;
	for (int t=0 ; t<T ; t++ ) {
		for (int z=0 ; z<LZ ; z++ ) {
			for (int y=0 ; y<LY ; y++ ) {
				for (int x=0 ; x<LX/2 ; x++ ) {
					for (int mu=0; mu<4 ; mu++ ) {
						for (int f=-1; f<=1 ; f+= 2) {
							su3 tmp = in[i];

							int isOddRow = (t & 1) ^ (z & 1) ^ (y & 1) ^ ieo;

							/*int mu_ = (mu+1)%4;
							int t_ = t;               // converting from checkerboarded
							int z_ = z/2;             // coordinates of qphix along x-axis
							int y_ = y;               // to tmLQCD checkerboarding along
							int x_ = (2*x)+isOddRow;  // along y-axis*/

							if (f == 1) {

								int xx = (mu==0)?( isOddRow ? x+1 :x ):x;
								int yy = (mu==1)?y+1:y;
								int zz = (mu==2)?z+1:z;
								int tt = (mu==3)?t+1:t;

								tt = (tt+T) % T;
								zz = (zz+LZ) % LZ;
								yy = (yy+LY) % LY;
								xx = (xx+LX) % (LX/2);

								even[ ((((((tt*LZ)+zz)*LY)+yy)*LX/2)+xx)*4+mu ] = tmp;

							} else {

								int xx = (mu==0)?( isOddRow ? x : x-1 ):x;
								int yy = (mu==1)?y-1:y;
								int zz = (mu==2)?z-1:z;
								int tt = (mu==3)?t-1:t;

								tt = (tt+T) % T;
								zz = (zz+LZ) % LZ;
								yy = (yy+LY) % LY;
								xx = (xx+LX) % (LX/2);

								odd[ ((((((tt*LZ)+zz)*LY)+yy)*LX/2)+xx)*4+mu ] = tmp;

							}

							i++;
						}
					}
				}
			}
		}
	}
}

void print_spinors (spinor* s) {
	for (int i=0 ; i < VOLUME/2 ; i++ ) {
		printf("%f %f\n", creal(s->s0.c0), cimag(s->s0.c0));
		s++;
	}
}
void print_gauges (su3* s) {
	for (int i=0 ; i < VOLUME/2 * 4 ; i++ ) {
		printf("%f %f\n", creal(s->c00), cimag(s->c00));
		s++;
	}
}

/*int LX, LY, LZ;
LX = LY = L+2;
LZ = L/2+1;
int i = 0;
for (int t = 0 ; t < T ; t++ ) {
		for (int x = 0 ; x < L ; x++ ) {
			for (int y = 0 ; y < L ; y++ ) {
				for (int z = 0 ; z < L/2 ; z++ ) {
					int isOddRow = (t & 1) ^ (x & 1) ^ (y & 1) ^ 1;
					int error = compare_spinor(
							out + i  ,
							out_dfe + (t+1)*LX*LY*LZ +
				            (x+1)*LY*LZ +
				            (y+1)*LZ + z + isOddRow
				            );
					i++;
					if (error) {
						printf("Wrong %d! %d\n", error,i);
						spinor *a = out + i;
						spinor *b = out_dfe + i;
						printf("%f %f    %f %f ", creal(a->s0.c0), cimag(a->s0.c0),
								                  creal(b->s0.c0), cimag(b->s0.c0));
						return 1;
					}
				}
			}
		}
	}*



spinor **** create_4d_spinor_wrapper(spinor *s) {
	spinor**** output;
	output = calloc(T, sizeof(spinor***));
	output[0] = calloc(T * LX, sizeof(spinor**));
	for (int i = 1; i < T; i++)
		output[i] = output[0] + i * LX;

	output[0][0] = calloc(T * LX * LY, sizeof(spinor*));
	for (int i = 1; i < T * LX; i++)
		output[0][i] = output[0][0] + i * LY;

	for (int i = 0; i < T * LX * LY; i++)
		output[0][0][i] = s + i * LX / 2;

	return output;
}*/
