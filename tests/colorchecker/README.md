Sekonic C-700 Spectrum
==================

This example demonstrates how to **digitize a spectral power distribution (SPD) from a Sekonic C-700 spectrum image** and use the extracted data for colorimetric validation and testing.

The purpose is **not to recover exact instrument data**, but to produce a **reasonable spectral approximation** suitable for downstream spectral analysis and color pipeline verification.

## Purpose

- Approximate illuminant spectral data from a Sekonic C-700 screenshot.
- Validate extracted spectra against:
  - Measured reflectance curves
  - CIE color matching functions (CMFs)
- Verify color space output from camera pipelines (e.g. Canon 5D).

This example acts as a bridge between:
- Instrument screenshots
- Spectral mathematics
- Practical color validation workflows

## Usage

Run the digitization script on a Sekonic C-700 spectrum image:

```bash
python3 sekonic_c-700-digitize_spectrum_image.py ./sekonic_c-700_5392K_SpectralDistribution.jpg
````

## Reference Colorimetric Data

Colorimetric validation in this example uses CIE color matching functions (CMFs) obtained from the
Colour & Vision Research Laboratory (CVRL), University College London:

http://cvrl.ucl.ac.uk/cmfs.htm

Specifically:

- CIE 1931 2° observer
- XYZ color matching functions
- Modified by Judd (1951) and Vos (1978)

These CMFs are used to convert the extracted spectral data into CIE XYZ tristimulus values for comparison and validation.


## Reference ColorChecker Classic spectra data

The reflectance spectra are sourced from **BabelColor**, which provides measured spectral data for the original Macbeth ColorChecker under standardized conditions:

https://babelcolor.com/colorchecker-2.htm

Specifically:

- ColorChecker Classic (24 patches)
- Spectral reflectance
- Wavelength range 380–780 nm
