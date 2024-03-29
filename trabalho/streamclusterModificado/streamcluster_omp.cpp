#include <assert.h>
#include <limits.h>
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include <fstream>
#include <iostream>

#ifdef ENABLE_THREADS
#include <pthread.h>

#include "parsec_barrier.hpp"
#endif

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

using namespace std;

#define MAXNAMESIZE 1024  // max filename length
#define SEED 1
#define SP 1	// number of repetitions of speedy must be >=1
#define ITER 3	// iterate ITER* k log k times; ITER >= 1

#define CACHE_LINE 32  // cache line in byte

typedef struct {
	float weight;
	float* coord;
	long assign; /* number of point where this one is assigned */
	float cost;	 /* cost of that assignment, weight*distance */
} Point;

typedef struct {
	long num; /* number of points; may not be N if this is a sample */
	int dim;  /* dimensionality */
	Point* p; /* the array itself */
} Points;

static bool* switch_membership;	 //whether to switch membership in pgain
static bool* is_center;			 //whether a point is a center
static int* center_table;		 //index table of centers

static int nproc;  //# of threads
//int gg = 0;
//double* timesgg;
//double ggsum = 0;

float dist(Point p1, Point p2, int dim);

int isIdentical(float* i, float* j, int D) {
	int a = 0;
	int equal = 1;

	while (equal && a < D) {
		if (i[a] != j[a])
			equal = 0;
		else
			a++;
	}
	if (equal)
		return 1;
	else
		return 0;
}

static int floatcomp(const void* i, const void* j) {
	float a, b;
	a = *(float*)(i);
	b = *(float*)(j);
	if (a > b) return (1);
	if (a < b) return (-1);
	return (0);
}

void shuffle(Points* points) {
	long i, j;
	Point temp;
	for (i = 0; i < points->num - 1; i++) {
		j = (lrand48() % (points->num - i)) + i;
		temp = points->p[i];
		points->p[i] = points->p[j];
		points->p[j] = temp;
	}
}

void intshuffle(int* intarray, int length) {
	long i, j;
	int temp;
	for (i = 0; i < length; i++) {
		j = (lrand48() % (length - i)) + i;
		temp = intarray[i];
		intarray[i] = intarray[j];
		intarray[j] = temp;
	}
}

float dist(Point p1, Point p2, int dim) {
	int i;
	float result = 0.0;
	for (i = 0; i < dim; i++)
		result += (p1.coord[i] - p2.coord[i]) * (p1.coord[i] - p2.coord[i]);
	return (result);
}

