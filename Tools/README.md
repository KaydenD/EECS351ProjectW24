# Tools

- `rtl_sdr.exe` Used to record IQ samples from SDR. Example: `rtl_sdr.exe -f 96.3e6 -s 2.4e6 -n 24e6 rawiqsamples.bin` where n is the total number of samples we want to record. These are precomplied binaries from the [osmocom librtlsdr](https://gitea.osmocom.org/sdr/rtl-sdr/) project. There is also a popular [fork](https://github.com/librtlsdr/librtlsdr) that seems to have some improvements.
- `librtlsdr.dll`, `libusb-1.0.dll`, and `libwinpthread-1.dll` are just the DLLs used by rtl_sdr.exe. Our final C/C++ program will likely require these 3 DLLs as well.
- `sdrpp_windows_x64.zip` SDR++ is a plug and play program that can interact with a variety of SDRs and has built in demodulation. This is being used as a working example that we can compare against. Note: it doesn't have the ability to output multiple simultaneous audio steams like we intend to.

