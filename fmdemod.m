%data captured using command rtl_sdr.exe -f 96.3e6 -s 2.4e6 -n 24e6 rawiqsamples.bin
fileID = fopen('BasebandRecordings/IQRaw.bin');
[rawADCIQSamples, n] = fread(fileID,'uint8'); %iq interleaved starting with i
fs = 2.4e6;
%rawADCIQSamples = rawADCIQSamples(1:16384);
%n = 16384;
iq = zeros([n/2, 1]);
for i = 1:(n/2) 
    %uint8 goes from 0 to 255. The following scales it to -1 to 1.
    %If there is any DC offset in the raw samples, it may be useful to adjust
    %the 127.5 number in the subtraction. Some people seem to use 127.4, but I 
    %couldn't notice a difference 
    iq(i) = (double(rawADCIQSamples(i * 2 - 1)) - 127.5) + j*(double(rawADCIQSamples(i * 2)) - 127.5);
            
end


%figure(1);
%plotFFT(iq(1:25e3), fs);
%title('FFT of IQ signal');

%Offset from the center freq that we want to demodulate
n = (0:length(iq)-1)';
%If center freq is 96.3MHz, then a shift of -0.8MHz gets us 95.5MHz and 0.8MHz gets us 97.1MHz
%95.5MHz, 96.3MHz, 97.1MHz are all FM stations in my area that get good ish reception. 
shift = -0.8e6; 
iq = iq .* exp(j*-shift/2.4e6*2*pi.*n);

%figure(2);
%plotFFT(iq(1:25e3), fs);
%title('FFT after shift');

%Only need about 200 kHz of bandwidth around center of FM signal
b = fir1(10, 100e3/fs);
iq = filter(b, 1, iq);


%figure(7);
%plot(real(iq(1:2000)/max(abs(iq(1:2000)))),imag(iq(1:2000)/max(abs(iq(1:2000)))),"o")
%xlabel('Real');
%ylabel('Imag');

%decimate to make calcuations faster 
iq = downsample(iq, 10);
fs = fs/10;

%figure(3);
%plotFFT(iq(1:25e3), fs);
%title('FFT after lowpass and decimation');

%Actual demodulation. First get real phase from complex IQ signal (need to unwrap it so there are no sudden jumps that would make derivative undefined)
%Then take derivative of phase signal using differentiation filter

demod = unwrap(angle(iq));

%design a 7th order FIR filter that approximates a staight line from 0 to
%0.75 * pi. the [1 0 -1] filter might be good enough here
b = firls(6,[0 0.75],[0 0.75*pi],'d');
%apply the filter we just designed
demod = filter(b, 1, demod);

%figure(4);
%plotFFT(demod(1:25e3), fs);
%title('FFT after demodulation');

%scale audio signal to compensate for some constants that come out in the 
%demod process.
audio = demod;%/(2*pi*(1/fs)*15000);

%We don't need anything above ~18kHz because we only care about the 
%mono audio
%b = fir1(2, 18e3/fs);
%audio = filter(b, 1, audio);

%Bring it down to the final audio sample rate

%De-emphasis filter H(w) = 1/(1+j*w*tau) so apply bilinear to get discrete
%filter
[b,a] = bilinear(1, [75e-6 1], fs);
audio = filter(b, a, audio);
%figure(5);
%freqz(b, a);

audio = downsample(audio, 5);
fs = fs/5;

%figure(6);
%plotFFT(audio(1:25e3), fs);
%title('FFT of audio after deemphasis');

%Play audio with lower volume
sound(audio*0.2, fs);


function plotFFT(data, fs)
    N = length(data);
    fftdata = abs(fftshift(fft(data.*hann(N))));
    dF = fs/N;
    f = -fs/2:dF:fs/2-dF;
    plot(f,fftdata);
    xlabel('Frequency (in hertz)');
    title('Magnitude Response');
end