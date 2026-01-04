#!/usr/bin/env python3
"""
Digitize a spectrum plot image and export values at 5 nm intervals.

Usage:
  python3 scripts/digitize_spectrum_image.py data/garage_spectrum.png

Outputs: same directory, file named <stem>_5nm.csv

Notes:
- This uses simple image heuristics: detects the plotting rectangle, maps
  pixel x->wavelength (380..780 nm) and pixel y->relative value (0..1),
  then finds the topmost colored pixel per column as the curve, and
  interpolates to 5 nm steps. Accuracy depends on image clarity.
"""
from PIL import Image
from PIL import ImageDraw
import sys
import numpy as np
from pathlib import Path


def find_plot_bbox(im):
    gray = np.array(im.convert("L"))
    h, w = gray.shape

    # Strong dark pixels (axes)
    dark = gray < 120

    # Sum dark pixels per row / column
    row_sum = dark.sum(axis=1)
    col_sum = dark.sum(axis=0)

    # Horizontal axis lines: long dark runs
    row_idxs = np.where(row_sum > w * 0.4)[0]
    col_idxs = np.where(col_sum > h * 0.4)[0]

    if len(row_idxs) < 2 or len(col_idxs) < 2:
        raise RuntimeError("Could not detect plot axes")

    # Topmost and bottommost horizontal axis
    top = row_idxs[0]
    bottom = row_idxs[-1]

    # Leftmost and rightmost vertical axis
    left = col_idxs[0]
    right = col_idxs[-1]

    # Tighten slightly to stay inside the axes
    inset = 2
    return (
        left + inset,
        top + inset,
        right - inset,
        bottom - inset,
    )

def is_colored_pixel(px):
    # px is (r,g,b) or (r,g,b,a)
    r, g, b = px[:3]
    # consider pixel colored if it's not near-white and not very dark
    if r > 245 and g > 245 and b > 245:
        return False
    # also ignore near-black (axis lines) to avoid picking grid lines
    if r < 30 and g < 30 and b < 30:
        return False
    return True


def extract_curve(im, bbox):
    left, top, right, bottom = bbox
    crop = im.crop((left, top, right + 1, bottom + 1))
    arr = np.array(crop.convert('RGBA'))
    h, w, _ = arr.shape
    xs = []
    ys = []
    for x in range(w):
        col = arr[:, x, :3]
        y_found = None
        # scan from top to bottom to find first colored pixel
        for y in range(h):
            if is_colored_pixel(col[y]):
                y_found = y
                break
        if y_found is None:
            # try from bottom up to catch outlines
            for y in range(h - 1, -1, -1):
                if is_colored_pixel(col[y]):
                    y_found = y
                    break
        if y_found is None:
            # mark as NaN
            xs.append(x)
            ys.append(np.nan)
        else:
            xs.append(x)
            ys.append(y_found)
    # convert to image-space coordinates
    xs = np.array(xs, dtype=float)
    ys = np.array(ys, dtype=float)
    return xs, ys, (left, top, right, bottom)


def map_pixels_to_physical(xs, ys, bbox, wavelength_min=380.0, wavelength_max=780.0):
    left, top, right, bottom = bbox
    width = right - left + 1
    height = bottom - top + 1
    # map x: left -> wavelength_min, right -> wavelength_max
    wavelengths = wavelength_min + (xs / (width - 1)) * (wavelength_max - wavelength_min)
    # map y: top -> 1.0, bottom -> 0.0
    values = 1.0 - (ys / (height - 1))
    return wavelengths, values


def clean_and_interpolate(wav, val):
    # remove NaNs
    mask = ~np.isnan(val)
    if mask.sum() < 2:
        raise RuntimeError('Not enough detected curve points')
    wavc = wav[mask]
    valc = val[mask]
    # smooth small gaps by linear interpolation
    # produce values at exact 5 nm steps
    new_wav = np.arange(380.0, 780.0 + 1e-6, 10.0)
    new_val = np.interp(new_wav, wavc, valc)
    return new_wav, new_val


def write_bbox_debug_image(im, bbox, out_path):
    """
    Write a copy of the image with the detected bbox drawn on top.
    """
    debug = im.copy()
    draw = ImageDraw.Draw(debug)

    left, top, right, bottom = bbox

    # draw rectangle (red)
    draw.rectangle(
        [(left, top), (right, bottom)],
        outline=(255, 0, 0),
        width=3
    )

    debug.save(out_path)

def write_curve_debug_image(im, bbox, xs, ys, out_path, line_width=10):
    """
    Write an image with the detected curve overlaid on top of the plot.
    """
    debug = im.copy()
    draw = ImageDraw.Draw(debug)

    left, top, right, bottom = bbox

    # Draw bbox
    draw.rectangle(
        [(left, top), (right, bottom)],
        outline=(255, 0, 0),
        width=2
    )

    # Build polyline of valid points
    points = []
    for x, y in zip(xs, ys):
        if np.isnan(y):
            continue
        px = int(left + x)
        py = int(top + y)
        points.append((px, py))

    # Draw thick curve
    if len(points) > 1:
        draw.line(
            points,
            fill=(0, 255, 0),
            width=line_width,
            joint="curve"
        )

    debug.save(out_path)

def main():
    if len(sys.argv) < 2:
        print('Usage: python3 scripts/digitize_spectrum_image.py data/garage_spectrum.png')
        sys.exit(1)
    inp = Path(sys.argv[1])
    if not inp.exists():
        print('Input not found:', inp)
        sys.exit(2)
    im = Image.open(inp).convert('RGB')
    bbox = find_plot_bbox(im)

    # write debug image with bbox overlay
    debug_out = inp.with_name(inp.stem + '_bbox_debug.png')
    write_bbox_debug_image(im, bbox, debug_out)

    print('Wrote bbox debug image:', debug_out)

    xs, ys, bbox = extract_curve(im, bbox)

    # write curve debug image
    curve_debug_out = inp.with_name(inp.stem + '_curve_debug.png')
    write_curve_debug_image(im, bbox, xs, ys, curve_debug_out)
    print('Wrote curve debug image:', curve_debug_out)
    
    wav, val = map_pixels_to_physical(xs, ys, bbox)
    new_wav, new_val = clean_and_interpolate(wav, val)
    outp = inp.with_name(inp.stem + '_10nm.csv')
    with open(outp, 'w') as f:
        f.write('wavelength_nm,value\n')
        for wv, vv in zip(new_wav, new_val):
            f.write(f'{wv:.1f},{vv:.6f}\n')
    print('Wrote', outp)


if __name__ == '__main__':
    main()
