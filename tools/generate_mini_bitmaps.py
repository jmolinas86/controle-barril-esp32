from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "source"
OUTPUT = ROOT / "assets" / "ui"
CPP_DIR = ROOT / "src" / "ui" / "assets"


def fit_rgba(image: Image.Image, width: int, height: int, padding: int) -> Image.Image:
    alpha = image.getchannel("A")
    bbox = alpha.point(lambda value: 255 if value > 12 else 0).getbbox()
    if bbox is None:
        raise RuntimeError("No visible pixels found")
    cropped = image.crop(bbox)
    cropped.thumbnail((width - 2 * padding, height - 2 * padding), Image.Resampling.LANCZOS)
    result = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    result.alpha_composite(
        cropped, ((width - cropped.width) // 2, (height - cropped.height) // 2)
    )
    return result


def make_snowflake() -> Image.Image:
    source = Image.open(SOURCE / "header-snowflake-reference.png").convert("RGB")
    pixels = []
    for red, green, blue in source.get_flattened_data():
        strength = max(0, blue - ((red + green) // 2))
        alpha = max(0, min(255, (strength - 5) * 4))
        pixels.append((0, 174, 239, alpha))
    rgba = Image.new("RGBA", source.size)
    rgba.putdata(pixels)
    return fit_rgba(rgba, 18, 18, 1)


def make_thermometer() -> Image.Image:
    source = Image.open(SOURCE / "keezer-temperature-reference.png").convert("RGB")
    # Exclude the soft floor shadow from the reference; it becomes a hard
    # horizontal artifact after reducing the artwork to 36 x 54 pixels.
    source = source.crop((0, 0, source.width, source.height - 18))
    pixels = []
    for red, green, blue in source.get_flattened_data():
        distance_from_white = max(255 - red, 255 - green, 255 - blue)
        alpha = max(0, min(255, (distance_from_white - 3) * 4))
        pixels.append((red, green, blue, alpha))
    rgba = Image.new("RGBA", source.size)
    rgba.putdata(pixels)
    return fit_rgba(rgba, 36, 54, 1)


def make_keg(width: int, height: int) -> Image.Image:
    source = Image.open(SOURCE / "keg-reference.png").convert("RGB")
    # Crop away the decorative rounded-square frame before separating the
    # bright steel and blue cap from the navy background.
    source = source.crop((250, 105, 1005, 1090))
    pixels = []
    for red, green, blue in source.get_flattened_data():
        luminance = (3 * red + 6 * green + blue) // 10
        blue_accent = max(0, blue - max(red, green))
        steel_alpha = (luminance - 28) * 6
        cap_alpha = (blue_accent - 8) * 5
        alpha = max(0, min(255, max(steel_alpha, cap_alpha)))
        pixels.append((red, green, blue, alpha))
    rgba = Image.new("RGBA", source.size)
    rgba.putdata(pixels)
    return fit_rgba(rgba, width, height, 1)


def make_details_list() -> Image.Image:
    image = Image.new("RGBA", (18, 14), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    color = (238, 244, 250, 255)
    for y in (2, 6, 10):
        draw.ellipse((1, y, 3, y + 2), fill=color)
        draw.rounded_rectangle((6, y, 17, y + 2), radius=1, fill=color)
    return image


def make_freezer_nav() -> Image.Image:
    image = Image.new("RGBA", (18, 19), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    color = (238, 244, 250, 255)
    draw.rounded_rectangle((3, 0, 14, 17), radius=2, outline=color, width=2)
    draw.line((4, 8, 13, 8), fill=color, width=2)
    draw.rounded_rectangle((10, 3, 11, 6), radius=1, fill=color)
    draw.rounded_rectangle((10, 10, 11, 13), radius=1, fill=color)
    draw.rectangle((4, 17, 6, 18), fill=color)
    draw.rectangle((11, 17, 13, 18), fill=color)
    return image


def make_measurement_icon(crop: tuple[int, int, int, int]) -> Image.Image:
    source = Image.open(SOURCE / "measurement-icons-reference.png").convert("RGB")
    source = source.crop(crop)
    pixels = []
    for red, green, blue in source.get_flattened_data():
        intensity = max(red, green, blue)
        alpha = max(0, min(255, (intensity - 34) * 4))
        pixels.append((red, green, blue, alpha))
    rgba = Image.new("RGBA", source.size)
    rgba.putdata(pixels)
    return fit_rgba(rgba, 28, 30, 1)


def rgb565a8_bytes(image: Image.Image) -> bytes:
    colors = bytearray()
    alpha = bytearray()
    for red, green, blue, opacity in image.get_flattened_data():
        value = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)
        colors.extend((value & 0xFF, value >> 8))
        alpha.append(opacity)
    return bytes(colors + alpha)


def byte_lines(data: bytes) -> str:
    values = [f"0x{value:02X}" for value in data]
    return "\n".join(
        "    " + ", ".join(values[index : index + 12]) + ","
        for index in range(0, len(values), 12)
    )


def descriptor(name: str, image: Image.Image) -> str:
    data = rgb565a8_bytes(image)
    width, height = image.size
    return f"""alignas(4) const std::uint8_t {name}Map[] = {{
{byte_lines(data)}
}};

const lv_image_dsc_t {name} = {{
    {{LV_IMAGE_HEADER_MAGIC, LV_COLOR_FORMAT_RGB565A8, 0, {width}, {height}, {width * 2}, 0}},
    sizeof({name}Map),
    {name}Map,
    nullptr,
    nullptr,
}};
"""


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    CPP_DIR.mkdir(parents=True, exist_ok=True)

    snowflake = make_snowflake()
    thermometer = make_thermometer()
    keg_small = make_keg(36, 51)
    keg_large = make_keg(56, 76)
    details_list = make_details_list()
    freezer_nav = make_freezer_nav()
    measurement_tare = make_measurement_icon((30, 38, 120, 135))
    measurement_density = make_measurement_icon((510, 39, 602, 138))
    measurement_volume = make_measurement_icon((30, 180, 112, 287))
    measurement_weight = make_measurement_icon((510, 175, 602, 287))
    snowflake.save(OUTPUT / "header-snowflake-18.png")
    thermometer.save(OUTPUT / "keezer-temperature-36x54.png")
    keg_small.save(OUTPUT / "keg-36x51.png")
    keg_large.save(OUTPUT / "keg-56x76.png")
    details_list.save(OUTPUT / "details-list-18x14.png")
    freezer_nav.save(OUTPUT / "freezer-nav-18x19.png")
    measurement_tare.save(OUTPUT / "measurement-tare-28x30.png")
    measurement_density.save(OUTPUT / "measurement-density-28x30.png")
    measurement_volume.save(OUTPUT / "measurement-volume-28x30.png")
    measurement_weight.save(OUTPUT / "measurement-weight-28x30.png")

    header = """#pragma once

#include <lvgl.h>

namespace keezer::ui::assets {

extern const lv_image_dsc_t kHeaderSnowflake;
extern const lv_image_dsc_t kKeezerTemperature;
extern const lv_image_dsc_t kKegSmall;
extern const lv_image_dsc_t kKegLarge;
extern const lv_image_dsc_t kDetailsList;
extern const lv_image_dsc_t kFreezerNav;
extern const lv_image_dsc_t kMeasurementTare;
extern const lv_image_dsc_t kMeasurementDensity;
extern const lv_image_dsc_t kMeasurementVolume;
extern const lv_image_dsc_t kMeasurementWeight;

}  // namespace keezer::ui::assets
"""
    cpp = f"""// Generated by tools/generate_mini_bitmaps.py. Do not edit by hand.
#include "ui/assets/MiniBitmaps.h"

#include <cstdint>

namespace keezer::ui::assets {{

{descriptor("kHeaderSnowflake", snowflake)}
{descriptor("kKeezerTemperature", thermometer)}
{descriptor("kKegSmall", keg_small)}
{descriptor("kKegLarge", keg_large)}
{descriptor("kDetailsList", details_list)}
{descriptor("kFreezerNav", freezer_nav)}
{descriptor("kMeasurementTare", measurement_tare)}
{descriptor("kMeasurementDensity", measurement_density)}
{descriptor("kMeasurementVolume", measurement_volume)}
{descriptor("kMeasurementWeight", measurement_weight)}
}}  // namespace keezer::ui::assets
"""
    (CPP_DIR / "MiniBitmaps.h").write_text(header, encoding="utf-8", newline="\n")
    (CPP_DIR / "MiniBitmaps.cpp").write_text(cpp, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
