// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/specviz

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>

struct SpectralSample
{
    double wavelength_nm = 0.0;
    double value = 0.0;
};

struct ColorCheckerPatch
{
    int number = 0;
    std::string name;
    std::vector<SpectralSample> samples; // reflectance
};

struct IlluminantSPD
{
    std::string name;
    std::vector<SpectralSample> samples; // power
};

struct CMFSample
{
    double wavelength_nm = 0.0;
    double xbar = 0.0;
    double ybar = 0.0;
    double zbar = 0.0;
};

struct RGB
{
    double r, g, b;
};

struct RGB8
{
    int r, g, b;
};

struct XY
{
    double x, y;
};

struct XYZ
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

static std::vector<std::string>
split_csv_line(const std::string& line)
{
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, ','))
        tokens.push_back(token);

    return tokens;
}

static std::vector<ColorCheckerPatch>
parse_colorchecker_csv(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file)
        throw std::runtime_error("Failed to open ColorChecker CSV");

    std::string line;

    // read header and extract wavelengths
    if (!std::getline(file, line))
        throw std::runtime_error("Empty CSV file");

    auto header = split_csv_line(line);
    if (header.size() < 3)
        throw std::runtime_error("Invalid ColorChecker CSV header");

    // header format: no,name,380,390,400,...
    std::vector<double> wavelengths;
    for (size_t i = 2; i < header.size(); ++i)
        wavelengths.push_back(std::stod(header[i]));

    // read data rows
    std::vector<ColorCheckerPatch> patches;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        auto cols = split_csv_line(line);
        if (cols.size() < 2 + wavelengths.size())
            throw std::runtime_error("Row does not match header width");

        ColorCheckerPatch patch;
        patch.number = std::stoi(cols[0]);
        patch.name   = cols[1];

        for (size_t i = 0; i < wavelengths.size(); ++i)
        {
            double value = std::stod(cols[i + 2]);
            patch.samples.push_back({
                wavelengths[i],
                value
            });
        }

        patches.push_back(std::move(patch));
    }

    // ensure patches are ordered
    std::sort(
        patches.begin(),
        patches.end(),
        [](const ColorCheckerPatch& a, const ColorCheckerPatch& b) {
            return a.number < b.number;
        }
    );

    return patches;
}

static IlluminantSPD
parse_illuminant_csv(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file)
        throw std::runtime_error("Failed to open illuminant CSV");

    IlluminantSPD illuminant;
    illuminant.name = filename;

    std::string line;
    std::getline(file, line); // header

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        auto cols = split_csv_line(line);
        if (cols.size() < 2)
            continue;

        illuminant.samples.push_back({
            std::stod(cols[0]),
            std::stod(cols[1])
        });
    }

    std::sort(
        illuminant.samples.begin(),
        illuminant.samples.end(),
        [](const auto& a, const auto& b) {
            return a.wavelength_nm < b.wavelength_nm;
        });

    return illuminant;
}

static std::vector<CMFSample>
parse_cmf_csv(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file)
        throw std::runtime_error("Failed to open CMF CSV");

    std::string line;
    std::vector<CMFSample> cmfs;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        auto cols = split_csv_line(line);
        if (cols.size() < 4)
            continue;

        cmfs.push_back({
            std::stod(cols[0]),
            std::stod(cols[1]),
            std::stod(cols[2]),
            std::stod(cols[3])
        });
    }

    std::sort(cmfs.begin(), cmfs.end(),
        [](const auto& a, const auto& b) {
            return a.wavelength_nm < b.wavelength_nm;
        });

    return cmfs;
}

static std::vector<SpectralSample>
combine_reflectance_with_illuminant(
    const std::vector<SpectralSample>& reflectance,
    const std::vector<SpectralSample>& illuminant)
{
    std::vector<SpectralSample> result;

    size_t i = 0, j = 0;
    while (i < reflectance.size() && j < illuminant.size())
    {
        const auto& r = reflectance[i];
        const auto& e = illuminant[j];

        if (r.wavelength_nm == e.wavelength_nm)
        {
            result.push_back({
                r.wavelength_nm,
                r.value * e.value
            });
            ++i;
            ++j;
        }
        else if (r.wavelength_nm < e.wavelength_nm)
        {
            ++i;
        }
        else
        {
            ++j;
        }
    }

    return result;
}

static double srgb_encode(double x)
{
    if (x <= 0.0031308)
        return 12.92 * x;
    return 1.055 * std::pow(x, 1.0 / 2.4) - 0.055;
}

static RGB xyz_to_srgb(double X, double Y, double Z)
{
    double r =  3.2406 * X - 1.5372 * Y - 0.4986 * Z;
    double g = -0.9689 * X + 1.8758 * Y + 0.0415 * Z;
    double b =  0.0557 * X - 0.2040 * Y + 1.0570 * Z;

    r = std::max(0.0, r);
    g = std::max(0.0, g);
    b = std::max(0.0, b);

    return {
        srgb_encode(r),
        srgb_encode(g),
        srgb_encode(b)
    };
}

