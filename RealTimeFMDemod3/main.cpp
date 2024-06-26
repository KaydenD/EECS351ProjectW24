﻿#include <iostream>
#include "rtl-sdr-helpers.hpp"
#include <windows.h>
#include "fm-demod/fm-demod.hpp"
#include <RtAudio.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <chrono>


const uint32_t SDR_SAMPLE_RATE = 2400000U;
const uint32_t SDR_BUFFER_SIZE = 512 * 2 * 100;
const uint32_t AUDIO_BUFFER_SIZE = SDR_BUFFER_SIZE / 100;

using namespace std::chrono_literals;
const auto IQFileReadPeriod = std::chrono::microseconds(SDR_BUFFER_SIZE * 1000000 / (SDR_SAMPLE_RATE * 2));

rtlsdr_dev_t* dev = nullptr;

bool WINAPI sighandler(int signum) {
	if (signum == CTRL_C_EVENT) {
		std::cout << "Signal caught, exiting!" << std::endl;
		rtlsdr_cancel_async(dev);
		return true;
	}
	return false;
}

//std::ofstream fout("IQRaw2.bin", std::ios::binary);
void rtlsdr_callback(uint8_t* buf, uint32_t len, void* ctx) {
	std::vector<FmDemod*>* demodulators = (std::vector<FmDemod*>*)ctx;
	for (uint32_t i = 0; i < demodulators->size(); i++) {
		if (!demodulators->at(i)->addRawIQSamples(buf, len)) {
			rtlsdr_cancel_async(dev);
			return;
		}
	}
	//fout.write((char*)buf, len);
}

void readIQRecordingWorker(std::string filename, std::vector<FmDemod*>* demodulators) {
	std::ifstream fin(filename, std::ios::binary);
	uint8_t buf[SDR_BUFFER_SIZE];
	while (1) {
		if (fin.read((char*)buf, SDR_BUFFER_SIZE)) {
			for (uint32_t i = 0; i < demodulators->size(); i++) {
				if (!demodulators->at(i)->addRawIQSamples(buf, SDR_BUFFER_SIZE)) {
					std::cout << "Something bad happened" << std::endl;
					goto cleanup;
				}
			}
			std::this_thread::sleep_for(IQFileReadPeriod);
		} else {
			fin.clear();
			fin.seekg(0);
		}
	}
cleanup:
	fin.close();
}

int audio_callback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* ctx) {
	float* buffer = (float*)outputBuffer;
	FmDemod* demodulator = (FmDemod*)ctx;
	if (!demodulator->getAudioSamples(buffer, nBufferFrames)) {
		std::cout << "Not enough audio samples" << std::endl;
	}
	memcpy(buffer + nBufferFrames, buffer, nBufferFrames * sizeof(float));
	return 0;
}

int main(){
	/*
	std::ifstream fin("IQRaw.bin", std::ios::binary);
	uint8_t testIQ[SDR_BUFFER_SIZE*2];
	fin.read((char*) testIQ, SDR_BUFFER_SIZE*2);
	FmDemod test(SDR_BUFFER_SIZE / 2, 512, 0);
	test.addRawIQSamples(testIQ, SDR_BUFFER_SIZE*2);
	test.startProcessing();
	*/

	std::vector<FmDemod*> demodulators;
	std::vector<RtAudio*> outputs;
	std::string temp;

	std::cout << "Use IQ Recording? y/n [n]: ";
	std::getline(std::cin, temp);
	temp += "n";
	bool useRecording = temp.front() == 'Y' || temp.front() == 'y';
	std::string filename;
	if (useRecording) {
		std::cout << "Enter recording filename [iq.bin]: ";
		std::getline(std::cin, filename);
		if (filename.empty()) {
			filename = "iq.bin";
		}
	}

	std::cout << "Enter desired SDR center frequency (Hz) (eg 96.3e6): ";
	double centerFreq = 0;
	std::cin >> centerFreq;
	if (centerFreq > 108e6 || centerFreq < 88e6) {
		std::cout << "Center frequency must fall inside the US FM broadcast range 88.0MHz to 108MHz." << std::endl;
		return 1;
	}
	boolean done = false;
	while (!done) {
		double stationFreq = 0;
		std::cout << "Enter desired FM station frequency (must be within 1 MHz of the center frequency) (Hz): ";
		std::cin >> stationFreq;
		if (std::abs(stationFreq - centerFreq) > 1e6) {
			std::cout << "FM station must be within 1 MHz of the center frequency" << std::endl;
			continue;
		}

		RtAudio* audioOutput = new RtAudio();

		if (audioOutput->getDeviceIds().size() == 0) {
			std::cout << "No audio output devices found" << std::endl;
			delete audioOutput;
			return 1;
		}

		std::vector<uint32_t> ids = audioOutput->getDeviceIds();
		for (uint32_t id : ids) {
			RtAudio::DeviceInfo devInfo = audioOutput->getDeviceInfo(id);
			if (devInfo.outputChannels >= 2) {
				std::cout << "[" << id << "]: " << devInfo.name << std::endl;
			}
		}

		std::cout << "Please pick a output device id to play this station on [" << audioOutput->getDefaultOutputDevice() << "]: ";
		RtAudio::StreamParameters parameters;
		std::string deviceId;

		std::cin.ignore();
		std::getline(std::cin, deviceId);
		try {
			parameters.deviceId = stoi(deviceId);
		} catch(const std::exception e) {
			parameters.deviceId = 0;
		}
		if (std::find(ids.begin(), ids.end(), parameters.deviceId) == ids.end()) {
			parameters.deviceId = audioOutput->getDefaultOutputDevice();
		}

		std::cout << "Add more channels y/n [n]: ";
		temp.clear();
		std::getline(std::cin, temp);
		temp += "n";
		done = temp.front() != 'Y' && temp.front() != 'y';

		demodulators.push_back(new FmDemod(SDR_BUFFER_SIZE / 2, AUDIO_BUFFER_SIZE * 4, (int32_t)(stationFreq - centerFreq)));

		parameters.nChannels = 2;
		parameters.firstChannel = 0;
		uint32_t bufferFrames = AUDIO_BUFFER_SIZE;
		RtAudio::StreamOptions so;
		so.flags = RTAUDIO_NONINTERLEAVED;
		audioOutput->openStream(&parameters, NULL, RTAUDIO_FLOAT32, 48000U, &bufferFrames, &audio_callback, (void*)demodulators.back(), &so);
		outputs.push_back(audioOutput);

	}
	std::thread sdrthread;
	if(!useRecording){
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
		verbose_set_frequency(dev, (uint32_t)centerFreq);
		rtlsdr_set_bias_tee(dev, 1);
		verbose_auto_gain(dev);
		verbose_reset_buffer(dev);

		sdrthread = std::thread(rtlsdr_read_async, dev, rtlsdr_callback, (void*)&demodulators, 0, SDR_BUFFER_SIZE);
	} else {
		sdrthread = std::thread(readIQRecordingWorker, "iq.bin", &demodulators);
	}

	for (uint32_t i = 0; i < demodulators.size(); i++) {
		demodulators[i]->startProcessing();
	}

	for (uint32_t i = 0; i < outputs.size(); i++) {
		if (outputs[i]->startStream()) {
			std::cout << outputs[i]->getErrorText() << std::endl;
			rtlsdr_cancel_async(dev);
		}
	}

	sdrthread.join();

	rtlsdr_close(dev);
	for (uint32_t i = 0; i < outputs.size(); i++) {
		if (outputs[i]->isStreamRunning()) {
			outputs[i]->closeStream();
		}
	}

	return 0;
}
