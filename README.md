# EECS351ProjectW24
## WFM Demodulation using an RTL-SDR

### Project Description
The primary objective of this project is to achieve real-time demodulation of multiple broadcast FM stations using an economical commercial off-the-shelf (COTS) software-defined radio (SDR), specifically the RTL-SDR. The RTL-SDR serves as our source of IQ data, capturing a segment of the RF spectrum that we can process efficiently.

By configuring the RTL-SDR with a center frequency \(f_0\) and a sampling rate \(f_s\), it provides a stream of 8-bit ADC samples with interleaved In-phase (I) and Quadrature (Q) components, effectively doubling the data rate to \(2f_s\). The bandwidth of the captured IQ data is equal to the commanded sample rate. For this project, we utilize a sample rate of 2.4 Msps, corresponding to a bandwidth of 2.4 MHz. Given that commercial FM stations occupy approximately 200 kHz of bandwidth, we theoretically can demodulate up to 12 stations simultaneously. However, due to regulatory spacing, we realistically expect to handle about three stations simultaneously.

### Demodulation Process
Demodulation exploits the crucial relationship between instantaneous phase and frequency; specifically, instantaneous frequency is the derivative of phase. The downconversion of the signal to IQ baseband in the SDR provides direct access to the signal's phase, calculated as the angle formed by the complex number \(I[n] + jQ[n]\), or \(atan2(Q[n], I[n])\). Differentiating these angles—after unwrapping them to avoid discontinuities—yields the instantaneous frequency. As FM encodes changes in the baseband amplitude as changes in carrier frequency, recovering the instantaneous frequency allows us to retrieve the baseband signal, modified by certain constant factors.

Further processing steps include filtering and decimation to reduce the data volume. A critical and somewhat counterintuitive step is applying a de-emphasis filter post-demodulation. This step counteracts the pre-emphasis filter applied before modulation, which boosts the high-frequency components to enhance signal noise resistance. Without de-emphasis, the audio output would sound distorted. The de-emphasis filter, a simple RC lowpass filter, is characterized by a time constant of 75us in the US.

### Project Structure

- **`fmdemod.m`**: A MATLAB script capable of demodulating FM stations from an IQ recording.
- **`/BasebandRecordings/96e3HzBW24e5Hz.bin`**: An IQ recording of 96.3 MHz with a 2.4 MHz bandwidth, utilized in the aforementioned MATLAB script.
- **`/Tools`**: Assorted utilities for capturing or interacting with the RTL-SDR. The final aim is to eliminate the need for these tools.
- **`/Screenshots`**: Various images of our setup, including FFTs and frequency response graphs.
- **`/RealTimeFMDemod3`**: The C++ source code for real-time demodulation.