float pspeedy(Points* points, float z, long* kcenter, int pid, pthread_barrier_t* barrier) {
#ifdef ENABLE_THREADS
	pthread_barrier_wait(barrier);
#endif
	long bsize = points->num / nproc;
	long k1 = bsize * pid;
	long k2 = k1 + bsize;
	if (pid == nproc - 1) k2 = points->num;

	static double totalcost;

	static bool open = false;
	static double* costs;  //cost for each thread.
	static int i;

#ifdef ENABLE_THREADS
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
#endif

	for (int k = k1; k < k2; k++) {
		float distance = dist(points->p[k], points->p[0], points->dim);
		points->p[k].cost = distance * points->p[k].weight;
		points->p[k].assign = 0;
	}

	if (pid == 0) {
		*kcenter = 1;
		costs = (double*)malloc(sizeof(double) * nproc);
	}

#ifdef ENABLE_THREADS
	pthread_barrier_wait(barrier);
#endif

	if (pid != 0) {	 // we are not the master threads. we wait until a center is opened.
		while (1) {
#ifdef ENABLE_THREADS
			pthread_mutex_lock(&mutex);
			while (!open) pthread_cond_wait(&cond, &mutex);
			pthread_mutex_unlock(&mutex);
#endif
			if (i >= points->num) break;
			for (int k = k1; k < k2; k++) {
				float distance = dist(points->p[i], points->p[k], points->dim);
				if (distance * points->p[k].weight < points->p[k].cost) {
					points->p[k].cost = distance * points->p[k].weight;
					points->p[k].assign = i;
				}
			}
#ifdef ENABLE_THREADS
			pthread_barrier_wait(barrier);
			pthread_barrier_wait(barrier);
#endif
		}
	} else {  // I am the master thread. I decide whether to open a center and notify others if so.
		for (i = 1; i < points->num; i++) {
			bool to_open = ((float)lrand48() / (float)INT_MAX) < (points->p[i].cost / z);
			if (to_open) {
				(*kcenter)++;
#ifdef ENABLE_THREADS
				pthread_mutex_lock(&mutex);
#endif
				open = true;
#ifdef ENABLE_THREADS
				pthread_mutex_unlock(&mutex);
				pthread_cond_broadcast(&cond);
#endif
				for (int k = k1; k < k2; k++) {
					float distance = dist(points->p[i], points->p[k], points->dim);
					if (distance * points->p[k].weight < points->p[k].cost) {
						points->p[k].cost = distance * points->p[k].weight;
						points->p[k].assign = i;
					}
				}
#ifdef ENABLE_THREADS
				pthread_barrier_wait(barrier);
#endif
				open = false;
#ifdef ENABLE_THREADS
				pthread_barrier_wait(barrier);
#endif
			}
		}
#ifdef ENABLE_THREADS
		pthread_mutex_lock(&mutex);
#endif
		open = true;
#ifdef ENABLE_THREADS
		pthread_mutex_unlock(&mutex);
		pthread_cond_broadcast(&cond);
#endif
	}
#ifdef ENABLE_THREADS
	pthread_barrier_wait(barrier);
#endif
	open = false;
	double mytotal = 0;
	for (int k = k1; k < k2; k++) {
		mytotal += points->p[k].cost;
	}
	costs[pid] = mytotal;
#ifdef ENABLE_THREADS
	pthread_barrier_wait(barrier);
#endif
	// aggregate costs from each thread
	if (pid == 0) {
		totalcost = z * (*kcenter);
		for (int i = 0; i < nproc; i++) {
			totalcost += costs[i];
		}
		free(costs);
	}
#ifdef ENABLE_THREADS
	pthread_barrier_wait(barrier);
#endif

	return (totalcost);
}

