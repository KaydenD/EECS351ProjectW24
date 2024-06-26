#include "fm-demod.hpp"
#include <chrono>

FmDemod::FmDemod(uint32_t _frameSize, uint32_t initalAudioBufSize, int32_t _frequencyShiftHz){
	frameSize = _frameSize;
	if (frameSize > MAXFRAMESIZE) {
		frameSize = MAXFRAMESIZE;
		std::cout << "Framesize too big. Resetting to " << MAXFRAMESIZE << std::endl;
	}

	IQSampleInput = moodycamel::ReaderWriterQueue<std::complex<float>>(frameSize * 2);
	audioSampleOutput = moodycamel::ReaderWriterQueue<float>(initalAudioBufSize);
	frequencyShiftHz = _frequencyShiftHz;

}

bool FmDemod::addRawIQSamples(uint8_t* buf, uint32_t len) {
	float i, q;
	for (uint32_t n = 0; n < len; n += 2) {
		i = buf[n] - 127.5f;
		q = buf[n + 1] - 127.5f;
		if (!IQSampleInput.emplace(i, q)) {
			std::cout << "Error in memory allocation" << std::endl;
			return false;
		}
	}
	return true;
}

bool FmDemod::getAudioSamples(float* buf, uint32_t len){
	bool ret = true;
	for (uint32_t i = 0; i < len; i++) {
		if (!audioSampleOutput.try_dequeue(buf[i])) {
			buf[i] = 0.0f;
			ret = false;
		}
	}
	return ret;
}

void FmDemod::startProcessing(){
	std::thread(&FmDemod::processingLoop, this).detach();
}

void FmDemod::processingLoop(){
	std::complex<float>* IQFrame = new std::complex<float>[frameSize];
	uint32_t CurFrameLen = 0;
	float* realFrame = new float[frameSize];

	/* DSP State storage */
	uint32_t nFreqShift = 0;
	std::complex<float>* shiftLookupValues = nullptr;
	uint32_t freqShiftPeriod = calcuateFrequencyShiftLookup(shiftLookupValues);

	std::complex<float> bandwidthFIRFilterState[2*bandwidthFilterOrder] = { 0 };

	uint32_t nFirstDecimate = 0;

	float lastPhase = 0;

	float derivativeFIRFilterState[2*derivativeFilterOrder] = { 0 };

	uint32_t nLastDecimate = 0;

	float lastDeEmphInput = 0;
	float lastDeEmphOutput = 0;

	while (true) {
		if (IQSampleInput.size_approx() < frameSize)
			continue;
		for (uint32_t i = 0; i < frameSize; i++) {
			if (!IQSampleInput.try_dequeue(IQFrame[i])) {
				std::cout << "Not enough samples to assemble a frame" << std::endl;
			}
			
		}
		CurFrameLen = frameSize;

		frequencyShift(IQFrame, CurFrameLen, nFreqShift, shiftLookupValues, freqShiftPeriod);

		applyFIRFilter(bandwidthFilterCoeff, bandwidthFilterOrder, IQFrame, CurFrameLen, bandwidthFIRFilterState);

		CurFrameLen = decimate(10, IQFrame, CurFrameLen, nFirstDecimate);

		calculatePhase(IQFrame, realFrame, CurFrameLen, lastPhase);

		applyFIRFilter(derivativeFilterCoeff, derivativeFilterOrder, realFrame, CurFrameLen, derivativeFIRFilterState);

		deEmphasis(realFrame, CurFrameLen, lastDeEmphInput, lastDeEmphOutput);

		CurFrameLen = decimate(5, realFrame, CurFrameLen, nLastDecimate);

		for (uint32_t i = 0; i < CurFrameLen; i++) {
			if (!audioSampleOutput.enqueue(realFrame[i] * 0.05f)) {
				std::cout << "Error in memory allocation" << std::endl;
			}
		}

	}
	delete[] IQFrame;
	delete[] realFrame;
	delete[] shiftLookupValues;
}

uint32_t FmDemod::calcuateFrequencyShiftLookup(std::complex<float>*& lookupvalues) {
	uint32_t period = (uint32_t)std::abs(std::_Lcm(frequencyShiftHz, IQSampleRate)/std::_Gcd(frequencyShiftHz, IQSampleRate));
	lookupvalues = new std::complex<float>[period];
	for (uint32_t n = 0; n < period; n++) {
		lookupvalues[n] = std::exp((float)frequencyShiftHz / IQSampleRate * 2 * PI * n * -1if);
	}
	return period;
}

/* Shift the IQ baseband by frequencyShiftHz */
void FmDemod::frequencyShift(std::complex<float>* samples, uint32_t len, uint32_t& n, std::complex<float>* lookupvalues, uint32_t freqShiftPeriod) {
	if (freqShiftPeriod == 0)
		return; 
	for (uint32_t i = 0; i < len; i++) {
		samples[i] *= lookupvalues[n++];
		n %= freqShiftPeriod;
	}
}

template <class T> void FmDemod::applyFIRFilter(const float* coeffs, const uint32_t order, T* samples, const uint32_t len, T* firState) {
	T* firStateTemp = new T[order - 1];
	T newValue;
	for (uint32_t i = len - 1; i >= (len - order + 1); i--) {
		newValue = 0;
		for (uint32_t j = 0; j < order; j++) {
			newValue += samples[i - j] * coeffs[j];
		}
		firStateTemp[order - (len - i + 1)] = samples[i];
		samples[i] = newValue;
	}

	for (uint32_t i = len - order; i >= (order - 1); i--) {
		newValue = 0;
		for (uint32_t j = 0; j < order; j++) {
			newValue += samples[i - j] * coeffs[j];
		}
		samples[i] = newValue;
	}

	for (uint32_t i = order - 1; (i--) > 0;) {
		newValue = 0;
		for (uint32_t j = 0; j < order; j++) {
			if (j > i) {
				newValue += firState[order - (j - i) - 1] * coeffs[j];
			} else {
				newValue += samples[i - j] * coeffs[j];
			}
		}
		samples[i] = newValue;
	}
	
	memcpy(firState, firStateTemp, sizeof(T) * (order - 1));
	delete[] firStateTemp;
}

template <class T> uint32_t FmDemod::decimate(const uint8_t ratio, T* samples, const uint32_t inLen, uint32_t& n) {
	uint32_t outlen = 0;
	for (uint32_t i = 0; i < inLen; i++) {
		n %= ratio; 
		if (n++ == 0) {
			samples[outlen++] = samples[i];
		}
	}
	return outlen;
}

/* std::arg and std::floor are slow (probably) but it might be ok  */
void FmDemod::calculatePhase(const std::complex<float>* samples, float* phase, const uint32_t len, float& lastPhase) {
	float phaseDiff;
	for (uint32_t i = 0; i < len; ++i) {
		phaseDiff = std::arg(samples[i]) - lastPhase;
		phaseDiff -= 2 * PI * std::floor((phaseDiff + PI) / (2 * PI));

		lastPhase += phaseDiff;
		phase[i] = lastPhase;
	}
}

/* IIR filter that implments a RC lowpass filter with a time constant of 75us */
void FmDemod::deEmphasis(float* samples, const uint32_t len, float& lastInput, float& lastOutput) {
	for (uint32_t i = 0; i < len; i++) {
		lastOutput = samples[i] * deEmpasisB + lastInput * deEmpasisB - deEmpasisA * lastOutput;
		lastInput = samples[i];
		samples[i] = lastOutput;
	}
}