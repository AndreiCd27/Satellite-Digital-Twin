#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include <future>
#include <thread>
#include <numeric>

inline constexpr int MAX_FACTORIAL_ORDER = 20;

class FactorialException : public std::runtime_error {
public:
	FactorialException(int n) : std::runtime_error(
		"Maximum factorial order is " + std::to_string(MAX_FACTORIAL_ORDER) +
		" Query : Compute " + std::to_string(n) + "! \n"
	) { }
};

inline long long int Factorial(const int N) {
	static int factorials[MAX_FACTORIAL_ORDER + 1] = { 0 };

	if (N > MAX_FACTORIAL_ORDER || N < 0) throw FactorialException(N);

	if (N == 0 || N == 1) return 1;
	if (factorials[N] > 0) return factorials[N];

	int i = N - 1;
	while (factorials[i] == 0 && i > 0) i--;

	long long int prod = (i <= 1) ? 1 : factorials[i];

	for (int j = i + 1; j <= N; j++) {
		prod *= j;
		factorials[j] = prod;
	}

	return prod;
};

inline long long int DoubleFactorial(const int N) {
	static int factorials[MAX_FACTORIAL_ORDER + 1] = { 0 };

	if (N > MAX_FACTORIAL_ORDER || N < 0) throw FactorialException(N);

	if (N == 0 || N == 1) return 1;
	if (factorials[N] > 0) return factorials[N];

	int i = N - 1;
	while (factorials[i] == 0 && i > 0) i-=2;

	long long int prod = (i <= 1) ? 1 : factorials[i];

	for (int j = i + 1; j <= N; j+=2) {
		prod *= j;
		factorials[j] = prod;
	}

	return prod;
};

template <int L, int M>
class LegendrePolynomial {
	

	double FactorialTerm = (M == 0) ? 1.0f : (double)DoubleFactorial(2 * M - 1); // Computes (2M-1)!!, the double factorial
	// i.e. the product of all odd integers less than or equal to 2M-1
public:
	double Compute(double x) const {
		// P<M,M>(x) = ((-1)^M) * (2*M - 1)!! * (1-x*x)^(M/2)
		double PMM = 1.0f;
		if constexpr (M > 0) {
			double xsq = sqrt((1.0f - x) * (1.0f + x));
			double pow_xsq = 1.0f;
			for (int ord = 0; ord < M; ord++) {
				pow_xsq *= xsq;
			}
			PMM = FactorialTerm * ((M % 2 == 0) ? 1.0f : -1.0f) * pow_xsq;
		}
		if constexpr (L == M) return PMM;
		if constexpr (L != M) {
			double P_Mp1_M = x * (2.0f * M + 1.0f) * PMM; // P<M+1,M>(x)
			if constexpr (L == M + 1) return P_Mp1_M;
			if constexpr (L != M + 1) {
				double res = 0.0f;
				for (int i = M + 2; i <= L; i++) {
					res = ((2.0f * i - 1.0f) * x * P_Mp1_M - (i + M - 1.0f) * PMM) / (i - M);
					PMM = P_Mp1_M;
					P_Mp1_M = res;
				}
				return res;
			}
		}
	}
};

// Spherical Harmonic Basis

// The basis interface (stored later in SH class)

class SHbasis {
public:
	virtual double Compute(double tetha, double phi) const = 0;
	virtual double ComputeLegendre(double x) const = 0;
	virtual void ComputeConstantsForAllM(float* out, const int i) const = 0;
	virtual ~SHbasis() = default;
};

constexpr double __PI = 3.141592653589793f;

template <int L, int M>
class Y : public SHbasis {
	const int absM = (M > 0) ? M : -M;
	double NormalizationTerm = sqrt(
		((2.0f * L + 1.0f) * Factorial(L - absM)) /
		(4.0f * __PI * Factorial(L + absM))
	);
	const double sqrt2 = sqrt(2.0f);
public:
	const LegendrePolynomial< L, (M > 0) ? M : -M > P;

	double Compute(double theta, double phi) const override {
		double P_at_tetha = P.Compute(cos(theta));

		if constexpr (M == 0) return NormalizationTerm * P_at_tetha;

		if constexpr (M > 0) return sqrt2 * NormalizationTerm * cos(M * phi) * P_at_tetha;

		if constexpr (M < 0) return sqrt2 * NormalizationTerm * sin(-M * phi) * P_at_tetha;
	}

	double ComputeLegendre(double x) const override {
		return P.Compute(x);
	}

	void ComputeConstantsForAllM(float* out, const int i) const override {
		for (int m = -L; m <= L; m++) {
			float val;
			if constexpr (M == 0) {
				val = NormalizationTerm;
			}
			else {
				val = sqrt2 * NormalizationTerm;
			}
			out[i + m + L] = val;
		}
	}
};