// serial impl -> real    0m2,513s w/ n=1000
double pgain(long x, Points* points, double z, long int* numcenters) {
	//declares
	int i;
	int number_of_centers_to_close = 0;
	double cost_of_opening_x = 0;
	double* work_mem;
	double gl_cost_of_opening_x = 0;
	int gl_number_of_centers_to_close = 0;
	int stride = *numcenters + 2;
	int cl = CACHE_LINE / sizeof(double);

	//small if
	if (stride % cl != 0) {
		stride = cl * (stride / cl + 1);
	}

	//memory stuff 1
	work_mem = (double*)malloc(stride * 2 * sizeof(double));

	//small for
	int count = 0;
	for (int i = 0; i < points->num; i++) {
		if (is_center[i]) {
			center_table[i] = count++;
		}
	}

	//memory stuff 3
	memset(switch_membership, 0, points->num * sizeof(bool));
	memset(work_mem, 0, stride * 2 * sizeof(double));
	double* lower = &work_mem[0];
	double* gl_lower = &work_mem[stride];

	//big for 1 - WORST ONE - 0.04s ea
	double initime = omp_get_wtime();

	float* x_cost_arr = (float*)malloc(points->num * sizeof(float));

#pragma omp parallel num_threads(4)
#pragma omp for
	for (i = 0; i < points->num; i++) {
		x_cost_arr[i] = dist(points->p[i], points->p[x], points->dim);
		x_cost_arr[i] *= points->p[i].weight;
	}

	double finishtime = omp_get_wtime();
	//printf("time = %f\n",finishtime-initime);
	//ggsum += finishtime-initime;
	//gg++;
	//timesgg[gg] = finishtime-initime;

	for (i = 0; i < points->num; i++) {
		float current_cost = points->p[i].cost;

		if (x_cost_arr[i] < current_cost) {
			switch_membership[i] = 1;
			cost_of_opening_x += x_cost_arr[i] - current_cost;

		} else {
			int assign = points->p[i].assign;
			lower[center_table[assign]] += current_cost - x_cost_arr[i];
		}
	}

	/* sequencial
  for ( i = 0; i < points->num; i++ ) {
    float x_cost = dist(points->p[i], points->p[x], points->dim) * points->p[i].weight;
    float current_cost = points->p[i].cost;

    if ( x_cost < current_cost ) {
      switch_membership[i] = 1;
      cost_of_opening_x += x_cost - current_cost;

    } else {
      int assign = points->p[i].assign;
      lower[center_table[assign]] += current_cost - x_cost;
    }
  }*/

	// big for 2 - 0.00006s ea
	for (int i = 0; i < points->num; i++) {
		if (is_center[i]) {
			double low = z;
			low += work_mem[center_table[i]];
			gl_lower[center_table[i]] = low;
			if (low > 0) {
				++number_of_centers_to_close;
				cost_of_opening_x -= low;
			}
		}
	}

	// memory stuff 4
	work_mem[*numcenters] = number_of_centers_to_close;
	work_mem[*numcenters + 1] = cost_of_opening_x;
	gl_cost_of_opening_x = z;
	gl_number_of_centers_to_close += number_of_centers_to_close;
	gl_cost_of_opening_x += cost_of_opening_x;

	if (gl_cost_of_opening_x < 0) {
		// big for 3 - 0.001 ea <<
		for (int i = 0; i < points->num; i++) {
			bool close_center = gl_lower[center_table[points->p[i].assign]] > 0;
			if (switch_membership[i] || close_center) {
				points->p[i].cost = points->p[i].weight * dist(points->p[i], points->p[x], points->dim);
				points->p[i].assign = x;
			}
		}

		//small for 2 - 0.000007 ea <<
		for (int i = 0; i < points->num; i++) {
			if (is_center[i] && gl_lower[center_table[i]] > 0)
				is_center[i] = false;
		}

		if (x >= 0 && x < points->num)
			is_center[x] = true;

		*numcenters = *numcenters + 1 - gl_number_of_centers_to_close;

	} else {
		gl_cost_of_opening_x = 0;
	}

	free(work_mem);
	return -gl_cost_of_opening_x;
}

float pFL(Points* points, int* feasible, int numfeasible,
		  float z, long* k, double cost, long iter, float e,
		  int pid, pthread_barrier_t* barrier) {
#ifdef ENABLE_THREADS
	pthread_barrier_wait(barrier);
#endif
	long i;
	long x;
	double change;
	long numberOfPoints;

	change = cost;

	while (change / cost > 1.0 * e) {
		change = 0.0;
		numberOfPoints = points->num;
		/* randomize order in which centers are considered */

		if (pid == 0) {
			intshuffle(feasible, numfeasible);
		}
#ifdef ENABLE_THREADS
		pthread_barrier_wait(barrier);
#endif
		for (i = 0; i < iter; i++) {
			x = i % numfeasible;
			change += pgain(feasible[x], points, z, k);
		}
		cost -= change;
#ifdef ENABLE_THREADS
		pthread_barrier_wait(barrier);
#endif
	}
	return (cost);
}

