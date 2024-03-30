# EECS351ProjectW24
WFM Demodulation using a RTL-SDR

## Project Description
The goal of this project is successfully demodulation multiple broadcast FM stations in real time using a cheap COTS SDR receiver called a RTL-SDR. 
The RTL-SDR is simply our data source. It provides a convenient and well understood way of obtaining IQ data. Simply provide the RTL-SDR with a 
center frequency f<sub>0</sub> and sample rate f<sub>s</sub>, and it will provide a stream of 8 bit ADC samples with I and Q interleaved. (both I and Q are sampled at f<sub>s</sub> so the raw output from is SDR is actually 2*f<sub>s</sub>).

This IQ data represents a segment of the RF spectrum that we can easily process. The bandwidth of IQ data is equal to the sample rate that was commanded. For our project,
we will be using a sample rate of 2.4Msps representing a 2.4MHz bandwidth capatured. Since commercial FM station only occupy about 200kHz of bandwidth, we should be able to demodulate, in theroy, about 12 stations at the same time.
In reality, the FCC spaces stations a little further apart than that so about 3 stations simultaneously is about all we can expect. 

The demodulation will be done using an important property of instantaneous phase and frequency; that is instantaneous frequency is the derivative of phase. Because our signal has been downcoverted to a IQ baseband in the SDR, we have
direct access to the phase of the signal. It is simply the angle formed by the complex number I[n] + j*Q[n], or atan2(Q[n], I[n]). Taking the derivative of these angles (insuring they have been unwarped to avoid any sudden jumps) gets up back 
to our instantaneous frequency. Recall that FM is simply encoding changes in the baseband amplitude to changes in carrier frequency. So since we have recovered the changes in carrier frequency, we therefore have recovered the baseband signal (times some constant factors).

Before and after this demodulation, we have to do some filtering and decimation to decrease the number of samples that need to be processed. One particularly unintuitive filtering step is the de-emphasis filter applied to the audio after demoduation. Before the audio was modulated onto the carrier
a pre-emphasis filter was applied that boosted the high frequency components of the signal. This is done to increase the noise resistance of the signal (somehow?). If we listen to the audio without a de-emphasis filter, it will sound slight stange. So we apply a de-emphasis filter (a simple RC lowpass filter) to the audio before playing it back.
The exact value is RC is specified by their time constant which is 75us in the US.
## File outline

- `fmdemod.m` A matlab script which can demodulate the FM stations from a IQ recording.
- `/BasebandRecordings/96e3HzBW24e5Hz.bin` An IQ recording of 96.3MHz with a 2.4MHz bandwidth. Used as the recording in previous matlab script.
- `/Tools` Various tools used to capture or interact with the RTL-SDR. The goal is that we shouldn't need any of these tools in the end.
- `/Screenshots` Various pictures of our setup, FFTs, and frequency reponse graphs. 
## TODO

- Implement the DSP in C/C++;
- Use the librtlsdr libary to interact with SDR in real time.
- Use RtAudio to output the demodulated audio. 