static RGB8 srgb_to_8bit(double r, double g, double b)
{
    auto to_u8 = [](double v) {
        v = std::clamp(v, 0.0, 1.0);
        return static_cast<int>(std::lround(v * 255.0));
    };

    return {
        to_u8(r),
        to_u8(g),
        to_u8(b)
    };
}

static XYZ
integrate_xyz(
    const std::vector<SpectralSample>& illuminated,
    const std::vector<CMFSample>& cmfs,
    const std::vector<SpectralSample>& illuminant,
    double delta_nm
)
{
    XYZ xyz{};
    double k_denom = 0.0;

    size_t i = 0, j = 0, k = 0;

    while (i < illuminated.size() &&
           j < cmfs.size() &&
           k < illuminant.size())
    {
        double wl = illuminated[i].wavelength_nm;

        if (wl == cmfs[j].wavelength_nm &&
            wl == illuminant[k].wavelength_nm)
        {
            const auto& S = illuminated[i].value;
            const auto& E = illuminant[k].value;
            const auto& cmf = cmfs[j];

            xyz.x += S * cmf.xbar;
            xyz.y += S * cmf.ybar;
            xyz.z += S * cmf.zbar;

            // normalization denominator (perfect diffuser)
            k_denom += E * cmf.ybar;

            ++i;
            ++j;
            ++k;
        }
        else
        {
            double next = std::max({
                illuminated[i].wavelength_nm,
                cmfs[j].wavelength_nm,
                illuminant[k].wavelength_nm
            });

            if (illuminated[i].wavelength_nm < next) ++i;
            if (cmfs[j].wavelength_nm < next) ++j;
            if (illuminant[k].wavelength_nm < next) ++k;
        }
    }
    if (k_denom > 0.0)
    {
        double scale = delta_nm / k_denom;
        xyz.x *= scale;
        xyz.y *= scale;
        xyz.z *= scale;
    }
    return xyz;
}

static inline XYZ mul3x3(const double M[3][3], const XYZ& v)
{
    return {
        M[0][0]*v.x + M[0][1]*v.y + M[0][2]*v.z,
        M[1][0]*v.x + M[1][1]*v.y + M[1][2]*v.z,
        M[2][0]*v.x + M[2][1]*v.y + M[2][2]*v.z
    };
}

static inline void mul3x3(const double A[3][3], const double B[3][3], double out[3][3])
{
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            out[r][c] = A[r][0]*B[0][c] + A[r][1]*B[1][c] + A[r][2]*B[2][c];
}

// Bradford matrix and inverse (standard)
static constexpr double M_BRADFORD[3][3] = {
    {  0.8951,  0.2664, -0.1614 },
    { -0.7502,  1.7135,  0.0367 },
    {  0.0389, -0.0685,  1.0296 }
};

static constexpr double M_BRADFORD_INV[3][3] = {
    {  0.9869929, -0.1470543,  0.1599627 },
    {  0.4323053,  0.5183603,  0.0492912 },
    { -0.0085287,  0.0400428,  0.9684867 }
};

static XYZ adapt_bradford(const XYZ& xyz, const XYZ& src_white_xyz, const XYZ& dst_white_xyz)
{
    // Convert whites to LMS
    XYZ src_lms = mul3x3(M_BRADFORD, src_white_xyz);
    XYZ dst_lms = mul3x3(M_BRADFORD, dst_white_xyz);

    // Diagonal scaling in LMS
    double D[3][3] = {
        { dst_lms.x / src_lms.x, 0.0, 0.0 },
        { 0.0, dst_lms.y / src_lms.y, 0.0 },
        { 0.0, 0.0, dst_lms.z / src_lms.z }
    };

    // Adaptation matrix: M^-1 * D * M
    double DM[3][3], A[3][3];
    mul3x3(D, M_BRADFORD, DM);
    mul3x3(M_BRADFORD_INV, DM, A);

    return mul3x3(A, xyz);
}



// Approximate daylight locus xy from CCT (valid ~4000K–25000K; OK-ish around 5392K)
static XY cct_to_xy_daylight(double T)
{
    // CIE daylight locus approximation (often used for D-series)
    // For your use (5392K), this is in a good range.
    double x;
    if (T <= 7000.0) {
        x = 0.244063 + 0.09911e3 / T + 2.9678e6 / (T*T) - 4.6070e9 / (T*T*T);
    } else {
        x = 0.237040 + 0.24748e3 / T + 1.9018e6 / (T*T) - 2.0064e9 / (T*T*T);
    }
    double y = -3.000 * x*x + 2.870 * x - 0.275;
    return { x, y };
}

static XYZ xyY_to_XYZ(double x, double y, double Y = 1.0)
{
    if (y <= 0.0) return {0,0,0};
    double X = (x / y) * Y;
    double Z = ((1.0 - x - y) / y) * Y;
    return { X, Y, Z };
}