int selectfeasible_fast(Points* points, int** feasible, int kmin, int pid, pthread_barrier_t* barrier) {
	int numfeasible = points->num;
	if (numfeasible > (ITER * kmin * log((double)kmin)))
		numfeasible = (int)(ITER * kmin * log((double)kmin));
	*feasible = (int*)malloc(numfeasible * sizeof(int));

	float* accumweight;
	float totalweight;

	long k1 = 0;
	long k2 = numfeasible;

	float w;
	int l, r, k;

	if (numfeasible == points->num) {
		for (int i = k1; i < k2; i++)
			(*feasible)[i] = i;
		return numfeasible;
	}

	accumweight = (float*)malloc(sizeof(float) * points->num);

	accumweight[0] = points->p[0].weight;
	totalweight = 0;
	for (int i = 1; i < points->num; i++) {
		accumweight[i] = accumweight[i - 1] + points->p[i].weight;
	}
	totalweight = accumweight[points->num - 1];

	for (int i = k1; i < k2; i++) {
		w = (lrand48() / (float)INT_MAX) * totalweight;
		l = 0;
		r = points->num - 1;
		if (accumweight[0] > w) {
			(*feasible)[i] = 0;
			continue;
		}
		while (l + 1 < r) {
			k = (l + r) / 2;
			if (accumweight[k] > w) {
				r = k;
			} else {
				l = k;
			}
		}
		(*feasible)[i] = r;
	}

	free(accumweight);

	return numfeasible;
}

float pkmedian(Points* points, long kmin, long kmax, long* kfinal,
			   int pid, pthread_barrier_t* barrier) {
	int i;
	double cost;
	double lastcost;
	double hiz, loz, z;

	static long k;
	static int* feasible;
	static int numfeasible;
	static double* hizs;

	if (pid == 0) hizs = (double*)calloc(nproc, sizeof(double));
	hiz = loz = 0.0;
	long numberOfPoints = points->num;
	long ptDimension = points->dim;

	//my block
	long bsize = points->num / nproc;
	long k1 = bsize * pid;
	long k2 = k1 + bsize;
	if (pid == nproc - 1) k2 = points->num;

#ifdef ENABLE_THREADS
	pthread_barrier_wait(barrier);
#endif

	double myhiz = 0;
	for (long kk = k1; kk < k2; kk++) {
		myhiz += dist(points->p[kk], points->p[0],
					  ptDimension) *
				 points->p[kk].weight;
	}
	hizs[pid] = myhiz;

#ifdef ENABLE_THREADS
	pthread_barrier_wait(barrier);
#endif

	for (int i = 0; i < nproc; i++) {
		hiz += hizs[i];
	}

	loz = 0.0;
	z = (hiz + loz) / 2.0;
	/* NEW: Check whether more centers than points! */
	if (points->num <= kmax) {
		/* just return all points as facilities */
		for (long kk = k1; kk < k2; kk++) {
			points->p[kk].assign = kk;
			points->p[kk].cost = 0;
		}
		cost = 0;
		if (pid == 0) {
			free(hizs);
			*kfinal = k;
		}
		return cost;
	}

	if (pid == 0) shuffle(points);
	cost = pspeedy(points, z, &k, pid, barrier);

	i = 0;
	/* give speedy SP chances to get at least kmin/2 facilities */
	while ((k < kmin) && (i < SP)) {
		cost = pspeedy(points, z, &k, pid, barrier);
		i++;
	}

	/* if still not enough facilities, assume z is too high */
	while (k < kmin) {
		if (i >= SP) {
			hiz = z;
			z = (hiz + loz) / 2.0;
			i = 0;
		}
		if (pid == 0) shuffle(points);
		cost = pspeedy(points, z, &k, pid, barrier);
		i++;
	}

	if (pid == 0) {
		numfeasible = selectfeasible_fast(points, &feasible, kmin, pid, barrier);
		for (int i = 0; i < points->num; i++) {
			is_center[points->p[i].assign] = true;
		}
	}

#ifdef ENABLE_THREADS
	pthread_barrier_wait(barrier);
#endif

	while (1) {
		/* first get a rough estimate on the FL solution */
		lastcost = cost;
		cost = pFL(points, feasible, numfeasible,
				   z, &k, cost, (long)(ITER * kmax * log((double)kmax)), 0.1, pid, barrier);

		/* if number of centers seems good, try a more accurate FL */
		if (((k <= (1.1) * kmax) && (k >= (0.9) * kmin)) ||
			((k <= kmax + 2) && (k >= kmin - 2))) {
			cost = pFL(points, feasible, numfeasible,
					   z, &k, cost, (long)(ITER * kmax * log((double)kmax)), 0.001, pid, barrier);
		}

		if (k > kmax) {
			loz = z;
			z = (hiz + loz) / 2.0;
			cost += (z - loz) * k;
		}
		if (k < kmin) {
			hiz = z;
			z = (hiz + loz) / 2.0;
			cost += (z - hiz) * k;
		}

		if (((k <= kmax) && (k >= kmin)) || ((loz >= (0.999) * hiz))) {
			break;
		}
#ifdef ENABLE_THREADS
		pthread_barrier_wait(barrier);
#endif
	}

	//clean up...
	if (pid == 0) {
		free(feasible);
		free(hizs);
		*kfinal = k;
	}

	return cost;
}

