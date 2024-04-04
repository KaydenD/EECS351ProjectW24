#include <iostream>
#include "rtl-sdr-helpers.hpp"
#include <windows.h>
#include "fm-demod/fm-demod.hpp"
#include <RtAudio.h>
#include <thread>
#include <fstream>

const uint32_t SDR_SAMPLE_RATE = 2400000U;
const uint32_t SDR_BUFFER_SIZE = 16384;
rtlsdr_dev_t* dev = nullptr;

bool WINAPI sighandler(int signum) {
	if (signum == CTRL_C_EVENT) {
		std::cout << "Signal caught, exiting!" << std::endl;
		rtlsdr_cancel_async(dev);
		return true;
	}
	return false;
}

void rtlsdr_callback(uint8_t* buf, uint32_t len, void* ctx) {
	std::vector<FmDemod>* demodulators = (std::vector<FmDemod>*)ctx;
	for (uint32_t i = 0; i < demodulators->size(); i++) {
		if (!demodulators->at(i).addRawIQSamples(buf, len)) {
			rtlsdr_cancel_async(dev);
			return;
		}
	}
}

int audio_callback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* ctx) {
	float* buffer = (float*)outputBuffer;
	FmDemod* demodulator = (FmDemod*)ctx;
	if (!demodulator->getAudioSamples(buffer, nBufferFrames)) {
		std::cout << "Not enough audio samples" << std::endl;
	}
	return 0;
}

int main(){
	/*
	std::ifstream fin("IQRaw.bin", std::ios::binary);
	uint8_t testIQ[SDR_BUFFER_SIZE];
	fin.read((char*) testIQ, SDR_BUFFER_SIZE);
	FmDemod test(SDR_BUFFER_SIZE / 2, 512, 0);
	test.addRawIQSamples(testIQ, SDR_BUFFER_SIZE);
	test.startProcessing();
	*/

	std::vector<FmDemod> demodulators;
	demodulators.push_back(FmDemod(SDR_BUFFER_SIZE / 2, 512, 0));

	RtAudio audioOutput;
	if (audioOutput.getDeviceIds().size() == 0) {
		std::cout << "No audio output devices found" << std::endl;
		return 1;
	}
	RtAudio::StreamParameters parameters;
	parameters.deviceId = audioOutput.getDefaultOutputDevice();
	parameters.nChannels = 1;
	parameters.firstChannel = 0;
	uint32_t bufferFrames = 256;
	audioOutput.openStream(&parameters, NULL, RTAUDIO_FLOAT32, 48000U, &bufferFrames, &audio_callback, (void*)&demodulators[0]);

	int devIndex = verbose_device_search("0");
	if (devIndex < 0) {
		return 1;
	}
	if (rtlsdr_open(&dev, (uint32_t)devIndex) < 0) {
		std::cout << "Failed to open SDR!" << std::endl;
		return 1;
	}

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)sighandler, true);

	verbose_set_sample_rate(dev, SDR_SAMPLE_RATE);
	verbose_set_frequency(dev, 96300000U);
	verbose_auto_gain(dev);
	verbose_reset_buffer(dev);

	std::thread sdrthread(rtlsdr_read_async, dev, rtlsdr_callback, (void*)&demodulators, 0, SDR_BUFFER_SIZE);
	demodulators[0].startProcessing();

	if (audioOutput.startStream()) {
		std::cout << audioOutput.getErrorText() << std::endl;
		rtlsdr_cancel_async(dev);
	}

	sdrthread.join();

	rtlsdr_close(dev);

	if (audioOutput.isStreamRunning()) {
		audioOutput.closeStream();
	}

	return 0;
}
