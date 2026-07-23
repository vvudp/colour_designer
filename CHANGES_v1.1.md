# v1.1 Mask boundary update

- Replaced `lower_body_mask.png` and `lower_base_mask.png` with one `lower_mask.png`.
- Rebuilt the upper/lower shared contour using a luminance-snapped upper edge and a broad complementary lower housing region.
- Added joint feathering so upper and lower alpha values share one housing alpha instead of being blurred independently.
- Updated OpenGL preview, CPU export, shader uniforms, Qt resources, documentation, and mask generator for the two-mask design.
- Removed the duplicate `qt_finalize_executable()` call.
- Replaced the deprecated `QImage::mirrored()` call with `QImage::flipped(Qt::Vertical)`.