// BLOCKERS

template <int num>
struct BlockerXYZR {
	// global coords
	float x[num];
	float y[num];
	float z[num];
	// radius of the sphere at center (x,y,z)
	float r[num];
	// instance ID associated
	int insID[num];
};

template <int num>
struct Blockers {
	// spherical coords
	float phi[num];
	float theta[num];
	// dist to center
	float dist[num];
	// radius of the sphere
	float radius[num];
};

// SPHERICAL HARMONICS

class SH_Exception : public std::runtime_error {
public:
	SH_Exception(std::string MSG, int ERR) : std::runtime_error("[SH_ERROR_" + std::to_string(ERR) + "] " + MSG) {}
};

template <int MaxL>
class SH {
	SHbasis* bases[(MaxL + 1) * (MaxL + 1)];

	const LegendrePolynomial<MaxL + 1, 0> Pnxt;

public:

	const float SHNorm[16] = {
		0.282095f,
		0.488604f, 0.488604f, 0.488604f,
		1.092548f, 1.092548f, 0.315392f, 1.092548f, 0.546274f,
		0.590044f, 2.890611f, 0.457046f, 0.373176f, 0.457046f, 1.4453055f, 0.590044f
	};

	// ZH Coefficients for Clamped Cosine
	const float ZHcosine[4] = { __PI, 2.0f * __PI / 3.0f, __PI / 4.0f, __PI / 16.0f };

	// ZH Coefficients for Convolution
	const float ZHconv[4] = {
		// Convolution Terms for ZH to SH conversion (Funke-Hecke Conv Theorem) ( sqrt( 4pi / (2l+1) ) )
		3.544907f, // sqrt(4pi)
		2.046653f, // sqrt(4pi/3)
		1.585331f, // sqrt(4pi/5)
		1.339857f // sqrt(4pi/7)
	};

	const float logEpsilon = log(0.01f);

	void GetCombinedZHtoSH(float* ZHtoSH) {
		for (int i = 0; i < 4; i++) ZHtoSH[i] = ZHconv[i] * ZHcosine[i];
	}

	void ComputeCoefficients(const float phi, const float theta,
		const float radius, const float dist, float* outCoeff) {

		float cosAlpha = sqrt(1.0f - radius * radius / (dist * dist));

		float Vcoeff[MaxL + 1];

		Vcoeff[0] = (1.0f - cosAlpha) / 2.0f;

		for (int l = 1; l <= MaxL; l++) {
			double p_prev = (l == 0) ? 1.0 : bases[(l - 1) * l]->ComputeLegendre(cosAlpha);
			double p_next = (l < MaxL) ? bases[(l + 1) * (l + 2)]->ComputeLegendre(cosAlpha)
				: Pnxt.Compute(cosAlpha);

			Vcoeff[l] = p_prev - p_next;
		}

		for (int l = 0; l <= MaxL; l++) {
			//float coeffNormTerm = 2.0f * __PI * sqrt(1.0f / (float)(4 * (2 * l + 1) * __PI) );
			float normFactor = sqrt((4.0f * __PI) / (2.0f * l + 1.0f));

			for (int m = -l; m <= l; m++) {

				outCoeff[l * (l + 1) + m] = normFactor * Vcoeff[l] * 
					bases[l * (l + 1) + m]->Compute(theta, phi);
			}
		}
	}
	void ComputeLogSHCoefficients(const float phi, const float theta,
		const float radius, const float dist, float* outCoeff) {

		float cosAlpha = sqrt(1.0f - radius * radius / (dist * dist));

		float SHcoeff[(MaxL + 1) * (MaxL + 1)];
		ComputeCoefficients(phi, theta, radius, dist, SHcoeff);

		// We use the DC component to estimate the occlusion
		float occl = (1.0f - cosAlpha) / 2.0f;
		occl = std::clamp(occl, 0.00001f, 0.95f);

		//float logV = -x - 0.5f * x * x - 0.33f * x * x * x; // ln(1-x) approximation
		float logV = log(std::max(1.0f - occl, 0.00001f));
		
		// Conversion factor from liniar to log space
		float logScaleFactor = logV / std::max(occl, 0.00001f);

		// Convert all SH coefficients
		for (int i = 0; i < (MaxL + 1)*(MaxL + 1); i++) {
			outCoeff[i] = SHcoeff[i] * logScaleFactor;
		}

	}