int contcenters(Points* points) {
	long i, ii;
	float relweight;

	for (i = 0; i < points->num; i++) {
		/* compute relative weight of this point to the cluster */
		if (points->p[i].assign != i) {
			relweight = points->p[points->p[i].assign].weight + points->p[i].weight;
			relweight = points->p[i].weight / relweight;
			for (ii = 0; ii < points->dim; ii++) {
				points->p[points->p[i].assign].coord[ii] *= 1.0 - relweight;
				points->p[points->p[i].assign].coord[ii] +=
					points->p[i].coord[ii] * relweight;
			}
			points->p[points->p[i].assign].weight += points->p[i].weight;
		}
	}

	return 0;
}

void copycenters(Points* points, Points* centers, long* centerIDs, long offset) {
	long i;
	long k;

	bool* is_a_median = (bool*)calloc(points->num, sizeof(bool));

	/* mark the centers */
	for (i = 0; i < points->num; i++) {
		is_a_median[points->p[i].assign] = 1;
	}

	k = centers->num;

	/* count how many  */
	for (i = 0; i < points->num; i++) {
		if (is_a_median[i]) {
			memcpy(centers->p[k].coord, points->p[i].coord, points->dim * sizeof(float));
			centers->p[k].weight = points->p[i].weight;
			centerIDs[k] = i + offset;
			k++;
		}
	}

	centers->num = k;

	free(is_a_median);
}

struct pkmedian_arg_t {
	Points* points;
	long kmin;
	long kmax;
	long* kfinal;
	int pid;
	pthread_barrier_t* barrier;
};

void* localSearchSub(void* arg_) {
	pkmedian_arg_t* arg = (pkmedian_arg_t*)arg_;
	pkmedian(arg->points, arg->kmin, arg->kmax, arg->kfinal, arg->pid, arg->barrier);

	return NULL;
}

void localSearch(Points* points, long kmin, long kmax, long* kfinal) {
	pthread_barrier_t barrier;
	pthread_t* threads = new pthread_t[nproc];
	pkmedian_arg_t* arg = new pkmedian_arg_t[nproc];

#ifdef ENABLE_THREADS
	pthread_barrier_init(&barrier, NULL, nproc);
#endif
	for (int i = 0; i < nproc; i++) {
		arg[i].points = points;
		arg[i].kmin = kmin;
		arg[i].kmax = kmax;
		arg[i].pid = i;
		arg[i].kfinal = kfinal;

		arg[i].barrier = &barrier;
#ifdef ENABLE_THREADS
		pthread_create(threads + i, NULL, localSearchSub, (void*)&arg[i]);
#else
		localSearchSub(&arg[0]);
#endif
	}

#ifdef ENABLE_THREADS
	for (int i = 0; i < nproc; i++) {
		pthread_join(threads[i], NULL);
	}
#endif

	delete[] threads;
	delete[] arg;
#ifdef ENABLE_THREADS
	pthread_barrier_destroy(&barrier);
#endif
}

class PStream {
   public:
	virtual size_t read(float* dest, int dim, int num) = 0;
	virtual int ferror() = 0;
	virtual int feof() = 0;
	virtual ~PStream() {
	}
};

