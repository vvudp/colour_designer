from pathlib import Path
import argparse

import cv2
import numpy as np
from PIL import Image

parser = argparse.ArgumentParser(
    description="Generate mutually exclusive upper/lower masks for the supplied 717x2048 machine rendering."
)
parser.add_argument("--input", required=True, type=Path)
parser.add_argument("--output", required=True, type=Path)
args = parser.parse_args()

src_path = args.input
out_dir = args.output
out_dir.mkdir(parents=True, exist_ok=True)

rgba = np.array(Image.open(src_path).convert("RGBA"))
rgb_u8 = rgba[..., :3]
rgb = rgb_u8.astype(np.float32) / 255.0
h, w = rgb.shape[:2]

hsv = cv2.cvtColor(rgb_u8, cv2.COLOR_RGB2HSV)
sat = hsv[..., 1].astype(np.float32) / 255.0
val = hsv[..., 2].astype(np.float32) / 255.0
gray = cv2.cvtColor(rgb_u8, cv2.COLOR_RGB2GRAY)
y_grid = np.arange(h, dtype=np.int32)[:, None]

# The points below are traced against machine_original.png (717 x 2048).
# upper_poly only supplies a conservative geometric envelope.  The real
# upper/lower split is snapped to the strong luminance edge in the source.
upper_poly = np.array([
    [117, 75], [246, 29], [351, 3], [580, 34], [630, 73],
    [658, 306], [641, 350], [614, 383], [594, 468], [569, 632],
    [535, 807], [494, 925], [444, 994], [385, 1033], [314, 1045],
    [249, 1027], [193, 986], [144, 930], [108, 855], [79, 754],
    [62, 645], [56, 529], [56, 342], [69, 220], [88, 125]
], dtype=np.int32)

# A broad envelope for the complete main lower housing.  It intentionally
# overlaps the upper shell; subtracting the final upper mask makes the two
# regions exact complements at their shared structural seam.
main_body_poly = np.array([
    [58, 292], [650, 292], [659, 330], [659, 1515], [651, 1588],
    [633, 1651], [594, 1707], [538, 1743], [454, 1762],
    [159, 1739], [105, 1701], [87, 1625], [80, 680], [58, 540]
], dtype=np.int32)

base_poly = np.array([
    [42, 1642], [91, 1611], [124, 1696], [224, 1717], [426, 1733],
    [513, 1745], [591, 1702], [650, 1750], [676, 1821], [671, 1876],
    [642, 1914], [595, 1936], [542, 1939], [502, 1968], [420, 1978],
    [127, 1941], [62, 1913], [12, 1865], [0, 1808], [9, 1722]
], dtype=np.int32)

rear_cover_poly = np.array([
    [618, 1366], [676, 1387], [709, 1435], [716, 1512],
    [694, 1577], [661, 1605], [622, 1593]
], dtype=np.int32)


def polygon_mask(points: np.ndarray) -> np.ndarray:
    mask = np.zeros((h, w), np.uint8)
    cv2.fillPoly(mask, [points], 255)
    return mask


upper_geo = polygon_mask(upper_poly)
main_body_geo = polygon_mask(main_body_poly)
base_geo = polygon_mask(base_poly)
rear_geo = polygon_mask(rear_cover_poly)

# Detect only near-white canvas pixels connected to the image border.
near_white = (
    (rgba[..., 0] >= 249)
    & (rgba[..., 1] >= 249)
    & (rgba[..., 2] >= 249)
).astype(np.uint8)
_, labels = cv2.connectedComponents(near_white, connectivity=8)
border_labels = set(np.unique(np.concatenate([
    labels[0, :], labels[-1, :], labels[:, 0], labels[:, -1]
])))
border_labels.discard(0)
background = np.isin(labels, list(border_labels))

# Components that must stay in their original colour.  The upper cut-out
# follows the visible bezel tightly.  The lower cut-out is slightly larger so
# the broad lower envelope can never leak through underneath the display.
upper_fixed = np.zeros((h, w), np.uint8)
lower_fixed = np.zeros((h, w), np.uint8)
upper_screen_poly = np.array([
    [197, 157], [397, 169], [382, 318], [184, 304]
], dtype=np.int32)
lower_screen_poly = np.array([
    [194, 151], [403, 162], [389, 331], [179, 316]
], dtype=np.int32)
cv2.fillPoly(upper_fixed, [upper_screen_poly], 255)
cv2.fillPoly(lower_fixed, [lower_screen_poly], 255)
cv2.rectangle(upper_fixed, (367, 0), (468, 49), 255, -1)
cv2.rectangle(lower_fixed, (367, 0), (468, 49), 255, -1)