	void ComputeZHVisibilityXYZR(const float cosAlpha, float* ZHout) const {
		float V_ratio = (1.0f + cosAlpha) * 0.5f;

		float logV = log(std::max(V_ratio, 0.01f));

		ZHout[0] = logV * ZHconv[0];

		ZHout[1] = logV * ZHconv[1] * 1.5f;
		ZHout[2] = logV * ZHconv[2] * 1.2f * cosAlpha;
		ZHout[3] = logV * ZHconv[3] * 0.9f * (5.0f * cosAlpha * cosAlpha - 1.0f) * 0.25f;
	}

	void ComputeZHVisibilityXYZR_LogEpsilon(const float cosAlpha, const float sinAlpha, float* ZHout) const {

		ZHout[0] = 2.0f * __PI * logEpsilon * (1.0f - cosAlpha) * ZHconv[0];
		ZHout[1] = __PI * logEpsilon * sinAlpha * sinAlpha * ZHconv[1];
		ZHout[2] = 0.5f * __PI * logEpsilon * cosAlpha * sinAlpha * sinAlpha * ZHconv[2];
		ZHout[3] = 0.25f * __PI * logEpsilon * (5.0f * cosAlpha * cosAlpha - 1.0f) * sinAlpha * sinAlpha * ZHconv[3];
	}

	void ComputeZHVisibilityXYZR_Opacity(const float cosAlpha, const float sinAlpha, float* ZHout) const {

		ZHout[0] = 2.0f * __PI * (1.0f - cosAlpha) * ZHconv[0];
		ZHout[1] = __PI * sinAlpha * sinAlpha * ZHconv[1];
		ZHout[2] = 0.5f * __PI * cosAlpha * sinAlpha * sinAlpha * ZHconv[2];
		ZHout[3] = 0.25f * __PI * (5.0f * cosAlpha * cosAlpha - 1.0f) * sinAlpha * sinAlpha * ZHconv[3];

		const float opacity = -8.0f;
		for (int i = 0; i < 4; i++) ZHout[i] *= opacity;
	}



	void ComputeLogSHCoefficientsXYZR(const float x, const float y, const float z, 
		const float* ZHcoeff, float* LogSH) const {

		float c0 = ZHcoeff[0];
		float c1 = ZHcoeff[1];
		float c2 = ZHcoeff[2];
		float c3 = ZHcoeff[3];

		const float x2 = x * x;
		const float y2 = y * y;
		const float z2 = z * z;
		const float xy = x * y;
		const float xz = x * z;
		const float yz = y * z;

		// ORDER L = 0

		LogSH[0] += c0 * SHNorm[0];

		// ORDER L = 1

		LogSH[1] += c1 * SHNorm[1] * x;
		LogSH[2] += c1 * SHNorm[2] * y;
		LogSH[3] += c1 * SHNorm[3] * z;

		// ORDER L = 2

		LogSH[4] += c2 * SHNorm[4] * xz;
		LogSH[5] += c2 * SHNorm[5] * xy;
		LogSH[6] += c2 * SHNorm[6] * (3.0f * y2 - 1.0f);
		LogSH[7] += c2 * SHNorm[7] * yz;
		LogSH[8] += c2 * SHNorm[8] * (x2 - z2);

		// ORDER L = 3

		LogSH[9] += c3 * SHNorm[9] * x * (x2 - 3.0f * z2);
		LogSH[10] += c3 * SHNorm[10] * xy * z;
		LogSH[11] += c3 * SHNorm[11] * x * (5.0f * y2 - 1.0f);
		LogSH[12] += c3 * SHNorm[12] * y * (5.0f * y2 - 3.0f);
		LogSH[13] += c3 * SHNorm[13] * z * (5.0f * y2 - 1.0f);
		LogSH[14] += c3 * SHNorm[14] * y * (x2 - z2);
		LogSH[15] += c3 * SHNorm[15] * z * (3.0f * x2 - z2);
	}

	void GetLightBasisYUp(float x, float y, float z, float* outSH) {
		// L=0
		outSH[0] = SHNorm[0];

		// L=1 (Y-up: x, y, z)
		outSH[1] = SHNorm[1] * x;
		outSH[2] = SHNorm[2] * y;
		outSH[3] = SHNorm[3] * z;

		// L=2
		outSH[4] = SHNorm[4] * (x * z);
		outSH[5] = SHNorm[5] * (x * y);
		outSH[6] = SHNorm[6] * (3.0f * y * y - 1.0f);
		outSH[7] = SHNorm[7] * (y * z);
		outSH[8] = SHNorm[8] * (x * x - z * z);

		// L=3
		outSH[9] = SHNorm[9] * x * (x * x - 3.0f * z * z);
		outSH[10] = SHNorm[10] * (x * y * z);
		outSH[11] = SHNorm[11] * x * (5.0f * y * y - 1.0f);
		outSH[12] = SHNorm[12] * y * (5.0f * y * y - 3.0f);
		outSH[13] = SHNorm[13] * z * (5.0f * y * y - 1.0f);
		outSH[14] = SHNorm[14] * y * (x * x - z * z);
		outSH[15] = SHNorm[15] * z * (3.0f * x * x - z * z);
	}

