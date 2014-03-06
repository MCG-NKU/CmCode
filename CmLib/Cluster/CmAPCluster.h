#pragma once

/************************************************************************/
/* Wrapper class for the science paper:                                 */
/*		"Clustering by Passing Messages Between Data Points".			*/
/*      http://www.psi.toronto.edu/index.php?q=affinity%20propagation	*/
/************************************************************************/

class CmAPCluster
{
public:
	CmAPCluster(void);
	~CmAPCluster(void);

	typedef struct {
		int cbSize; /* size of options structure, in bytes */
		double lambda; /* damping factor (between 0.5 and 1) */
		int minimum_iterations, /* minimum number of iterations (=1) */
			converge_iterations, /* number of static iterations until considered converged */
			maximum_iterations; /* maximum number of iterations */
		int nonoise; /* add noise (modifies input similarities!) to break symmetry (=0) or don't add noise (=1) */
		/* Function called to report iteration-by-iteration progress */
		int (*progress)(double *a, double *r, int N, int *idx, int I, double netsim, double dpsim, double expref, int iter);
		int (*progressf)(float *a, float *r, int N, int *idx, int I, double netsim, double dpsim, double expref, int iter);
	} APOPTIONS, *PAPOPTIONS;

	typedef struct {
		int cbSize; /* size of options structure, in bytes */
		int number_of_restarts, /* number of random initializations (restarts) to perform */
			maximum_iterations; /* maximum number of iterations for each restart */
		/*Mutually exclusive options: */
		int use_input_clusterings, /* idx contains cluster assignments -- use them instead of random initializations */
			use_input_exemplars; /* idx contains exemplars -- use them instead of random initializations */
	} KCOPTIONS, *PKCOPTIONS;

	enum ERROR_CODE {APCLUSTER_NOERROR = 0, 
		APCLUSTER_ERROR_EMPTYSIMILARITYROW = -1,
		APCLUSTER_ERROR_DUPLICATESIMILARITY = -2,
		APCLUSTER_ERROR_OUTOFMEMORY = -3,
		APCLUSTER_ERROR_BADOPTIONS = -4,
		APCLUSTER_ERROR_BADFULLMATRIX = -5,
		APCLUSTER_CALLBACK_ABORT = -6,
		APCLSUTER_USERERROR_MAX = -101
	};

public:  // Functions from the DLL
	/*   *f=>single-precision (float) version;  *32=>32-bit indexing version (for 65k+ datasets)
	sij: array of similarities of length 'N'
	i: array of row indices ('i' from 'sij') of length 'N'; can be NULL to indicate a full similarity matrix
	j: array of column indices ('j' from 'sij') of length 'N'; can be NULL to indicate a full similarity matrix
	N: number of input similarities
	idx: array of index values where clustering solution is returned
	netsim: net similarity value(s) returned to caller
	apoptions: algorithm options (see above struct definitions)*/
	typedef int (__stdcall *apcluster)(double *sij, unsigned short int *i, unsigned short int *j, unsigned int N, int *idx, double *netsim, APOPTIONS *apoptions);
	typedef int (__stdcall *apclusterf)(float *sij, unsigned short int *i, unsigned short int *j, unsigned int N, int *idx, double *netsim, APOPTIONS *apoptions);
	typedef int (__stdcall *apcluster32)(double *sij, unsigned int *i, unsigned int *j, unsigned int N, int *idx, double *netsim, APOPTIONS *apoptions);
	typedef int (__stdcall *apclusterf32)(float *sij, unsigned int *i, unsigned int *j, unsigned int N, int *idx, double *netsim, APOPTIONS *apoptions);

	typedef int (__stdcall *kcenters)(double *sij, unsigned short int *i, unsigned short int *j, unsigned int N, unsigned int K, int *idx, double *netsim, KCOPTIONS *kcoptions);
	typedef int (__stdcall *kcentersf)(float *sij, unsigned short int *i, unsigned short int *j, unsigned int N, unsigned int K, int *idx, double *netsim, KCOPTIONS *kcoptions);
	typedef int (__stdcall *kcenters32)(double *sij, unsigned int *i, unsigned int *j, unsigned int N, unsigned int K, int *idx, double *netsim, KCOPTIONS *kcoptions);
	typedef int (__stdcall *kcentersf32)(float *sij, unsigned int *i, unsigned int *j, unsigned int N, unsigned int K, int *idx, double *netsim, KCOPTIONS *kcoptions);
	
public: // Options and functions from DLL
	APOPTIONS apoptions;
	//KCOPTIONS kcoptions;

	static int callback(double *a, double *r, int N, int *idx, int I, double netsim, double dpsim, double expref, int iter);

	static int ReMapIdx(vecI &idx);

	apcluster32 GetApFun() {return apFun;} // Get affinity propagation function from DLL
	kcenters32 GetKcFun()  {return kcFun;}  // Get K center function from DLL

private: // private variables
	HMODULE dlh;
	apcluster32 apFun;
	kcenters32 kcFun;
};