class SimStream : public PStream {
   public:
	SimStream(long n_) {
		n = n_;
	}
	size_t read(float* dest, int dim, int num) {
		size_t count = 0;
		for (int i = 0; i < num && n > 0; i++) {
			for (int k = 0; k < dim; k++) {
				dest[i * dim + k] = lrand48() / (float)INT_MAX;
			}
			n--;
			count++;
		}
		return count;
	}
	int ferror() {
		return 0;
	}
	int feof() {
		return n <= 0;
	}
	~SimStream() {
	}

   private:
	long n;
};

class FileStream : public PStream {
   public:
	FileStream(char* filename) {
		fp = fopen(filename, "rb");
		if (fp == NULL) {
			fprintf(stderr, "error opening file %s\n.", filename);
			exit(1);
		}
	}
	size_t read(float* dest, int dim, int num) {
		return std::fread(dest, sizeof(float) * dim, num, fp);
	}
	int ferror() {
		return std::ferror(fp);
	}
	int feof() {
		return std::feof(fp);
	}
	~FileStream() {
		fprintf(stderr, "closing file stream\n");
		fclose(fp);
	}

   private:
	FILE* fp;
};

void outcenterIDs(Points* centers, long* centerIDs, char* outfile) {
	FILE* fp = fopen(outfile, "w");
	if (fp == NULL) {
		fprintf(stderr, "error opening %s\n", outfile);
		exit(1);
	}
	int* is_a_median = (int*)calloc(sizeof(int), centers->num);
	for (int i = 0; i < centers->num; i++) {
		is_a_median[centers->p[i].assign] = 1;
	}

	for (int i = 0; i < centers->num; i++) {
		if (is_a_median[i]) {
			fprintf(fp, "%u\n", centerIDs[i]);
			fprintf(fp, "%lf\n", centers->p[i].weight);
			for (int k = 0; k < centers->dim; k++) {
				fprintf(fp, "%lf ", centers->p[i].coord[k]);
			}
			fprintf(fp, "\n\n");
		}
	}
	fclose(fp);
}

void streamCluster(PStream* stream,
				   long kmin, long kmax, int dim,
				   long chunksize, long centersize, char* outfile) {
	float* block = (float*)malloc(chunksize * dim * sizeof(float));
	float* centerBlock = (float*)malloc(centersize * dim * sizeof(float));
	long* centerIDs = (long*)malloc(centersize * dim * sizeof(long));

	if (block == NULL) {
		fprintf(stderr, "not enough memory for a chunk!\n");
		exit(1);
	}

	Points points;
	points.dim = dim;
	points.num = chunksize;
	points.p = (Point*)malloc(chunksize * sizeof(Point));

	for (int i = 0; i < chunksize; i++) {
		points.p[i].coord = &block[i * dim];
	}

	Points centers;
	centers.dim = dim;
	centers.p =

		(Point*)malloc(centersize * sizeof(Point));
	centers.num = 0;

	for (int i = 0; i < centersize; i++) {
		centers.p[i].coord = &centerBlock[i * dim];
		centers.p[i].weight = 1.0;
	}

	long IDoffset = 0;
	long kfinal;
	while (1) {
		size_t numRead = stream->read(block, dim, chunksize);
		//fprintf(stderr,"read %d points\n",numRead);

		if (stream->ferror() || numRead < (unsigned int)chunksize && !stream->feof()) {
			fprintf(stderr, "error reading data!\n");
			exit(1);
		}

		points.num = numRead;
		for (int i = 0; i < points.num; i++) {
			points.p[i].weight = 1.0;
		}

		switch_membership = (bool*)malloc(points.num * sizeof(bool));
		is_center = (bool*)calloc(points.num, sizeof(bool));
		center_table = (int*)malloc(points.num * sizeof(int));

		localSearch(&points, kmin, kmax, &kfinal);	// parallel
		contcenters(&points);						/* sequential */

		if (kfinal + centers.num > centersize) {
			//here we don't handle the situation where # of centers gets too large.
			fprintf(stderr, "oops! no more space for centers\n");
			exit(1);
		}

		copycenters(&points, &centers, centerIDs, IDoffset); /* sequential */
		IDoffset += numRead;

		free(is_center);
		free(switch_membership);
		free(center_table);

		if (stream->feof()) {
			break;
		}
	}

	//finally cluster all temp centers

	switch_membership = (bool*)malloc(centers.num * sizeof(bool));
	is_center = (bool*)calloc(centers.num, sizeof(bool));
	center_table = (int*)malloc(centers.num * sizeof(int));
	//timesgg = (double*)malloc(2000*sizeof(double));

	localSearch(&centers, kmin, kmax, &kfinal);	 // parallel
	contcenters(&centers);
	outcenterIDs(&centers, centerIDs, outfile);
}