	void ApplyFunkHeckeCosine(float* outSH, bool ApplyZH_Norm) {
		// See Funk-Hecke theorem for computing the SH projection coefficients
		// of a circularly-simetric (ZH) about an arbitrary direction d,
		// having the effect of rotating the ZH canonical z-up vector

		float conv[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

		if (ApplyZH_Norm) {
			for (int i = 0; i < 4; i++) conv[i] = ZHconv[i];
		}

		// L = 0
		outSH[0] *= ZHcosine[0] * conv[0];

		// L = 1
		for (int i = 1; i < 4; i++) outSH[i] *= ZHcosine[1] * conv[1];

		// L = 2
		for (int i = 4; i < 9; i++) outSH[i] *= ZHcosine[2] * conv[2];

		// L = 3
		for (int i = 9; i < 16; i++) outSH[i] *= ZHcosine[3] * conv[3];
	}


	template <int num>
	std::vector<float> ComputeTotalVisibilityCoefficients(const Blockers<num>& b, const int BlockerCount) {

		const int NumCoeff = (MaxL + 1) * (MaxL + 1);

		if (BlockerCount < 1024) {
			std::vector<float> VisCoeff(NumCoeff, 0.0f);

			float out_coeff[NumCoeff];

			for (int i = 0; i < BlockerCount; i++) {
				this->ComputeLogSHCoefficients(b.phi[i], b.theta[i], b.radius[i], b.dist[i], out_coeff);

				for (int k = 0; k < NumCoeff; k++) VisCoeff[k] += out_coeff[k];
			}

			return VisCoeff;
		}

		int numThreads = std::thread::hardware_concurrency();
		if (numThreads == 0) numThreads = 2;

		int batchSize = (BlockerCount + numThreads - 1) / numThreads;

		std::vector<std::future<std::vector<float>>> futureValues;

		for (int t = 0; t < numThreads; t++) {
			int startI = t * batchSize;
			int endI = std::min(startI + batchSize, BlockerCount);

			//if (startI >= endI) break;

			futureValues.push_back(
				std::async(std::launch::async, [this, &b, startI, endI, NumCoeff]() {

					std::vector<float> VisCoeff(NumCoeff, 0.0f);

					float out_coeff[NumCoeff];

					for (int i = startI; i < endI; i++) {
						this->ComputeLogSHCoefficients(b.phi[i], b.theta[i], b.radius[i], b.dist[i], out_coeff);

						for (int k = 0; k < NumCoeff; k++) VisCoeff[k] += out_coeff[k];
					}

					return VisCoeff;
				})
			);
		}
		
		std::vector<float> VisCoeffFinal(NumCoeff, 0.0f);

		for (auto& f : futureValues) {
			std::vector<float> VisResult = f.get();
			for (int k = 0; k < NumCoeff; k++) VisCoeffFinal[k] += VisResult[k];
		}
		
		return VisCoeffFinal;
	}

	template <int L, int M>
	void InitBases() {
		bases[L*(L+1) + M] = new Y<L, M>();
		if constexpr (M < L) {
			InitBases<L, M + 1>();
		}
		else if constexpr (L < MaxL) {
			InitBases<L + 1, -L - 1>();
		}
	}
	SH() {
		for (int i = 0; i < (MaxL + 1) * (MaxL + 1); i++) bases[i] = nullptr;
		InitBases<0, 0>();
	}
	~SH() {
		for (int i = 0; i < (MaxL + 1) * (MaxL + 1); i++) delete bases[i];
	};

	float EvaluateBasis(int L, int M, float theta, float phi) const {
		if (L < 0 || L > MaxL || abs(M) > L) throw SH_Exception("L or M factor out of bounds", 0);

		int i = L * (L + 1) + M;

		if (bases[i] != nullptr) return bases[i]->Compute(theta, phi);

		throw SH_Exception("Basis<L,M> is nullptr; Y<L,M> not initialized", 1);
	}

	std::vector<float> EvaluateAll(float theta, float phi) const {
		std::vector<float> res((MaxL + 1) * (MaxL + 1));

		for (int i = 0; i < (MaxL + 1) * (MaxL + 1); i++) {
			if (bases[i] != nullptr) {
				res[i] = bases[i]->Compute(theta, phi);
			}
			else {
				res[i] = 0.0f;
			}
		}

		return res;
	}

	void ComputeYLMconstants(float* out) {
		for (int l = 0; l <= MaxL; l++) {
			bases[l * (l + 1)]->ComputeConstantsForAllM(out, l * l);
		}
	}
};