static XYZ D65_white_XYZ()
{
    // D65: x=0.3127, y=0.3290
    return xyY_to_XYZ(0.3127, 0.3290, 1.0);
}

static XYZ adapt_measured_kelvin_to_D65(const XYZ& xyz, double measured_kelvin)
{
    XY src_xy = cct_to_xy_daylight(measured_kelvin);
    XYZ src_white = xyY_to_XYZ(src_xy.x, src_xy.y, 1.0);
    XYZ dst_white = D65_white_XYZ();
    return adapt_bradford(xyz, src_white, dst_white);
}


std::vector<RGB8> expand_patches(
    const std::vector<RGB8>& patches,
    int cols,
    int rows,
    int patch_size
)
{
    if (patches.size() != static_cast<size_t>(cols * rows))
        throw std::runtime_error("Patch count does not match grid size");

    int width  = cols * patch_size;
    int height = rows * patch_size;

    std::vector<RGB8> pixels(width * height);

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            const RGB8& color = patches[r * cols + c];

            for (int y = 0; y < patch_size; ++y)
            {
                for (int x = 0; x < patch_size; ++x)
                {
                    int px = c * patch_size + x;
                    int py = r * patch_size + y;
                    pixels[py * width + px] = color;
                }
            }
        }
    }

    return pixels;
}

void write_tga_rgb8(
    const std::string& filename,
    int width,
    int height,
    const std::vector<RGB8>& pixels
)
{
    if (pixels.size() != static_cast<size_t>(width * height))
        throw std::runtime_error("Pixel count does not match width × height");

    std::ofstream out(filename, std::ios::binary);
    if (!out)
        throw std::runtime_error("Failed to open TGA file");

    uint8_t header[18] = {};
    header[2]  = 2;  // Uncompressed true-color
    header[12] = width & 0xFF;
    header[13] = (width >> 8) & 0xFF;
    header[14] = height & 0xFF;
    header[15] = (height >> 8) & 0xFF;
    header[16] = 24; // bits per pixel
    header[17] = 0x20; // top-left origin

    out.write(reinterpret_cast<char*>(header), sizeof(header));

    // TGA expects BGR order
    for (const auto& p : pixels)
    {
        out.put(static_cast<char>(p.b));
        out.put(static_cast<char>(p.g));
        out.put(static_cast<char>(p.r));
    }
}

int
main(int argc, char** argv)
{
    if (argc < 4)
    {
        std::cerr << "Usage:\n  "
                  << argv[0]
                  << " colorchecker.csv illuminant.csv cmfobserver.csv\n";
        return 1;
    }

    try
    {
        auto patches = parse_colorchecker_csv(argv[1]);
        auto illuminant = parse_illuminant_csv(argv[2]);
        auto cmfs = parse_cmf_csv(argv[3]);
        
        std::cout << "Loaded " << patches.size()
                  << " ColorChecker patches\n";
        
        std::cout << "Loaded illuminant SPD with "
                  << illuminant.samples.size()
                  << " samples\n";

        std::cout << "Loaded CMFs with "
                  << cmfs.size()
                  << " samples\n\n";
        
        std::vector<RGB8> patch_colors;
        
        constexpr double DELTA_NM = 10.0; // delta in reflectance + illuminant csv
        for (const auto& patch : patches)
        {
            auto illuminated = combine_reflectance_with_illuminant(
                patch.samples,
                illuminant.samples
            );

            XYZ xyz = integrate_xyz(
                illuminated,
                cmfs,
                illuminant.samples,
                DELTA_NM
            );
            
            // scene exposure (in stops)
            double exposure_ev = 0.0;
            double exposure_scale = std::pow(2.0, exposure_ev);

            xyz.z *= exposure_scale;
            xyz.y *= exposure_scale;
            xyz.z *= exposure_scale;
            
            // white-balance / adapt from measured CCT to D65
            xyz = adapt_measured_kelvin_to_D65(xyz, 5392.0);

            auto rgb  = xyz_to_srgb(xyz.x, xyz.y, xyz.z);
            auto rgb8 = srgb_to_8bit(rgb.r, rgb.g, rgb.b);
            patch_colors.push_back(rgb8);
            
            std::cout << "Patch " << patch.number
                      << " (" << patch.name << "): "
                      << "X=" << xyz.x << " "
                      << "Y=" << xyz.y << " "
                      << "Z=" << xyz.z << " | "
                      << "sRGB=("
                      << rgb.r << ", "
                      << rgb.g << ", "
                      << rgb.b << ") "
                      << "("
                      << rgb8.r << ", "
                      << rgb8.g << ", "
                      << rgb8.b << ")\n";
        }
        
        constexpr int cols = 6;
        constexpr int rows = 4;
        constexpr int size = 64;
        
        auto pixels = expand_patches(
            patch_colors,
            cols,
            rows,
            size
        );

        write_tga_rgb8(
            "colorchecker.tga",
            cols * size,
            rows * size,
            pixels
        );
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }

    return 0;
}