int main(int argc, char** argv) {
	char* outfilename = new char[MAXNAMESIZE];
	char* infilename = new char[MAXNAMESIZE];
	long kmin, kmax, n, chunksize, clustersize;
	int dim;

#ifdef PARSEC_VERSION
#define __PARSEC_STRING(x) #x
#define __PARSEC_XSTRING(x) __PARSEC_STRING(x)
	fprintf(stderr, "PARSEC Benchmark Suite Version "__PARSEC_XSTRING(PARSEC_VERSION) "\n");
	fflush(NULL);
#else
	fprintf(stderr, "PARSEC Benchmark Suite\n");
	fflush(NULL);
#endif	//PARSEC_VERSION
#ifdef ENABLE_PARSEC_HOOKS
	__parsec_bench_begin(__parsec_streamcluster);
#endif

	if (argc < 10) {
		fprintf(stderr, "usage: %s k1 k2 d n chunksize clustersize infile outfile nproc\n",
				argv[0]);
		fprintf(stderr, "  k1:          Min. number of centers allowed\n");
		fprintf(stderr, "  k2:          Max. number of centers allowed\n");
		fprintf(stderr, "  d:           Dimension of each data point\n");
		fprintf(stderr, "  n:           Number of data points\n");
		fprintf(stderr, "  chunksize:   Number of data points to handle per step\n");
		fprintf(stderr, "  clustersize: Maximum number of intermediate centers\n");
		fprintf(stderr, "  infile:      Input file (if n<=0)\n");
		fprintf(stderr, "  outfile:     Output file\n");
		fprintf(stderr, "  nproc:       Number of threads to use\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "if n > 0, points will be randomly generated instead of reading from infile.\n");
		exit(1);
	}

	kmin = atoi(argv[1]);
	kmax = atoi(argv[2]);
	dim = atoi(argv[3]);
	n = atoi(argv[4]);
	chunksize = atoi(argv[5]);
	clustersize = atoi(argv[6]);
	strcpy(infilename, argv[7]);
	strcpy(outfilename, argv[8]);
	nproc = atoi(argv[9]);

	srand48(SEED);
	PStream* stream;
	if (n > 0) {
		stream = new SimStream(n);
	} else {
		stream = new FileStream(infilename);
	}

#ifdef ENABLE_PARSEC_HOOKS
	__parsec_roi_begin();
#endif

	streamCluster(stream, kmin, kmax, dim, chunksize, clustersize, outfilename);

#ifdef ENABLE_PARSEC_HOOKS
	__parsec_roi_end();
#endif

	delete stream;
	/*
  FILE* fp = fopen("outtest1", "w");
  if( fp==NULL ) {
    fprintf(stderr, "error opening %s\n","outtest1");
    exit(1);
  }
  for( int k = 0; k < gg; k++ ) 
    fprintf(fp, "%f\n", timesgg[k]);
  fclose(fp);
  */

	//printf("%d\n", gg);
	//printf("%f", ggsum);
#ifdef ENABLE_PARSEC_HOOKS
	__parsec_bench_end();
#endif

	return 0;
}
