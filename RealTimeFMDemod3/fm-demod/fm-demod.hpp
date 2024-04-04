#include <readerwriterqueue/readerwriterqueue.h>
#include <complex>
#include <iostream>
#include <thread>
#include <fstream>

using namespace std::literals::complex_literals;

static const float PI = 3.14159265f;
static const uint32_t MAXFRAMESIZE = 16384U;

static const uint32_t IQSampleRate = 2400000;

/* FILTER ASSUMES A IQ SAMPLE RATE OF 2.4 MSPS */
static const uint32_t bandwidthFilterOrder = 11;
static const float bandwidthFilterCoeff[bandwidthFilterOrder] = { 0.0137473f, 0.0296134f, 0.0716293f, 0.124594f, 0.16804f, 0.184752f, 0.16804f, 0.124594f, 0.0716293f, 0.0296134f, 0.0137473f };

/* FILTER ASSUMES A SAMPLE RATE OF 240 KSPS */
static const uint32_t derivativeFilterOrder = 7;
static const float derivativeFilterCoeff[derivativeFilterOrder] = { 0.0802699f, -0.310117f, 0.895233f, 0.0f, -0.895233f, 0.310117f, -0.0802699f };

/* FILTER ASSUMES A SAMPLE RATE OF 240 KSPS */
static const float deEmpasisB =  0.027027027027027f;
static const float deEmpasisA = -0.945945945945946f;


class FmDemod {
public:
	FmDemod(uint32_t _frameSize, uint32_t initalAudioBufSize, int32_t _frequencyShiftHz);
	bool addRawIQSamples(uint8_t* buf, uint32_t len);
	bool getAudioSamples(float* buf, uint32_t len);
	void startProcessing();
private:
	uint32_t frameSize;
	int32_t frequencyShiftHz;

	moodycamel::ReaderWriterQueue<std::complex<float>> IQSampleInput;
	moodycamel::ReaderWriterQueue<float> audioSampleOutput;

	void processingLoop();

	void frequencyShift(std::complex<float>* samples, const uint32_t len, uint32_t& n);
	template <class T> void applyFIRFilter(const float* coeffs, const uint32_t order, T* samples, const uint32_t len, T* firState);
	template <class T> uint32_t decimate(const uint8_t ratio, T* samples, const uint32_t inLen, uint32_t& n);
	void calculatePhase(const std::complex<float>* samples, float* phase, const uint32_t len, float& lastPhase);
	void deEmphasis(float* samples, const uint32_t len, float& lastInput, float& lastOutput);
};