for x1, y1, x2, y2, threshold in [
    (238, 365, 310, 474, 0.68),
    (238, 542, 309, 628, 0.66),
    (207, 638, 365, 710, 0.76),
]:
    dark = (val[y1:y2, x1:x2] < threshold).astype(np.uint8) * 255
    dark = cv2.dilate(dark, np.ones((3, 3), np.uint8), iterations=1)
    upper_fixed[y1:y2, x1:x2] = np.maximum(upper_fixed[y1:y2, x1:x2], dark)
    lower_fixed[y1:y2, x1:x2] = np.maximum(lower_fixed[y1:y2, x1:x2], dark)

# Keep the darker top deck in the upper material.  Below y=300 the true
# upper/lower junction is high contrast, so a luminance gate snaps the mask
# to the rendered housing edge instead of trusting an approximate polygon.
upper_gate = np.where(y_grid < 300, val > 0.31, gray >= 126)
upper_hard = (
    (upper_geo > 0)
    & (~background)
    & (sat < 0.30)
    & upper_gate
).astype(np.uint8) * 255
upper_hard[upper_fixed > 0] = 0

# The neutral collar below the emergency stop belongs to the upper shell.
collar = np.zeros((h, w), np.uint8)
cv2.ellipse(collar, (420, 89), (42, 20), 0, 0, 360, 255, -1)
collar_pixels = (
    (collar > 0)
    & (~background)
    & (sat < 0.22)
    & (val > 0.50)
).astype(np.uint8) * 255
upper_hard = np.maximum(upper_hard, collar_pixels)
upper_hard = cv2.morphologyEx(
    upper_hard, cv2.MORPH_CLOSE, np.ones((3, 3), np.uint8), iterations=1
)

# Main lower body.  The broad envelope reaches all the way to the upper
# shell, then upper_hard is subtracted.  This removes the old uncovered grey
# rim and guarantees that the real housing seam is represented by one label.
lower_body_hard = (
    (main_body_geo > 0)
    & (~background)
    & (sat < 0.35)
    & (val > 0.10)
).astype(np.uint8) * 255
lower_body_hard[lower_fixed > 0] = 0
lower_body_hard[(val > 0.985) & (sat < 0.02)] = 0
lower_body_hard[upper_hard > 0] = 0

# Base and rear cover are now part of the same lower region.  Only the dark
# wheel components are cut out; their light outer housings remain recolorable.
lower_base_hard = (
    ((base_geo > 0) | (rear_geo > 0))
    & (~background)
    & (sat < 0.30)
    & (val > 0.30)
).astype(np.uint8) * 255

left_wheel_poly = np.array([
    [0, 1794], [18, 1787], [49, 1804], [61, 1841],
    [56, 1907], [42, 1932], [12, 1927], [0, 1908]
], dtype=np.int32)
right_wheel_poly = np.array([
    [526, 1925], [563, 1915], [594, 1941], [603, 2048], [526, 2048]
], dtype=np.int32)
for wheel_poly in (left_wheel_poly, right_wheel_poly):
    wheel_geo = polygon_mask(wheel_poly)
    wheel_dark = (wheel_geo > 0) & (val < 0.86)
    lower_base_hard[wheel_dark] = 0

# One lower mask: main body + base + rear cover.
lower_hard = np.maximum(lower_body_hard, lower_base_hard)
lower_hard[upper_hard > 0] = 0

# Joint feathering is important.  Blurring the two masks independently causes
# overlap or gaps at the shared seam.  Here the blurred union supplies the
# total alpha and the two labels divide it proportionally, so:
#     upper_alpha + lower_alpha == housing_alpha
# at every shared-edge pixel.
def joint_feather(
    upper: np.ndarray,
    lower: np.ndarray,
    sigma: float = 0.72,
) -> tuple[np.ndarray, np.ndarray]:
    union = ((upper > 0) | (lower > 0)).astype(np.uint8) * 255
    upper_blur = cv2.GaussianBlur(upper, (0, 0), sigmaX=sigma, sigmaY=sigma).astype(np.float32)
    lower_blur = cv2.GaussianBlur(lower, (0, 0), sigmaX=sigma, sigmaY=sigma).astype(np.float32)
    union_alpha = cv2.GaussianBlur(union, (0, 0), sigmaX=sigma, sigmaY=sigma).astype(np.float32)

    label_sum = upper_blur + lower_blur
    upper_alpha = np.where(
        label_sum > 1.0e-3,
        union_alpha * upper_blur / np.maximum(label_sum, 1.0e-3),
        0.0,
    )
    lower_alpha = np.where(
        label_sum > 1.0e-3,
        union_alpha * lower_blur / np.maximum(label_sum, 1.0e-3),
        0.0,
    )

    for alpha in (upper_alpha, lower_alpha):
        alpha[alpha < 1.0] = 0.0
        alpha[alpha > 254.0] = 255.0

    return upper_alpha.astype(np.uint8), lower_alpha.astype(np.uint8)


upper_mask, lower_mask = joint_feather(upper_hard, lower_hard)

Image.fromarray(rgba).save(out_dir / "machine_original.png")
Image.fromarray(upper_mask, mode="L").save(out_dir / "upper_mask.png")
Image.fromarray(lower_mask, mode="L").save(out_dir / "lower_mask.png")

# Debug overlay: blue = upper, orange = the single merged lower region.
visualization = rgba[..., :3].copy().astype(np.float32)
for mask, color in [
    (upper_mask, (50, 170, 255)),
    (lower_mask, (255, 120, 50)),
]:
    alpha = (mask.astype(np.float32) / 255.0 * 0.45)[..., None]
    visualization = visualization * (1.0 - alpha) + np.array(color, np.float32) * alpha
Image.fromarray(np.clip(visualization, 0, 255).astype(np.uint8)).save(
    out_dir / "mask_debug.png"
)

# CPU reference preview using the same two-region algorithm as the application.
def srgb_to_linear(colour: np.ndarray) -> np.ndarray:
    return np.where(
        colour <= 0.04045,
        colour / 12.92,
        ((colour + 0.055) / 1.055) ** 2.4,
    )


def linear_to_srgb(colour: np.ndarray) -> np.ndarray:
    return np.where(
        colour <= 0.0031308,
        12.92 * colour,
        1.055 * np.power(np.maximum(colour, 0), 1.0 / 2.4) - 0.055,
    )


linear = srgb_to_linear(rgb)
luminance = linear @ np.array([0.2126, 0.7152, 0.0722], np.float32)


def median_reference(mask: np.ndarray) -> float:
    values = luminance[mask > 128]
    return float(np.median(values)) if values.size else 0.5


upper_reference = median_reference(upper_mask)
lower_reference = median_reference(lower_mask)
print("references", upper_reference, lower_reference)


def apply_region(
    output: np.ndarray,
    mask: np.ndarray,
    target_srgb: tuple[int, int, int],
    reference: float,
    highlight: float = 0.22,
    shadow: float = 1.0,
    brightness: float = 1.0,
    matte: float = 0.88,
) -> np.ndarray:
    alpha = (mask.astype(np.float32) / 255.0)[..., None]
    target = srgb_to_linear(np.array(target_srgb, np.float32) / 255.0) * brightness
    relative = np.clip(luminance / max(reference, 1.0e-4), 0.055, 2.4)
    exponent = np.interp(shadow, [0, 2], [0.82, 1.22])
    shade = np.power(relative, exponent)
    shade = np.minimum(shade, 1.85 - matte * 0.35)
    painted = target[None, None, :] * shade[..., None]
    highlight_alpha = np.clip((relative - 1.02) / (1.72 - 1.02), 0, 1)
    neutral = np.repeat(luminance[..., None], 3, axis=2)
    painted = np.maximum(
        painted,
        neutral * (highlight_alpha[..., None] * highlight * (1.0 - matte * 0.45)),
    )
    return output * (1.0 - alpha) + painted * alpha


preview_linear = linear.copy()
preview_linear = apply_region(
    preview_linear, upper_mask, (198, 205, 210), upper_reference
)
preview_linear = apply_region(
    preview_linear, lower_mask, (96, 151, 180), lower_reference
)
preview = linear_to_srgb(np.clip(preview_linear, 0, 1))
Image.fromarray((np.clip(preview, 0, 1) * 255).astype(np.uint8)).save(
    out_dir / "recolor_preview.png"
)
